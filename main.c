// ==========================================
// LoRa P2P — Compatível com ROS2_LR (ESP32)
// BitDogLab (Raspberry Pi Pico) + SX1276
//
// Firmware universal: mesmo binário para ambas as placas.
// A+B juntos alterna entre TRANSMISSOR (Gateway) e RECEPTOR (Alvo).
// A sozinho altera o Spreading Factor (SF7–SF12).
// B sozinho pausa/retoma a operação.
// ==========================================

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/watchdog.h"

#include "config.h"
#include "lora_radio.h"
#include "display.h"
#include "buttons.h"

// ==========================================
// Protocolo ROS2_LR / TCC Iglésio
// ==========================================
#define GATEWAY_ID      0
#define ALVO_ID         1
#define MSG_PING_TEST   0xE0

// Estrutura de pacote exata (21 bytes) usada pelo ESP32
typedef struct {
  uint8_t target_id;
  uint8_t sender_id;
  uint8_t msg_type;
  uint8_t raw_data[16];
  uint16_t checksum;
} __attribute__((packed)) lora_packet_t;

// ==========================================
// Média Móvel de RSSI
// ==========================================
typedef struct {
    int  buffer[RSSI_AVG_WINDOW];
    int  index;
    int  count;
    long sum;
} rssi_avg_t;

static void rssi_avg_reset(rssi_avg_t *avg) {
    memset(avg, 0, sizeof(*avg));
}

static void rssi_avg_push(rssi_avg_t *avg, int rssi) {
    if (avg->count >= RSSI_AVG_WINDOW) {
        avg->sum -= avg->buffer[avg->index];
    } else {
        avg->count++;
    }
    avg->buffer[avg->index] = rssi;
    avg->sum += rssi;
    avg->index = (avg->index + 1) % RSSI_AVG_WINDOW;
}

static int rssi_avg_get(const rssi_avg_t *avg) {
    if (avg->count == 0) return 0;
    return (int)(avg->sum / avg->count);
}

// ==========================================
// Estado da aplicação
// ==========================================
typedef struct {
    bool     is_transmitter;
    bool     is_paused;
    uint8_t  current_sf;
    int      tx_counter;
    uint32_t last_send_time;
    // Estatísticas (PDR)
    int      pkts_sent;
    int      acks_received;
    uint32_t ultima_latencia_ms;
    // Média móvel
    rssi_avg_t rssi_avg;
} app_state_t;

// ==========================================
// Funções auxiliares
// ==========================================
static const char* mode_label(const app_state_t *s) {
    return s->is_transmitter ? "GATEWAY (PING)" : "ALVO (PONG)";
}

static float get_pdr(const app_state_t *s) {
    if (s->pkts_sent == 0) return 100.0f;
    return (float)s->acks_received / (float)s->pkts_sent * 100.0f;
}

static void show_status(const app_state_t *s) {
    char sf_line[32], stats[32];
    snprintf(sf_line, sizeof(sf_line), "SF: %d", s->current_sf);
    snprintf(stats, sizeof(stats), "PKT:%d PDR:%.0f%%",
             s->pkts_sent, get_pdr(s));
    display_update4(
        mode_label(s),
        sf_line,
        s->is_paused ? "PAUSADO" : "RODANDO",
        stats
    );
}

