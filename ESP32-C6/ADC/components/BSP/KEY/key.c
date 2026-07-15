#include "key.h"
#include <portmacro.h>
/**
 * @brief       初始化按键
 * @param       无
 * @retval      无
 */
void key_init(void)
{
    gpio_config_t gpio_init_struct;

    gpio_init_struct.intr_type = GPIO_INTR_DISABLE;         /* 失能引脚中断 */
    gpio_init_struct.mode = GPIO_MODE_INPUT;                /* 输入模式 */
    gpio_init_struct.pull_up_en = GPIO_PULLUP_ENABLE;       /* 使能上拉（确保未按下时为高）*/
    gpio_init_struct.pull_down_en = GPIO_PULLDOWN_DISABLE;  /* 失能下拉 */
    
    // 配置 KEY1 和 KEY2 引脚
    gpio_init_struct.pin_bit_mask = (1ULL << KEY1_GPIO_PIN) | (1ULL << KEY2_GPIO_PIN);
    gpio_config(&gpio_init_struct);                         /* 配置使能 */
}

/**
 * @brief       按键扫描函数
 * @param       mode: 0 / 1, 具体含义如下:
 *              0, 不支持连续按（只有松开再按下才响应）
 *              1, 支持连续按（按住时每次调用都返回）
 * @retval      键值, 定义如下:
 *              0,           无按键按下
 *              KEY1_PRES,   KEY1 按下（值为1）
 *              KEY2_PRES,   KEY2 按下（值为1，但可通过不同返回值区分）
 *
 * 注意：由于两个按键都定义为 _PRES = 1，直接返回1无法区分。
 *       因此建议修改 key.h 中的定义，或在此处使用不同返回值。
 *       这里我们采用：KEY1 返回 1，KEY2 返回 2，以便区分。
 */
uint8_t key_scan(uint8_t mode)
{
    static uint8_t key1_state = 1;  // 松开标志（1=松开，0=已按下）
    static uint8_t key2_state = 1;

    uint8_t key_val = 0;

    if (mode)
    {
        // 支持连续按：每次调用都重置状态，相当于立即可再次触发
        key1_state = 1;
        key2_state = 1;
    }

    // 扫描 KEY1
    if (key1_state && (KEY1 == 0))
    {
        vTaskDelay(10 / portTICK_PERIOD_MS);  // 去抖动（FreeRTOS 延时单位需注意）
        if (KEY1 == 0)
        {
            key_val = 1;  // KEY1 按下
            key1_state = 0;
        }
    }
    else if (KEY1 == 1)
    {
        key1_state = 1;  // 松开了
    }

    // 扫描 KEY2（优先级低于 KEY1，若同时按下只返回 KEY1；如需同时处理需改逻辑）
    if (key_val == 0)  // 仅在 KEY1 未按下时检测 KEY2
    {
        if (key2_state && (KEY2 == 0))
        {
            vTaskDelay(10 / portTICK_PERIOD_MS);
            if (KEY2 == 0)
            {
                key_val = 2;  // KEY2 按下
                key2_state = 0;
            }
        }
        else if (KEY2 == 1)
        {
            key2_state = 1;
        }
    }

    return key_val;
}

// 检测 KEY1 长按（>2秒）用于开关功能
bool check_key1_long_press(void)
{
    if (KEY1 == 0) // 按下
    {
        uint32_t press_time = 0;
        while (KEY1 == 0 && press_time < 2000)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
            press_time += 10;
        }
        if (press_time >= 2000)
        {
            // 等待松开，避免重复触发
            while (KEY1 == 0) {
                vTaskDelay(pdMS_TO_TICKS(10));
            }
            return true;
        }
        else
        {
            // 短按，忽略（或可做其他用途）
            while (KEY1 == 0) {
                vTaskDelay(pdMS_TO_TICKS(10)); // 等待松开
            }
        }
    }
    return false;
}