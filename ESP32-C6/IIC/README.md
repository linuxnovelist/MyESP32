# Try7_IIC — ESP32-C6 多传感器数据采集与 OLED 显示

基于 ESP-IDF 的 ESP32-C6 项目，通过 I2C 总线采集温度、湿度、光照强度、接近感应及 6 轴姿态数据，并在 SPI OLED 屏幕上实时显示。

## 硬件连接

### SPI OLED (SSD1309, 128×64)

| 引脚 | GPIO |
|------|------|
| MOSI | 21   |
| SCLK | 20   |
| CS   | 15   |
| DC   | 23   |
| RST  | 22   |

### I2C 传感器总线 (I2C_NUM_0, 400kHz)

| 引脚 | GPIO |
|------|------|
| SDA  | 10   |
| SCL  | 11   |

### 其他外设

| 外设 | GPIO | 说明 |
|------|------|------|
| LED  | 3    | 低电平点亮 |
| KEY1 | 0    | 按键 1（下降沿中断） |
| KEY2 | 1    | 按键 2（下降沿中断） |

## 挂载的 I2C 传感器

| 型号         | 地址  | 功能                             |
|-------------|------|----------------------------------|
| SHT30       | 0x44 | 温度 (±0.3°C) + 湿度 (±2%RH)      |
| AP3216C     | 0x1E | 环境光强度 (ALS) + 接近感应 (PS)   |
| ICM-20608-G | 0x68 | 6 轴 IMU（3 轴加速度 + 3 轴陀螺仪） |

## 项目结构

```
Try7_IIC/
├── CMakeLists.txt                          # 项目根 CMake 配置
├── main/
│   ├── CMakeLists.txt                      # main 组件配置
│   └── hello_world_main.c                  # 主程序：初始化 + 主循环 + OLED 显示
├── components/
│   └── BSP/
│       ├── CMakeLists.txt                  # BSP 组件配置（聚合所有子模块）
│       ├── LED/
│       │   ├── led.c                       # LED 初始化（GPIO3, 低电平点亮）
│       │   └── led.h                       # LED 宏定义与控制接口
│       ├── KEY/
│       │   ├── key.c                       # 按键初始化 + 扫描（消抖/长按检测）
│       │   └── key.h                       # 按键引脚定义与接口
│       ├── OLED_SPI(M242)/
│       │   ├── oled.c                      # SSD1309 OLED 驱动（SPI, 6×8 字体）
│       │   └── oled.h                      # OLED 引脚定义与绘图接口
│       └── IIC/
│           ├── iic_sensor.c                # I2C 传感器驱动（SHT30/AP3216C/ICM20608）
│           └── iic_sensor.h                # 传感器地址、寄存器定义与 API
├── .devcontainer/
│   ├── devcontainer.json                   # VS Code Dev Container 配置
│   └── Dockerfile                          # ESP-IDF 开发容器镜像
├── .vscode/
│   ├── launch.json                         # 调试配置
│   ├── settings.json                       # 编辑器设置
│   └── c_cpp_properties.json               # C/C++ IntelliSense 配置
└── .gitignore
```

## 功能说明

### 主程序

`main/hello_world_main.c` 的工作流程：

1. 打印芯片信息（型号、核心数、WiFi/BT/BLE、Flash 大小等）
2. 初始化所有外设：LED → KEY → OLED → I2C 传感器（SHT30 + AP3216C + ICM-20608）
3. 配置按键中断（下降沿触发），KEY1 切换到关灯状态，KEY2 切换到开灯状态
4. 主循环：每 2 秒刷新一次传感器数据，在 OLED 上显示：

| 行 | 内容 |
|----|------|
| 1  | LED 状态 (ON/OFF) |
| 2  | 温度 (°C) + 湿度 (%RH) |
| 3  | 光照强度 (lux) |
| 4  | 接近值 (PS) |
| 5  | 加速度 X + 陀螺仪 X |
| 6  | 加速度 Y + 陀螺仪 Y |
| 7  | 加速度 Z + 陀螺仪 Z |

显示刷新率为 50ms/帧（20 fps），传感器数据每 40 帧（≈2 秒）更新一次。

### 驱动组件

#### LED (`components/BSP/LED/`)
- 宏封装：`LED(0)` 点亮、`LED(1)` 熄灭、`LED_TOGGLE()` 翻转
- 上电默认关闭（高电平）

#### KEY (`components/BSP/KEY/`)
- `key_init()` — 初始化 GPIO0/GPIO1 为上拉输入
- `key_scan(mode)` — 扫描式按键检测，mode=0 不支持连按，mode=1 支持
- `check_key1_long_press()` — 阻塞式长按检测（>2 秒）
- 主程序使用中断方式处理按键（下降沿触发 ISR）

#### OLED (`components/BSP/OLED_SPI(M242)/`)
- 基于 SSD1309（兼容 SSD1306 指令集）
- SPI 主机模式，8MHz 时钟，手动 CS 控制
- 内置 6×8 ASCII 字体（空格 ~ 可打印字符）
- 支持字符串绘制、清屏、开关显示

#### I2C 传感器 (`components/BSP/IIC/`)
- 使用 ESP-IDF 新版 `i2c_master` 驱动
- SHT30：CRC8 校验，输出温度 (×10) 和湿度 (×10)
- AP3216C：12-bit ALS + 10-bit PS，附 lux 换算函数
- ICM-20608-G：唤醒 → 配置加速度计 (±16g) / 陀螺仪 (±2000dps) → 读取 14 字节数据

## 开发环境

- **芯片**: ESP32-C6
- **框架**: ESP-IDF (v5.x+)
- **工具链**: riscv32-esp-elf
- **串口**: 默认 115200 baud

### 使用 Dev Container（推荐）

项目包含 `.devcontainer` 配置，可直接在 VS Code 中打开为容器开发环境：

1. 安装 [Dev Containers 扩展](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers)
2. VS Code → `F1` → "Dev Containers: Reopen in Container"

### 本地开发

```bash
# 设置目标芯片
idf.py set-target esp32c6

# 配置项目（可选）
idf.py menuconfig

# 编译
idf.py build

# 烧录并打开串口监视器
idf.py flash monitor
```

## 许可证

部分源文件遵循 [CC0-1.0](https://spdx.org/licenses/CC0-1.0.html) 或 ESP-IDF 默认许可。详情参见各文件头部的 SPDX 标识。