// ==========================================
// Rotina do Transmissor (Gateway)
// ==========================================
static void transmitter_tick(app_state_t *s) {
    if (s->is_paused) return;

    uint32_t now = to_ms_since_boot(get_absolute_time());
    if (now - s->last_send_time < TX_INTERVAL_MS) return;

    s->tx_counter++;
    char info[32];
    snprintf(info, sizeof(info), "Ping #%d (SF%d)", s->tx_counter, s->current_sf);

    // Monta o pacote de dados no formato do ESP32
    lora_packet_t pkt;
    memset(&pkt, 0, sizeof(pkt));
    pkt.target_id = ALVO_ID;
    pkt.sender_id = GATEWAY_ID;
    pkt.msg_type  = MSG_PING_TEST;
    
    s->pkts_sent++;
    bool ack_ok = false;

    // Tenta enviar + receber ACK, com retransmissões
    for (int attempt = 0; attempt <= MAX_RETRIES && !ack_ok; attempt++) {
        watchdog_update(); // Alimenta o watchdog

        if (attempt > 0) {
            printf("[TX] Retransmissao %d/%d do pacote #%d\n",
                   attempt, MAX_RETRIES, s->tx_counter);
            char retry_msg[32];
            snprintf(retry_msg, sizeof(retry_msg), "Retry %d/%d", attempt, MAX_RETRIES);
            display_update4("GATEWAY (PING)", info, retry_msg, NULL);
            sleep_ms(200);
        } else {
            display_update4("GATEWAY (PING)", info, "Transmitindo...", NULL);
        }

        uint32_t tempo_enviado = to_ms_since_boot(get_absolute_time());
        // Embala a struct PingPayload (8 bytes)
        memcpy(&pkt.raw_data[0], &tempo_enviado, 4);
        memcpy(&pkt.raw_data[4], &s->ultima_latencia_ms, 4);

        if (!lora_send_packet((const uint8_t *)&pkt, sizeof(pkt))) {
            display_update4("GATEWAY (PING)", info, "ERRO: TX Timeout", NULL);
            continue; 
        }

        lora_start_rx();

        if (attempt == 0) {
            display_update4("GATEWAY (PING)", info, "Aguardando Pong...", NULL);
        }

        uint32_t ack_start = to_ms_since_boot(get_absolute_time());

        while (to_ms_since_boot(get_absolute_time()) - ack_start < ACK_TIMEOUT_MS) {
            watchdog_update();
            uint8_t ack_buf[64];
            lora_rx_result_t rx = lora_check_rx(ack_buf, sizeof(ack_buf) - 1);

            // Verifica se tem o tamanho exato da struct (21 bytes)
            if (rx.length == sizeof(lora_packet_t) && rx.crc_ok) {
                lora_packet_t *rpkt = (lora_packet_t*)ack_buf;
                
                if (rpkt->target_id == GATEWAY_ID && rpkt->msg_type == MSG_PING_TEST) {
                    uint32_t tempo_retornado;
                    memcpy(&tempo_retornado, &rpkt->raw_data[0], 4);

                    // Só aceita se for o Pong do Ping que acabamos de enviar
                    if (tempo_retornado == tempo_enviado) {
                        uint32_t latencia_bruta = to_ms_since_boot(get_absolute_time()) - tempo_retornado;
                        // Subtrai o delay fixo do Alvo
                        uint32_t latency = (latencia_bruta > ACK_REPLY_DELAY_MS) ? (latencia_bruta - ACK_REPLY_DELAY_MS) : 0;
                        s->ultima_latencia_ms = latency;

                        rssi_avg_push(&s->rssi_avg, rx.rssi);
                        int avg_rssi = rssi_avg_get(&s->rssi_avg);
                        float dist = lora_estimate_distance(avg_rssi);

                        s->acks_received++;

                        printf("--> [GW] PONG OK #%d | Lat:%lums RSSI:%d SNR:%.1f Dist:%.1fm PDR:%.1f%%\n",
                               s->tx_counter, latency, rx.rssi, rx.snr, dist, get_pdr(s));

                        char l1[32], l2[32], l3[32];
                        snprintf(l1, sizeof(l1), "Lat:%lums SNR:%.1fdB", latency, rx.snr);
                        snprintf(l2, sizeof(l2), "RSSI:%d Dist:%.1fm", avg_rssi, dist);
                        snprintf(l3, sizeof(l3), "PDR:%.1f%% (%d/%d)",
                                 get_pdr(s), s->acks_received, s->pkts_sent);
                        display_update4("PONG RECEBIDO!", l1, l2, l3);
                        ack_ok = true;
                        break;
                    }
                }
            }
            sleep_ms(5);
        }

        lora_standby();
    }

    if (!ack_ok) {
        printf("[GW] FALHA TOTAL pacote #%d apos %d tentativas | PDR:%.1f%%\n",
               s->tx_counter, MAX_RETRIES + 1, get_pdr(s));
        char stats[32];
        snprintf(stats, sizeof(stats), "PDR:%.1f%% (%d/%d)",
                 get_pdr(s), s->acks_received, s->pkts_sent);
        display_update4("GATEWAY (PING)", info, "Falha: Sem Pong", stats);
    }

    s->last_send_time = to_ms_since_boot(get_absolute_time());
}

