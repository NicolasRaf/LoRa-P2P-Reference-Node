# LoRa P2P Interoperability Suite — BitDogLab (RP2040)

[Português] | [English]

---

## [Português]

Este projeto implementa um firmware de alta performance para comunicação LoRa Ponto-a-Ponto (P2P) utilizando a plataforma **BitDogLab** (Raspberry Pi Pico / RP2040) e o rádio **SX1276**. 

### 🌟 Destaques do Projeto
*   **Arquitetura Modular:** Separação clara entre lógica de aplicação, drivers de hardware e camada de configuração.
*   **Interoperabilidade Cross-Platform:** Projetado para se comunicar nativamente com a stack [ROS2_LR](https://github.com/iglesiojunior/ROS2_LR) (ESP32).
*   **Handshake Inteligente:** Protocolo Ping-Pong binário para medição precisa de latência real.

### 📊 Telemetria e Diagnóstico
*   **RSSI & SNR:** Monitoramento completo da potência e clareza do sinal.
*   **Média Móvel:** Suavização de RSSI para estimativa estável de distância.
*   **PDR (Packet Delivery Ratio):** Taxa de sucesso de entrega em tempo real.
*   **Auto-Retransmissão:** Reenvio automático de pacotes em caso de falha.
*   **Watchdog Timer:** Proteção de hardware contra travamentos de rádio/SPI.

### 🎮 Interface de Usuário
*   **Combo A+B:** Alterna entre **Gateway** e **Alvo**.
*   **Botão A:** Altera o **Spreading Factor** (SF7 a SF12).
*   **OLED 4 Linhas:** Exibe latência, RSSI, SNR, distância e PDR.

---

## [English]

This project implements a high-performance LoRa Point-to-Point (P2P) communication firmware for the **BitDogLab** platform (Raspberry Pi Pico / RP2040) and the **SX1276** radio.

### 🌟 Project Highlights
*   **Modular Architecture:** Clear separation between application logic, hardware drivers (LoRa, OLED, Buttons), and configuration layers.
*   **Cross-Platform Interoperability:** Designed to communicate natively with the [ROS2_LR](https://github.com/iglesiojunior/ROS2_LR) (ESP32) stack.
*   **Intelligent Handshake:** Binary Ping-Pong protocol for precise real-time latency measurement.

### 📊 Telemetry and Diagnostics
*   **RSSI & SNR:** Complete monitoring of signal strength and clarity.
*   **Moving Average:** RSSI smoothing for stable distance estimation.
*   **PDR (Packet Delivery Ratio):** Real-time packet success rate calculation.
*   **Auto-Retransmission:** Automatic packet resend upon acknowledgment failure.
*   **Watchdog Timer:** Hardware protection against Radio/SPI hangs.

### 🎮 User Interface
*   **Combo A+B:** Toggles between **Gateway** and **Target** modes.
*   **Button A:** Changes the **Spreading Factor** (SF7 to SF12) on the fly.
*   **4-Line OLED:** Displays latency, average RSSI, SNR, distance, and PDR statistics.

---

## 📂 Structure / Estrutura
```text
LoRa/
├── main.c                # Application logic & state machine
├── include/
│   └── config.h          # Configuration tokens & pin mapping
└── drivers/
    ├── lora/             # SX1276 Radio abstraction layer
    ├── display/          # Optimized SSD1306 OLED driver
    └── buttons/          # Debounce & combo detection logic
```

## 🛠️ Requirements / Requisitos
*   **Raspberry Pi Pico SDK** (v2.0.0+)
*   **CMake** & **Ninja/Make**
*   **Picotool** (for CLI flashing)

## 🔨 Build & Deploy
1.  Initialize build folder: `mkdir build && cd build`
2.  Generate build files: `cmake -G Ninja ..`
3.  Compile: `ninja`
4.  Flash (PowerShell): `./flash_all.ps1`

## 🔗 References / Referências
*   **Base Repository:** [iglesiojunior/ROS2_LR](https://github.com/iglesiojunior/ROS2_LR)

---
**Developed for advanced LoRa P2P network analysis.**
