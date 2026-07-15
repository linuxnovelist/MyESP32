// components/bsp/include/bsp_i2c_sensors.h

#ifndef __IIC_SENSOR_H__
#define __IIC_SENSOR_H__

#include <stdint.h>
#include "esp_err.h"

#define I2C_MASTER_SCL_IO    11
#define I2C_MASTER_SDA_IO    10
#define I2C_MASTER_FREQ_HZ   400000
#define I2C_MASTER_PORT      I2C_NUM_0

#define SHT30_ADDR          0x44
#define AP3216C_ADDR        0x1E

#define AP3216C_SYS_CFG     0x00
#define AP3216C_ALS_DATA_L  0x0C
#define AP3216C_PS_DATA_L   0x0E

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化 I2C 总线和所有传感器（SHT30 + AP3216C）
 * @return ESP_OK on success
 */
esp_err_t bsp_i2c_sensors_init(void);

/**
 * @brief 读取 SHT30 温湿度数据
 * @param[out] temperature 温度（单位：0.1°C，如 253 = 25.3°C）
 * @param[out] humidity    湿度（单位：0.1%RH，如 456 = 45.6%）
 * @return ESP_OK on success
 */
esp_err_t bsp_sht30_read(int16_t *temperature, uint16_t *humidity);

/**
 * @brief 读取 AP3216C 光强和接近数据
 * @param[out] als 光强原始值（需转换为 lux）
 * @param[out] ps  接近值（0~255，越大越近）
 * @return ESP_OK on success
 */
esp_err_t bsp_ap3216c_read(uint16_t *als, uint16_t *ps);

/**
 * @brief 将 ALS 原始值转换为 Lux（参考 RT-Thread 实现）
 * @param als 原始值（12-bit）
 * @return 光照强度（单位：lux）
 */
uint32_t bsp_ap3216c_als_to_lux(uint16_t als);

#ifdef __cplusplus
}
#endif

// ==================== ICM-20608-G ====================
#define ICM20608_ADDR         0x68    // AD0 = GND
#define ICM20608_WHO_AM_I     0x75
#define ICM20608_PWR_MGMT_1   0x6B
#define ICM20608_ACCEL_CONFIG 0x1C
#define ICM20608_GYRO_CONFIG  0x1B
#define ICM20608_ACCEL_XOUT_H 0x3B

/**
 * @brief 初始化 ICM-20608-G 传感器
 * @return ESP_OK on success
 */
esp_err_t bsp_icm20608_init(void);

/**
 * @brief 读取 ICM-20608-G 加速度计和陀螺仪原始数据
 * @param[out] accel 3-element array for X,Y,Z (int16_t)
 * @param[out] gyro  3-element array for X,Y,Z (int16_t)
 * @return ESP_OK on success
 */
esp_err_t bsp_icm20608_read(int16_t accel[3], int16_t gyro[3]);

#endif // __BSP_I2C_SENSORS_H__