// ==========================================
// Rotina do Receptor (Alvo)
// ==========================================
static void receiver_tick(app_state_t *s) {
    if (s->is_paused) return;

    uint8_t rx_buf[64];
    lora_rx_result_t rx = lora_check_rx(rx_buf, sizeof(rx_buf) - 1);

    if (rx.length == 0 || !rx.crc_ok) return;

    // Se o pacote for do mesmo tamanho da nossa struct padrão ESP32
    if (rx.length == sizeof(lora_packet_t)) {
        lora_packet_t *pkt = (lora_packet_t*)rx_buf;

        if (pkt->target_id == ALVO_ID && pkt->msg_type == MSG_PING_TEST) {
            uint32_t tempo_recebido;
            uint32_t latencia_anterior;
            memcpy(&tempo_recebido, &pkt->raw_data[0], 4);
            memcpy(&latencia_anterior, &pkt->raw_data[4], 4);

            s->pkts_sent++; // Conta pacotes recebidos válidos
            
            rssi_avg_push(&s->rssi_avg, rx.rssi);
            int avg_rssi = rssi_avg_get(&s->rssi_avg);
            float dist = lora_estimate_distance(avg_rssi);

            printf("--> [ALVO] PING rx! RSSI:%d SNR:%.1f Dist:%.1fm Lat Ant:%lums\n",
                   rx.rssi, rx.snr, dist, latencia_anterior);

            char l1[32], l2[32], l3[32];
            snprintf(l1, sizeof(l1), "Lat Ant: %lums", latencia_anterior);
            snprintf(l2, sizeof(l2), "RSSI:%d SNR:%.1f", rx.rssi, rx.snr);
            snprintf(l3, sizeof(l3), "Total Recebidos: %d", s->pkts_sent);
            display_update4("PING RECEBIDO!", l1, l2, l3);

            // Monta a resposta (Pong) idêntica ao que o ESP32 Receptor faria
            lora_packet_t ack_pkt;
            memset(&ack_pkt, 0, sizeof(ack_pkt));
            ack_pkt.target_id = pkt->sender_id; // Devolve para o Gateway
            ack_pkt.sender_id = ALVO_ID;
            ack_pkt.msg_type  = MSG_PING_TEST;
            memcpy(&ack_pkt.raw_data[0], &tempo_recebido, 4); 

            sleep_ms(ACK_REPLY_DELAY_MS);
            lora_send_packet((const uint8_t *)&ack_pkt, sizeof(ack_pkt));

            // Volta para modo de escuta
            lora_start_rx();
        }
    } else {
        // Se receber dados estranhos que não são da struct, apenas ignora
        // mas pode mostrar no Serial para debug
        rx_buf[rx.length] = '\0';
        printf("--> [RX UNKNOWN] %d bytes recebidos.\n", rx.length);
    }
}

// ==========================================
// Processamento de botões
// ==========================================
static void handle_buttons(app_state_t *s) {
    btn_event_t evt = buttons_poll();
    if (evt == BTN_NONE) return;

    switch (evt) {
        case BTN_AB_PRESS:
            s->is_transmitter = !s->is_transmitter;
            s->is_paused = false;
            s->pkts_sent = 0;
            s->acks_received = 0;
            s->tx_counter = 0;
            s->ultima_latencia_ms = 0;
            rssi_avg_reset(&s->rssi_avg);
            lora_standby();

            printf("\n=== MODO ALTERADO PARA %s ===\n",
                   s->is_transmitter ? "GATEWAY (PING)" : "ALVO (PONG)");

            display_update("SISTEMA", "MODO ALTERADO!", mode_label(s));
            sleep_ms(MODE_SWITCH_MS);

            if (!s->is_transmitter) {
                lora_start_rx();
            }
            s->last_send_time = to_ms_since_boot(get_absolute_time());
            show_status(s);
            break;

        case BTN_A_PRESS:
            s->current_sf++;
            if (s->current_sf > LORA_MAX_SF) s->current_sf = LORA_DEFAULT_SF;
            lora_set_sf(s->current_sf);

            if (!s->is_transmitter && !s->is_paused) {
                lora_start_rx();
            }
            show_status(s);
            break;

        case BTN_B_PRESS:
            s->is_paused = !s->is_paused;
            if (s->is_paused) {
                lora_standby();
            } else if (!s->is_transmitter) {
                lora_start_rx();
            }
            show_status(s);
            break;

        default:
            break;
    }
}

// ==========================================
// Ponto de entrada
// ==========================================
int main(void) {
    stdio_init_all();
    sleep_ms(3000);

    display_init();
    lora_spi_init();
    lora_hardware_reset();
    buttons_init();

    display_update("Modulo LoRa", "Verificando...", NULL);
    while (!lora_check_version()) {
        display_update("Erro LoRa", "Chip nao", "encontrado!");
        sleep_ms(2000);
    }
    display_update("Modulo LoRa", "Reconhecido!", "Versao: 0x12");
    sleep_ms(1000);

    lora_radio_init();

    watchdog_enable(WATCHDOG_TIMEOUT_MS, true);
    printf("Watchdog ativado (%d ms)\n", WATCHDOG_TIMEOUT_MS);

    // Estado inicial: Receptor (ALVO)
    app_state_t state = {
        .is_transmitter     = false,
        .is_paused          = false,
        .current_sf         = LORA_DEFAULT_SF,
        .tx_counter         = 0,
        .last_send_time     = 0,
        .pkts_sent          = 0,
        .acks_received      = 0,
        .ultima_latencia_ms = 0
    };
    rssi_avg_reset(&state.rssi_avg);

    printf("=== MODO ALVO (PONG) ATIVADO ===\n");
    show_status(&state);
    lora_start_rx();

    while (true) {
        watchdog_update(); 

        handle_buttons(&state);

        if (state.is_transmitter) {
            transmitter_tick(&state);
        } else {
            receiver_tick(&state);
        }

        sleep_ms(10);
    }

    return 0;
}
