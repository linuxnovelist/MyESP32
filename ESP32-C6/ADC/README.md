# Try6_ADC — ESP32-C6 ADC 传感器采集与 OLED 显示

基于 ESP-IDF 的 ESP32-C6 综合实验项目，通过 ADC 采集光敏传感器与热敏传感器数据，在 SSD1309 OLED 屏幕上实时显示，同时支持按键中断计数与 LED 控制。

## 功能概览

| 功能 | 说明 |
|------|------|
| 🌞 **光照采集** | 光敏电阻经 ADC1_CH2 (GPIO2) 采样，换算为 Lux 值 |
| 🌡️ **温度采集** | NTC 热敏电阻经 ADC1_CH6 (GPIO6) 采样，Steinhart-Hart 方程换算温度 |
| 📺 **OLED 显示** | SSD1309 128×64 SPI 屏幕，6 行信息实时刷新 |
| 🔘 **按键中断** | 两路按键 (GPIO0/GPIO1) 下降沿触发，中断内计数并控制 LED |
| 💡 **LED 指示** | GPIO3 低电平点亮，按键时切换状态 |

## 硬件连接

### OLED (SSD1309, SPI)

| 信号 | GPIO |
|------|------|
| MOSI | 21 |
| SCLK | 20 |
| CS   | 15 |
| DC   | 23 |
| RST  | 22 |

### 传感器 (ADC1)

| 传感器 | GPIO | ADC 通道 |
|--------|------|----------|
| 光敏电阻 | 2 | ADC1_CH2 |
| NTC 热敏 | 6 | ADC1_CH6 |

> 电路结构：传感器与 10kΩ 固定电阻组成分压电路，VCC = 3.3V。

### 按键 & LED

| 外设 | GPIO | 说明 |
|------|------|------|
| KEY1 | 0 | 下降沿中断，计数 + LED 开 |
| KEY2 | 1 | 下降沿中断，计数 + LED 关 |
| LED  | 3 | 低电平点亮 |

## 软件架构

```
Try6_ADC/
├── main/
│   ├── CMakeLists.txt          # 主程序编译配置
│   └── hello_world_main.c      # 主逻辑：初始化、中断、显示循环
├── components/
│   └── BSP/
│       ├── CMakeLists.txt      # BSP 组件配置
│       ├── LED/                # LED 驱动 (GPIO3)
│       ├── KEY/                # 按键驱动 (GPIO0/GPIO1)
│       ├── ADC/                # ADC 传感器驱动 (光敏 + 热敏)
│       └── OLED_SPI(M242)/     # SSD1309 OLED 驱动 (SPI, 6x8 字体)
├── CMakeLists.txt              # 项目顶层 CMake
├── sdkconfig                   # ESP-IDF 配置
└── README.md
```

### 显示内容

OLED 屏幕以 20Hz 刷新率循环显示 6 行信息：

```
ESP32-C6
LED:ON / LED:OFF
Light:12345
Temp:25.3 C
K1:3 K2:1
Sensors OK
```

- 光照和温度数据每 500ms 更新一次（每 10 帧）
- 按键计数器在中断服务函数中累加

### 温度计算

使用 Steinhart-Hart 方程：

$$\frac{1}{T} = \frac{1}{T_0} + \frac{\ln(R/R_0)}{B}$$

其中 $R_0 = 10k\Omega$ (@25°C)，$B = 4050$。

### 光照计算

通过光敏电阻分压公式反推电阻值，再经经验公式换算为 Lux：

$$Lux \approx 40000 \times R_{LDR}^{-0.6021}$$

## 构建与烧录

### 环境要求

- [ESP-IDF](https://github.com/espressif/esp-idf) v5.0 或更高版本
- ESP32-C6 开发板
- SSD1309 128×64 OLED 模块
- 光敏电阻 + NTC 热敏电阻模块

### 编译

```bash
idf.py set-target esp32-c6
idf.py build
```

### 烧录与监视

```bash
idf.py -p <串口号> flash monitor
```

## 关键技术点

- **ADC 采样**：12-bit 精度，0~3.3V 范围，配合 `ADC_ATTEN_DB_11` 衰减
- **SPI 通信**：8MHz 时钟，手动 CS 控制，兼容 SSD1306 指令集
- **GPIO 中断**：下降沿触发，`IRAM_ATTR` 修饰 ISR，FreeRTOS 中断服务
- **FreeRTOS**：`vTaskDelay` 控制主循环节奏，`portTICK_PERIOD_MS` 去抖动

## 目标芯片

ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C5 | **ESP32-C6** | ESP32-C61 | ESP32-H2 | ESP32-P4 | ESP32-S2 | ESP32-S3
