#include "pico/stdlib.h"
#include "buttons.h"
#include "config.h"

// Janela de tempo para detectar o combo A+B (ms).
// Quando um botão é pressionado, esperamos este tempo para ver se o outro
// também foi pressionado antes de emitir o evento individual.
#define COMBO_WINDOW_MS  80

static uint32_t last_event_time = 0;   // Timestamp do último evento emitido
static uint32_t press_start_time = 0;  // Quando o primeiro botão foi pressionado
static bool     waiting_combo = false; // Estamos na janela de espera do combo?

void buttons_init(void) {
    gpio_init(PIN_BTN_A);
    gpio_set_dir(PIN_BTN_A, GPIO_IN);
    gpio_pull_up(PIN_BTN_A);

    gpio_init(PIN_BTN_B);
    gpio_set_dir(PIN_BTN_B, GPIO_IN);
    gpio_pull_up(PIN_BTN_B);
}

btn_event_t buttons_poll(void) {
    uint32_t now = to_ms_since_boot(get_absolute_time());

    // Debounce global — bloqueia novos eventos por DEBOUNCE_MS após o último
    if (now - last_event_time < DEBOUNCE_MS) {
        return BTN_NONE;
    }

    bool a = (gpio_get(PIN_BTN_A) == 0);  // true = pressionado
    bool b = (gpio_get(PIN_BTN_B) == 0);

    // Nenhum botão pressionado
    if (!a && !b) {
        // Se estávamos esperando o combo e a janela expirou sem o segundo botão,
        // isso não deveria acontecer aqui (botões já soltos), então reseta.
        waiting_combo = false;
        return BTN_NONE;
    }

    // Ambos pressionados → combo imediato!
    if (a && b) {
        waiting_combo = false;
        last_event_time = now;
        return BTN_AB_PRESS;
    }

    // Apenas um botão pressionado
    if (!waiting_combo) {
        // Primeiro frame em que detectamos a pressão → inicia janela do combo
        waiting_combo = true;
        press_start_time = now;
        return BTN_NONE;  // Ainda não emite — espera para ver se o outro vem junto
    }

    // Estamos na janela de espera do combo
    if (now - press_start_time < COMBO_WINDOW_MS) {
        return BTN_NONE;  // Ainda dentro da janela, continua esperando
    }

    // Janela expirou — o segundo botão não veio. Emite o evento individual.
    waiting_combo = false;
    last_event_time = now;

    if (a) return BTN_A_PRESS;
    if (b) return BTN_B_PRESS;

    return BTN_NONE;
}
