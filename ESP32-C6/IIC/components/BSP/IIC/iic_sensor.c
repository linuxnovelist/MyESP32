// components/bsp/src/bsp_i2c_sensors.c （更新版）

#include "iic_sensor.h"
#include "driver/i2c_master.h"  // ← 新驱动头文件
#include "esp_log.h"
#include "math.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


static const char *TAG = "i2c_sensors";
// 全局 I2C 主机句柄
static i2c_master_bus_handle_t i2c_bus = NULL;
// Helper: write one byte to ICM20608 register
static esp_err_t icm20608_write_reg(uint8_t reg, uint8_t val)
{
    i2c_master_dev_handle_t dev;
    i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = ICM20608_ADDR,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    esp_err_t ret = i2c_master_bus_add_device(i2c_bus, &cfg, &dev);
    if (ret != ESP_OK) return ret;

    uint8_t buf[2] = {reg, val};
    ret = i2c_master_transmit(dev, buf, 2, -1);
    i2c_master_bus_rm_device(dev);
    return ret;
}

// Helper: read multiple bytes from ICM20608
static esp_err_t icm20608_read_regs(uint8_t reg, uint8_t *buf, size_t len)
{
    i2c_master_dev_handle_t dev;
    i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = ICM20608_ADDR,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    esp_err_t ret = i2c_master_bus_add_device(i2c_bus, &cfg, &dev);
    if (ret != ESP_OK) return ret;

    ret = i2c_master_transmit_receive(dev, &reg, 1, buf, len, -1);
    i2c_master_bus_rm_device(dev);
    return ret;
}

// CRC8 函数（不变）
static uint8_t sht30_crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0xFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int bit = 8; bit > 0; bit--) {
            crc = (crc & 0x80) ? (crc << 1) ^ 0x31 : (crc << 1);
        }
    }
    return crc;
}

// ==================== SHT30 ====================
esp_err_t bsp_sht30_read(int16_t *temperature, uint16_t *humidity)
{
    if (!temperature || !humidity) return ESP_ERR_INVALID_ARG;

    // 创建设备句柄
    i2c_master_dev_handle_t dev_handle;
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = SHT30_ADDR,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    esp_err_t ret = i2c_master_bus_add_device(i2c_bus, &dev_cfg, &dev_handle);
    if (ret != ESP_OK) return ret;

    uint8_t cmd[2] = {0x2C, 0x06};
    uint8_t data[6];

    // 发送命令
    ret = i2c_master_transmit(dev_handle, cmd, 2, -1);
    if (ret != ESP_OK) goto cleanup;

    vTaskDelay(pdMS_TO_TICKS(20));

    // 读取数据
    ret = i2c_master_receive(dev_handle, data, 6, -1);
    if (ret != ESP_OK) goto cleanup;

    // CRC 校验
    if (sht30_crc8(&data[0], 2) != data[2] ||
        sht30_crc8(&data[3], 2) != data[5]) {
        ret = ESP_ERR_INVALID_CRC;
        goto cleanup;
    }

    uint16_t raw_temp = (data[0] << 8) | data[1];
    uint16_t raw_humi = (data[3] << 8) | data[4];

    *temperature = (int16_t)((int32_t)raw_temp * 1750 / 65535 - 450);
    *humidity    = (uint16_t)((uint32_t)raw_humi * 1000 / 65535);

cleanup:
    i2c_master_bus_rm_device(dev_handle);
    return ret;
}

// ==================== AP3216C ====================
esp_err_t bsp_ap3216c_read(uint16_t *als, uint16_t *ps)
{
    if (!als || !ps) return ESP_ERR_INVALID_ARG;

    i2c_master_dev_handle_t dev_handle;
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = AP3216C_ADDR,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    esp_err_t ret = i2c_master_bus_add_device(i2c_bus, &dev_cfg, &dev_handle);
    if (ret != ESP_OK) return ret;

    uint8_t buf[4];

    // 读 ALS
    ret = i2c_master_transmit_receive(dev_handle, 
                                      (uint8_t[]){AP3216C_ALS_DATA_L}, 1, 
                                      buf, 2, -1);
    if (ret != ESP_OK) goto cleanup;
    *als = (buf[1] << 4) | (buf[0] & 0x0F); // 12-bit: [H7:H4][L7:L0]

    // 读 PS
    ret = i2c_master_transmit_receive(dev_handle,
                                      (uint8_t[]){AP3216C_PS_DATA_L}, 1,
                                      &buf[2], 2, -1);
    if (ret != ESP_OK) goto cleanup;
    *ps = (buf[3] << 8) | buf[2];

cleanup:
    i2c_master_bus_rm_device(dev_handle);
    return ret;
}

