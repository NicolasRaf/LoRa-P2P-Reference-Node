#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ssd1306.h"
#include "display.h"
#include "config.h"

static ssd1306_t oled;

void display_init(void) {
    i2c_init(I2C_PORT_DISP, 400 * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);

    if (ssd1306_init(&oled, 128, 64, 0x3C, I2C_PORT_DISP)) {
        ssd1306_clear(&oled);
        ssd1306_draw_string(&oled, 0, 0, 1, "LoRa Iniciando...");
        ssd1306_show(&oled);
    }
}

void display_update(const char *line1, const char *line2, const char *line3) {
    ssd1306_clear(&oled);
    if (line1) ssd1306_draw_string(&oled, 0,  0, 1, line1);
    if (line2) ssd1306_draw_string(&oled, 0, 16, 1, line2);
    if (line3) ssd1306_draw_string(&oled, 0, 32, 1, line3);
    ssd1306_show(&oled);
}

void display_update4(const char *line1, const char *line2, const char *line3, const char *line4) {
    ssd1306_clear(&oled);
    if (line1) ssd1306_draw_string(&oled, 0,  0, 1, line1);
    if (line2) ssd1306_draw_string(&oled, 0, 16, 1, line2);
    if (line3) ssd1306_draw_string(&oled, 0, 32, 1, line3);
    if (line4) ssd1306_draw_string(&oled, 0, 48, 1, line4);
    ssd1306_show(&oled);
}
