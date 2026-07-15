// components/bsp/src/adc_sensor.c

#include "adc_sensor.h"
#include "driver/adc.h"
#include "esp_log.h"
#include <math.h>

// 引脚定义（与硬件一致）
#define LIGHT_SENSOR_GPIO    2    // 光敏 → GPIO2 → ADC1_CH2
#define TEMP_SENSOR_GPIO     6    // 热敏 → GPIO6 → ADC1_CH6

// 电路参数
#define VCC_VOLTAGE         3300  // mV
#define DIVIDER_RESISTOR    10000 // Ω

#define NTC_R25             10000  
#define NTC_BETA            4050

static const char *TAG = "bsp_adc";

esp_err_t bsp_adc_sensor_init(void)
{
    // 配置 ADC1 为 12-bit
    adc1_config_width(ADC_WIDTH_BIT_12);

    // 配置通道衰减（0~3.3V 范围）
    adc1_config_channel_atten(ADC1_CHANNEL_2, ADC_ATTEN_DB_11); // GPIO2
    adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11); // GPIO6

    ESP_LOGI(TAG, "ADC sensor initialized (GPIO2=light, GPIO6=temp)");
    return ESP_OK;
}

static uint32_t calculate_lux(uint32_t adc_value)
{
    if (adc_value >= 4095) adc_value = 4094; // 防止除零
    float voltage = (float)adc_value * 3.3f / 4095.0f;
    float ldr_r = voltage / (3.3f - voltage) * 10000.0f;
    return (uint32_t)(40000.0f * powf(ldr_r, -0.6021f));
}

static void calculate_temperature_from_adc(uint32_t adc_value, float *temp_c)
{
    if (adc_value >= 4095) adc_value = 4094;
    if (adc_value == 0) {
        *temp_c = 0.0f;
        return;
    }

    // 计算 ADC 电压（单位：V）
    const float vcc = 3.3f;
    float v_adc = vcc * (float)adc_value / 4095.0f;

    // NTC 接 GND，固定电阻 10k 接 VCC
    float r_ntc = 10000.0f * v_adc / (vcc - v_adc);  // R_ntc = R_fixed * V_adc / (Vcc - V_adc)

    // Steinhart-Hart 方程
    float ln_r = logf(r_ntc / NTC_R25);
    float inv_t = (1.0f / 298.15f) + (ln_r / NTC_BETA);
    float t_kelvin = 1.0f / inv_t;
    *temp_c = t_kelvin - 273.15f;
}

//获取带一位小数的温度字符串 ---
void bsp_adc_get_temperature_str(char *buf, size_t len)
{
    int raw = adc1_get_raw(ADC1_CHANNEL_6);
    if (raw < 0) {
        snprintf(buf, len, "Err");
        return;
    }

    float temp_c;
    calculate_temperature_from_adc((uint32_t)raw, &temp_c);

    // 限制合理范围（可选）
    if (temp_c < -10.0f) temp_c = -10.0f;
    if (temp_c > 60.0f) temp_c = 60.0f;

    // 格式化为 "25.3"（一位小数）
    snprintf(buf, len, "%.1f", temp_c);
}

uint32_t bsp_adc_get_light_lux(void)
{
    int raw = adc1_get_raw(ADC1_CHANNEL_2);
    if (raw < 0) {
        ESP_LOGE(TAG, "Failed to read light ADC");
        return 0;
    }
    return calculate_lux((uint32_t)raw);
}
