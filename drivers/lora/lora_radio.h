#ifndef LORA_RADIO_H
#define LORA_RADIO_H

#include <stdint.h>
#include <stdbool.h>

// ==========================================
// Registradores SX1276
// ==========================================
#define REG_FIFO                0x00
#define REG_OP_MODE             0x01
#define REG_FRF_MSB             0x06
#define REG_FRF_MID             0x07
#define REG_FRF_LSB             0x08
#define REG_PA_CONFIG           0x09
#define REG_FIFO_ADDR_PTR      0x0D
#define REG_FIFO_TX_BASE_ADDR  0x0E
#define REG_FIFO_RX_BASE_ADDR  0x0F
#define REG_FIFO_RX_CURRENT    0x10
#define REG_IRQ_FLAGS           0x12
#define REG_RX_NB_BYTES        0x13
#define REG_PKT_SNR_VALUE     0x19
#define REG_PKT_RSSI_VALUE     0x1A
#define REG_MODEM_CONFIG_1     0x1D
#define REG_MODEM_CONFIG_2     0x1E
#define REG_PAYLOAD_LENGTH     0x22
#define REG_MODEM_CONFIG_3     0x26
#define REG_SYNC_WORD           0x39
#define REG_DIO_MAPPING_1      0x40
#define REG_VERSION             0x42

// Modos de Operação
#define MODE_LONG_RANGE        0x80
#define MODE_SLEEP             0x00
#define MODE_STDBY             0x01
#define MODE_TX                0x03
#define MODE_RX_CONTINUOUS     0x05

// IRQ Flags
#define IRQ_TX_DONE            0x08
#define IRQ_RX_DONE            0x40
#define IRQ_CRC_ERROR          0x20
#define IRQ_ALL                0xFF

#define EXPECTED_VERSION       0x12

// ==========================================
// Resultado de recepção
// ==========================================
typedef struct {
    int   length;     // Bytes recebidos (0 = nenhum pacote)
    int   rssi;       // RSSI em dBm
    float snr;        // SNR em dB (relação sinal/ruído)
    bool  crc_ok;     // Verdadeiro se CRC está OK
} lora_rx_result_t;

// ==========================================
// API Pública
// ==========================================

/// Inicializa o barramento SPI e os GPIOs do módulo LoRa.
void lora_spi_init(void);

/// Executa o reset por hardware do módulo (pino RST).
void lora_hardware_reset(void);

/// Verifica a comunicação SPI lendo o registrador de versão.
/// Retorna true se o chip SX1276 for detectado.
bool lora_check_version(void);

/// Configura os parâmetros de rádio (frequência, BW, SF, CR, potência).
void lora_radio_init(void);

/// Altera o Spreading Factor (6–12). Coloca o rádio em STDBY automaticamente.
void lora_set_sf(uint8_t sf);

/// Envia um pacote e bloqueia até TxDone ou timeout.
/// Retorna true se o envio foi concluído com sucesso.
bool lora_send_packet(const uint8_t *data, uint8_t len);

/// Coloca o rádio em modo de escuta contínua.
/// Deve ser chamada uma vez; depois, use lora_check_rx() para verificar pacotes.
void lora_start_rx(void);

/// Verifica de forma não-bloqueante se um pacote foi recebido.
/// Preenche buffer e retorna o resultado. Não altera o modo do rádio.
lora_rx_result_t lora_check_rx(uint8_t *buffer, uint8_t max_len);

/// Coloca o rádio em STANDBY e limpa todas as flags de IRQ.
void lora_standby(void);

/// Lê o RSSI do último pacote recebido, em dBm.
int lora_get_rssi(void);

/// Lê o SNR do último pacote recebido, em dB.
float lora_get_snr(void);

/// Calcula distância estimada (metros) a partir do RSSI usando FSPL.
float lora_estimate_distance(int rssi);

// ==========================================
// Acesso direto a registradores (uso interno, mas exposto para debug)
// ==========================================
uint8_t lora_read_reg(uint8_t reg);
void    lora_write_reg(uint8_t reg, uint8_t val);

#endif // LORA_RADIO_H
