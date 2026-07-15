/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "led.h"
#include "key.h"
#include "oled.h"
#include "iic_sensor.h"
#include "driver/i2c_master.h"  
#include "driver/i2c.h"

// 全局计数器
volatile uint32_t key1_count = 0;
volatile uint32_t key2_count = 0;

// 中断服务函数
void IRAM_ATTR key1_isr_handler(void* arg)
{
    if (gpio_get_level(KEY1_GPIO_PIN) == 0) {
        LED(0);
        key1_count++;
    }
}

void IRAM_ATTR key2_isr_handler(void* arg)
{
    if (gpio_get_level(KEY2_GPIO_PIN) == 0) {
        LED(1);
        key2_count++;
    }
}
void app_main(void)
{
    printf("Hello world!\n");

    /* Print chip information */
    esp_chip_info_t chip_info;
    uint32_t flash_size;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU core(s), %s%s%s%s, ",
           CONFIG_IDF_TARGET,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi/" : "",
           (chip_info.features & CHIP_FEATURE_BT) ? "BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "BLE" : "",
           (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");

    unsigned major_rev = chip_info.revision / 100;
    unsigned minor_rev = chip_info.revision % 100;
    printf("silicon revision v%d.%d, ", major_rev, minor_rev);
    if(esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
        printf("Get flash size failed\n");
        return;
    }

    printf("%" PRIu32 "MB %s flash\n", flash_size / (uint32_t)(1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    printf("Minimum free heap size: %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());

    // 初始化外设
    led_init();
    key_init();
    oled_init();
   // gpio_reset_pin((gpio_num_t)I2C_MASTER_SCL_IO); // 10
   // gpio_reset_pin((gpio_num_t)I2C_MASTER_SDA_IO); // 11
  //  gpio_set_direction((gpio_num_t)I2C_MASTER_SCL_IO, GPIO_MODE_INPUT);
  //  gpio_set_direction((gpio_num_t)I2C_MASTER_SDA_IO, GPIO_MODE_INPUT);
   // printf("I2C pins reset: SCL=%d, SDA=%d\n", I2C_MASTER_SCL_IO, I2C_MASTER_SDA_IO);
    // === 新增：初始化 I2C 传感器 ===
    esp_err_t err = bsp_i2c_sensors_init();
    if (err != ESP_OK) {
        printf("I2C sensor init failed!\n");
    }
    esp_err_t imu_err = bsp_icm20608_init();
    if (imu_err != ESP_OK) {
        printf("ICM-20608 init failed!\n");
    }
    // 安装中断
    gpio_install_isr_service(0);
    gpio_set_intr_type(KEY1_GPIO_PIN, GPIO_INTR_NEGEDGE);
    gpio_isr_handler_add(KEY1_GPIO_PIN, key1_isr_handler, NULL);
    gpio_set_intr_type(KEY2_GPIO_PIN, GPIO_INTR_NEGEDGE);
    gpio_isr_handler_add(KEY2_GPIO_PIN, key2_isr_handler, NULL);

    printf("例程六：OLED 显示 I2C 传感器数据\n");

    // 主循环：更新 OLED 显示
    // 显示缓冲区
    char line1[20], line2[20], line3[20], line4[20], line5[20], line6[20],line7[20],line8[20];
    int16_t temp_x10 = 0;      // 253 = 25.3°C
    uint16_t humi_x10 = 0;     // 456 = 45.6%
    uint16_t als_raw = 0;
    uint16_t ps_val = 0;
    uint32_t lux = 0;
    uint32_t update_counter = 0;
    int16_t accel[3] = {0}, gyro[3] = {0};

    while (1) {
    if (update_counter++ % 40 == 0) {
        bsp_sht30_read(&temp_x10, &humi_x10);
        bsp_ap3216c_read(&als_raw, &ps_val);
        lux = bsp_ap3216c_als_to_lux(als_raw);
        bsp_icm20608_read(accel, gyro); // ← 新增
    }

        const char* led_state = gpio_get_level(LED_GPIO_PIN) ? "OFF" : "ON";

        snprintf(line1, sizeof(line1), "LED:%s", led_state);
        snprintf(line2, sizeof(line2), "T:%d.%dC H:%d.%d",temp_x10/10, abs(temp_x10%10), humi_x10/10, humi_x10%10);
        snprintf(line3, sizeof(line3), "Lux:%lu", (unsigned long)lux);
        snprintf(line4, sizeof(line4), "PS:%u", ps_val);
        snprintf(line5, sizeof(line5), "AX:%5d GX:%5d", accel[0], gyro[0]);
        snprintf(line6, sizeof(line6), "AY:%5d GY:%5d", accel[1], gyro[1]);
        snprintf(line7, sizeof(line7), "AZ:%5d GZ:%5d", accel[2], gyro[2]);
        oled_clear();
        oled_draw_string(0, 0, line1);
        oled_draw_string(1, 0, line2);
        oled_draw_string(2, 0, line3);
        oled_draw_string(3, 0, line4);
        oled_draw_string(4, 0, line5);
        oled_draw_string(5, 0, line6);
        oled_draw_string(6, 0, line7);
        oled_draw_string(7, 0, line8);

        vTaskDelay(pdMS_TO_TICKS(50));
    }

}