#ifndef __KEY_H_
#define __KEY_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

/* 引脚定义 */
#define KEY1_GPIO_PIN   GPIO_NUM_0
#define KEY2_GPIO_PIN   GPIO_NUM_1

/* IO操作 */
#define KEY1            gpio_get_level(KEY1_GPIO_PIN)
#define KEY2            gpio_get_level(KEY2_GPIO_PIN)

/* 按键按下返回值 */
#define KEY_NONE        1
#define KEY1_PRES       0
#define KEY2_PRES       0

/* 函数声明 */
void key_init(void);
uint8_t key_scan(uint8_t mode);
bool check_key1_long_press(void);

#endif