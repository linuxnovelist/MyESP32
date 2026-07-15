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
#include "adc_sensor.h"// 新增传感器驱动

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
    // === 新增：初始化 ADC 传感器 ===
    bsp_adc_sensor_init();

    // 安装中断
    gpio_install_isr_service(0);
    gpio_set_intr_type(KEY1_GPIO_PIN, GPIO_INTR_NEGEDGE);
    gpio_isr_handler_add(KEY1_GPIO_PIN, key1_isr_handler, NULL);
    gpio_set_intr_type(KEY2_GPIO_PIN, GPIO_INTR_NEGEDGE);
    gpio_isr_handler_add(KEY2_GPIO_PIN, key2_isr_handler, NULL);

    printf("例程五：OLED 显示 ADC 传感器 + 按键\n");

    char line1[20], line2[20], line3[20], line4[20], line5[20];
    uint32_t light_lux = 0;
    uint32_t update_counter = 0;
    char temp_str[8];

    while (1)
    {
        // 每 10 次循环（500ms）更新传感器数据
        if (update_counter++ % 10 == 0) {
            light_lux = bsp_adc_get_light_lux();
            bsp_adc_get_temperature_str(temp_str, sizeof(temp_str));
        }

        const char* led_state = gpio_get_level(LED_GPIO_PIN) ? "OFF" : "ON";

        snprintf(line1, sizeof(line1), "ESP32-C6");
        snprintf(line2, sizeof(line2), "LED:%s", led_state);
        snprintf(line3, sizeof(line3), "Light:%lu", (unsigned long)light_lux);
        snprintf(line4, sizeof(line4), "Temp:%s C", temp_str);
        snprintf(line5, sizeof(line5), "K1:%lu K2:%lu", 
                 (unsigned long)key1_count, (unsigned long)key2_count);

        oled_clear();
        oled_draw_string(0, 0, line1);
        oled_draw_string(1, 0, line2);
        oled_draw_string(2, 0, line3);
        oled_draw_string(3, 0, line4);
        oled_draw_string(4, 0, line5);
        oled_draw_string(5, 0, "Sensors OK");

        vTaskDelay(pdMS_TO_TICKS(50)); // 20Hz
    }
}