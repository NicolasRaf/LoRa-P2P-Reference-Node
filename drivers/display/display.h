#ifndef DISPLAY_H
#define DISPLAY_H

/// Inicializa o barramento I2C e o display OLED SSD1306.
void display_init(void);

/// Atualiza o conteúdo do display com até 4 linhas de texto.
/// Passa NULL para pular uma linha.
void display_update(const char *line1, const char *line2, const char *line3);
void display_update4(const char *line1, const char *line2, const char *line3, const char *line4);

#endif // DISPLAY_H
