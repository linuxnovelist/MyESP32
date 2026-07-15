#ifndef __OLED_H__
#define __OLED_H__

#include <stdint.h>
#include "driver/spi_master.h"

// 屏幕参数
#define OLED_WIDTH   128
#define OLED_HEIGHT  64

// 引脚定义（TDY_C6）
#define OLED_SPI_HOST       SPI2_HOST
#define OLED_MOSI_GPIO      21
#define OLED_SCLK_GPIO      20
#define OLED_CS_GPIO        15
#define OLED_DC_GPIO        23
#define OLED_RST_GPIO       22

// 函数声明
esp_err_t oled_init(void);
void oled_clear(void);
void oled_display_on(void);
void oled_display_off(void);
void oled_set_pos(uint8_t page, uint8_t col);
void oled_write_data(const uint8_t *data, size_t len);
void oled_write_cmd(uint8_t cmd);
void oled_fill_screen(uint8_t data);
void oled_test_pattern(void);

// 字符显示（简单 6x8 ASCII）
void oled_draw_char(uint8_t page, uint8_t col, char ch);
void oled_draw_string(uint8_t page, uint8_t col, const char *str);

#endif