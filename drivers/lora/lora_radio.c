#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "lora_radio.h"
#include "config.h"

// ==========================================
// SPI — Primitivas de baixo nível
// ==========================================
static inline void cs_select(void) {
    asm volatile("nop \n nop \n nop");
    gpio_put(PIN_CS, 0);
    asm volatile("nop \n nop \n nop");
}

static inline void cs_deselect(void) {
    asm volatile("nop \n nop \n nop");
    gpio_put(PIN_CS, 1);
    asm volatile("nop \n nop \n nop");
}

uint8_t lora_read_reg(uint8_t reg) {
    uint8_t val;
    uint8_t addr = reg & 0x7F;
    cs_select();
    spi_write_blocking(SPI_PORT, &addr, 1);
    spi_read_blocking(SPI_PORT, 0, &val, 1);
    cs_deselect();
    return val;
}

void lora_write_reg(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = { reg | 0x80, val };
    cs_select();
    spi_write_blocking(SPI_PORT, buf, 2);
    cs_deselect();
}

static void lora_write_fifo(const uint8_t *data, uint8_t len) {
    uint8_t addr = REG_FIFO | 0x80;
    cs_select();
    spi_write_blocking(SPI_PORT, &addr, 1);
    spi_write_blocking(SPI_PORT, data, len);
    cs_deselect();
}

static void lora_read_fifo(uint8_t *data, uint8_t len) {
    uint8_t addr = REG_FIFO & 0x7F;
    cs_select();
    spi_write_blocking(SPI_PORT, &addr, 1);
    spi_read_blocking(SPI_PORT, 0, data, len);
    cs_deselect();
}

// ==========================================
// Inicialização de Hardware
// ==========================================
void lora_spi_init(void) {
    spi_init(SPI_PORT, 1000 * 1000); // 1 MHz

    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);

    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);

    gpio_init(PIN_RST);
    gpio_set_dir(PIN_RST, GPIO_OUT);
    gpio_put(PIN_RST, 1);
}

void lora_hardware_reset(void) {
    printf("Executando Hardware Reset do modulo LoRa...\n");
    gpio_put(PIN_RST, 0);
    sleep_ms(10);
    gpio_put(PIN_RST, 1);
    sleep_ms(10);
}

bool lora_check_version(void) {
    uint8_t version = lora_read_reg(REG_VERSION);
    if (version == EXPECTED_VERSION) {
        printf("SX1276 detectado (versao 0x%02X)\n", version);
        return true;
    }
    printf("FALHA: versao lida = 0x%02X (esperado 0x%02X)\n", version, EXPECTED_VERSION);
    return false;
}

// ==========================================
// Configuração do Rádio
// ==========================================
static void lora_set_frequency(uint32_t freq_hz) {
    uint64_t frf = ((uint64_t)freq_hz << 19) / 32000000;
    lora_write_reg(REG_FRF_MSB, (uint8_t)(frf >> 16));
    lora_write_reg(REG_FRF_MID, (uint8_t)(frf >> 8));
    lora_write_reg(REG_FRF_LSB, (uint8_t)(frf));
}

void lora_radio_init(void) {
    printf("Configurando parametros do radio LoRa...\n");

    // Sequência segura: Sleep → LoRa Sleep → LoRa Standby
    lora_write_reg(REG_OP_MODE, MODE_SLEEP);
    sleep_ms(10);
    lora_write_reg(REG_OP_MODE, MODE_LONG_RANGE | MODE_SLEEP);
    sleep_ms(10);
    lora_write_reg(REG_OP_MODE, MODE_LONG_RANGE | MODE_STDBY);
    sleep_ms(10);

    lora_set_frequency(LORA_FREQUENCY);

    lora_write_reg(REG_MODEM_CONFIG_1, 0x72); // BW=125kHz, CR=4/5, Explicit Header
    lora_write_reg(REG_MODEM_CONFIG_2, 0x74); // SF=7, CRC On
    lora_write_reg(REG_MODEM_CONFIG_3, 0x04); // AGC Auto On

    lora_write_reg(REG_SYNC_WORD, LORA_SYNC_WORD);
    lora_write_reg(REG_PA_CONFIG, LORA_TX_POWER);

    lora_write_reg(REG_FIFO_TX_BASE_ADDR, 0x00);
    lora_write_reg(REG_FIFO_RX_BASE_ADDR, 0x00);

    printf("Radio inicializado. Modo: STANDBY.\n");
}

