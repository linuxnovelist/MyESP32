// components/bsp/include/adc_sensor.h

#ifndef __ADC_SENSOR_H__
#define __ADC_SENSOR_H__

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化 ADC 传感器（光敏 + 热敏）
 * @return ESP_OK on success
 */
esp_err_t bsp_adc_sensor_init(void);

/**
 * @brief 读取光照强度（Lux）
 * @return 光照强度值（单位：Lux），0 表示无效
 */
uint32_t bsp_adc_get_light_lux(void);

/**
 * @brief 读取环境温度
 * @param[out] temp_int 温度整数部分（如 25）
 * @param[out] temp_frac 温度小数部分（如 30 表示 0.30°C）
 * @return ESP_OK on success
 */
esp_err_t bsp_adc_get_temperature(int *temp_int, int *temp_frac);
/**
 * @brief 获取格式化的温度字符串（如 "25.3"）
 * @param buf 输出缓冲区
 * @param len 缓冲区长度（建议 ≥ 8）
 */
void bsp_adc_get_temperature_str(char *buf, size_t len);
#ifdef __cplusplus
}
#endif

#endif // __BSP_ADC_SENSOR_H__