uint32_t bsp_ap3216c_als_to_lux(uint16_t als)
{
    return (als * 35 + 50) / 100;
}

// ==================== 初始化 ====================
esp_err_t bsp_i2c_sensors_init(void)
{
    // 配置 I2C 总线
    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_MASTER_PORT,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = 1,
    };
    esp_err_t err = i2c_new_master_bus(&i2c_mst_config, &i2c_bus);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C bus create failed");
        return err;
    }
    // 初始化 AP3216C
    i2c_master_dev_handle_t ap3216c_dev;
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = AP3216C_ADDR,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    err = i2c_master_bus_add_device(i2c_bus, &dev_cfg, &ap3216c_dev);
    if (err == ESP_OK) {
        // 正确的写寄存器函数：发送 [reg_addr, data]
        uint8_t write_buf[2];
        // 1. 软件复位: REG=0x00, VAL=0x04
        write_buf[0] = AP3216C_SYS_CFG; // 0x00
        write_buf[1] = 0x04;            // SW Reset
        err = i2c_master_transmit(ap3216c_dev, write_buf, 2, -1);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "AP3216C reset failed");
            i2c_master_bus_rm_device(ap3216c_dev);
            goto skip_ap3216c;
        }
        vTaskDelay(pdMS_TO_TICKS(10));

        // 2. 配置模式: REG=0x00, VAL=0x03 (ALS+PS continuous)
        write_buf[0] = AP3216C_SYS_CFG;
        write_buf[1] = 0x03;
        err = i2c_master_transmit(ap3216c_dev, write_buf, 2, -1);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "AP3216C config failed");
            i2c_master_bus_rm_device(ap3216c_dev);
            goto skip_ap3216c;
        }
        ESP_LOGI(TAG, "AP3216C initialized");
        i2c_master_bus_rm_device(ap3216c_dev);
    } else {
        ESP_LOGE(TAG, "Failed to add AP3216C device (addr=0x%02X)", AP3216C_ADDR);
    }
    skip_ap3216c:

    return ESP_OK;
}

esp_err_t bsp_icm20608_init(void)
{
    uint8_t who_am_i;
    esp_err_t ret;

    ret = icm20608_read_regs(ICM20608_WHO_AM_I, &who_am_i, 1);
    if (ret != ESP_OK || who_am_i != 0xAF) {
        ESP_LOGE(TAG, "ICM-20608 init failed, WHO_AM_I=0x%02X", who_am_i);
        return ESP_ERR_NOT_FOUND;
    }

    // Wake up and set clock source
    ret = icm20608_write_reg(ICM20608_PWR_MGMT_1, 0x01);
    if (ret != ESP_OK) return ret;
    vTaskDelay(pdMS_TO_TICKS(10));

    // Accel: ±16g
    ret = icm20608_write_reg(ICM20608_ACCEL_CONFIG, 0x18);
    if (ret != ESP_OK) return ret;

    // Gyro: ±2000dps
    ret = icm20608_write_reg(ICM20608_GYRO_CONFIG, 0x18);
    if (ret != ESP_OK) return ret;

    ESP_LOGI(TAG, "ICM-20608-G initialized");
    return ESP_OK;
}

esp_err_t bsp_icm20608_read(int16_t accel[3], int16_t gyro[3])
{
    if (!accel || !gyro) return ESP_ERR_INVALID_ARG;

    uint8_t buf[14];
    esp_err_t ret = icm20608_read_regs(ICM20608_ACCEL_XOUT_H, buf, 14);
    if (ret != ESP_OK) return ret;

    accel[0] = (int16_t)((buf[0] << 8) | buf[1]);  // X
    accel[1] = (int16_t)((buf[2] << 8) | buf[3]);  // Y
    accel[2] = (int16_t)((buf[4] << 8) | buf[5]);  // Z

    gyro[0]  = (int16_t)((buf[8] << 8) | buf[9]);   // X
    gyro[1]  = (int16_t)((buf[10] << 8) | buf[11]); // Y
    gyro[2]  = (int16_t)((buf[12] << 8) | buf[13]); // Z

    return ESP_OK;
}