void lora_set_sf(uint8_t sf) {
    if (sf < LORA_MIN_SF) sf = LORA_MIN_SF;
    if (sf > LORA_MAX_SF) sf = LORA_MAX_SF;

    lora_write_reg(REG_OP_MODE, MODE_LONG_RANGE | MODE_STDBY);

    // Atualiza SF no ModemConfig2
    uint8_t cfg2 = lora_read_reg(REG_MODEM_CONFIG_2);
    cfg2 = (cfg2 & 0x0F) | (sf << 4);
    lora_write_reg(REG_MODEM_CONFIG_2, cfg2);

    // LowDataRateOptimize: obrigatório para SF11 e SF12 com BW 125kHz
    // (duração do símbolo > 16ms). Bit 3 do REG_MODEM_CONFIG_3.
    uint8_t cfg3 = lora_read_reg(REG_MODEM_CONFIG_3);
    if (sf >= 11) {
        cfg3 |= 0x08;   // Liga LDRO (bit 3)
    } else {
        cfg3 &= ~0x08;  // Desliga LDRO (bit 3)
    }
    lora_write_reg(REG_MODEM_CONFIG_3, cfg3);

    printf("SF alterado para SF%d (LDRO: %s)\n", sf, (sf >= 11) ? "ON" : "OFF");
}

// ==========================================
// Transmissão
// ==========================================
bool lora_send_packet(const uint8_t *data, uint8_t len) {
    lora_write_reg(REG_OP_MODE, MODE_LONG_RANGE | MODE_STDBY);
    lora_write_reg(REG_IRQ_FLAGS, IRQ_ALL);

    lora_write_reg(REG_FIFO_ADDR_PTR, lora_read_reg(REG_FIFO_TX_BASE_ADDR));
    lora_write_reg(REG_PAYLOAD_LENGTH, len);
    lora_write_fifo(data, len);

    lora_write_reg(REG_OP_MODE, MODE_LONG_RANGE | MODE_TX);

    // Aguarda TxDone COM timeout de segurança
    uint32_t start = to_ms_since_boot(get_absolute_time());
    while (!(lora_read_reg(REG_IRQ_FLAGS) & IRQ_TX_DONE)) {
        if (to_ms_since_boot(get_absolute_time()) - start > TX_TIMEOUT_MS) {
            printf("ERRO: Timeout de transmissao!\n");
            lora_write_reg(REG_OP_MODE, MODE_LONG_RANGE | MODE_STDBY);
            lora_write_reg(REG_IRQ_FLAGS, IRQ_ALL);
            return false;
        }
        sleep_us(500);
    }

    lora_write_reg(REG_IRQ_FLAGS, IRQ_ALL);
    return true;
}

// ==========================================
// Recepção
// ==========================================
void lora_start_rx(void) {
    lora_write_reg(REG_OP_MODE, MODE_LONG_RANGE | MODE_STDBY);
    lora_write_reg(REG_IRQ_FLAGS, IRQ_ALL);
    lora_write_reg(REG_OP_MODE, MODE_LONG_RANGE | MODE_RX_CONTINUOUS);
}

lora_rx_result_t lora_check_rx(uint8_t *buffer, uint8_t max_len) {
    lora_rx_result_t result = { .length = 0, .rssi = 0, .snr = 0.0f, .crc_ok = true };

    uint8_t irq = lora_read_reg(REG_IRQ_FLAGS);

    if (!(irq & IRQ_RX_DONE)) {
        return result; // Nenhum pacote recebido
    }

    // Pacote recebido — limpa todas as flags imediatamente
    lora_write_reg(REG_IRQ_FLAGS, IRQ_ALL);

    // Verifica erro de CRC
    if (irq & IRQ_CRC_ERROR) {
        printf("Erro de CRC detectado!\n");
        result.crc_ok = false;
        return result;
    }

    // Lê RSSI e SNR do pacote recebido
    result.rssi = lora_read_reg(REG_PKT_RSSI_VALUE) - RSSI_OFFSET;
    result.snr  = (int8_t)lora_read_reg(REG_PKT_SNR_VALUE) / 4.0f;

    // Lê os dados do FIFO
    uint8_t nb = lora_read_reg(REG_RX_NB_BYTES);
    if (nb > max_len) nb = max_len;

    lora_write_reg(REG_FIFO_ADDR_PTR, lora_read_reg(REG_FIFO_RX_CURRENT));
    lora_read_fifo(buffer, nb);

    if (nb < max_len) buffer[nb] = '\0';

    result.length = nb;
    return result;
}

void lora_standby(void) {
    lora_write_reg(REG_OP_MODE, MODE_LONG_RANGE | MODE_STDBY);
    lora_write_reg(REG_IRQ_FLAGS, IRQ_ALL);
}

int lora_get_rssi(void) {
    return lora_read_reg(REG_PKT_RSSI_VALUE) - RSSI_OFFSET;
}

float lora_get_snr(void) {
    return (int8_t)lora_read_reg(REG_PKT_SNR_VALUE) / 4.0f;
}

float lora_estimate_distance(int rssi) {
    return powf(10.0f, (FSPL_REF_DBM - (float)rssi) / 20.0f);
}
