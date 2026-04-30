#ifndef CONFIG_H
#define CONFIG_H

// ==========================================
// Mapeamento de Pinos — SPI (SX1276 LoRa)
// ==========================================
#define SPI_PORT        spi0
#define PIN_MISO        16
#define PIN_CS          17
#define PIN_SCK         18
#define PIN_MOSI        19
#define PIN_RST         20
#define PIN_DIO0        4

// ==========================================
// Mapeamento de Pinos — I2C (SSD1306 OLED)
// ==========================================
#define I2C_PORT_DISP   i2c1
#define I2C_SDA_PIN     14
#define I2C_SCL_PIN     15

// ==========================================
// Mapeamento de Pinos — Botões (BitDogLab)
// ==========================================
#define PIN_BTN_A       5
#define PIN_BTN_B       6

// ==========================================
// Parâmetros de Rádio
// ==========================================
#define LORA_FREQUENCY      915000000   // 915 MHz (AU915/BR)
#define LORA_SYNC_WORD      0x12
#define LORA_TX_POWER       0x8F        // PA_BOOST +17 dBm
#define LORA_DEFAULT_SF     7
#define LORA_MIN_SF         6
#define LORA_MAX_SF         12

// ==========================================
// Timeouts (ms)
// ==========================================
#define TX_TIMEOUT_MS       5000    // Timeout máximo para envio de pacote
#define ACK_TIMEOUT_MS      2000    // Tempo de espera pelo ACK
#define ACK_REPLY_DELAY_MS  10      // Delay mínimo antes do RX enviar o ACK
#define TX_INTERVAL_MS      3000    // Intervalo entre transmissões
#define DEBOUNCE_MS         250     // Debounce dos botões
#define MODE_SWITCH_MS      1000    // Delay ao trocar de modo

// ==========================================
// Retransmissão
// ==========================================
#define MAX_RETRIES         2       // Tentativas extras em caso de falha no ACK

// ==========================================
// Média Móvel (suavização de RSSI/distância)
// ==========================================
#define RSSI_AVG_WINDOW     8       // Janela de média móvel (últimas N leituras)

// ==========================================
// Watchdog
// ==========================================
#define WATCHDOG_TIMEOUT_MS 8000    // Reset automático se travar por 8 segundos

// ==========================================
// RSSI / Distância
// ==========================================
#define RSSI_OFFSET         157     // Offset para converter valor bruto em dBm (HF band)
#define FSPL_REF_DBM        (-74.64f)  // Perda no espaço livre a 1 metro @915MHz

#endif // CONFIG_H
