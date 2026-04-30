#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdbool.h>

/// Tipos de evento de botão
typedef enum {
    BTN_NONE,       // Nenhum evento
    BTN_A_PRESS,    // Botão A pressionado
    BTN_B_PRESS,    // Botão B pressionado
    BTN_AB_PRESS,   // Ambos os botões pressionados simultaneamente
} btn_event_t;

/// Inicializa os GPIOs dos botões com pull-up.
void buttons_init(void);

/// Verifica o estado dos botões e retorna o evento detectado.
/// Usa detecção por pressão com janela de combo para A+B.
/// Deve ser chamada a cada iteração do loop principal.
btn_event_t buttons_poll(void);

#endif // BUTTONS_H
