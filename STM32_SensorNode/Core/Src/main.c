/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */

#include "main.h"
#include "adc.h"
#include "spi.h"
#include "tim.h"
#include "gpio.h"
#include "mcp2515.h"

/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
#include <stdint.h>
/* USER CODE END Includes */

/* USER CODE BEGIN PV */
uint8_t dht11_temp = 0;
uint8_t dht11_hum = 0;
uint8_t dht11_valid = 0;

uint8_t seat_occupied = 1;
uint16_t fan_pwm = 0;
uint8_t ac_mode = 0;
uint16_t manual_fan_pwm = 0;

uint8_t manual_mode = 0;
uint32_t manual_last_action = 0;
uint32_t btn_last_tick[3] = {0, 0, 0};

#define BTN_DEBOUNCE_MS 200
#define MANUAL_TIMEOUT_MS 10000

char lcd_line1[17];
char lcd_line2[17];
/* USER CODE END PV */

void SystemClock_Config(void);

/* USER CODE BEGIN 0 */
void delay_us(uint16_t us)
{
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    while (__HAL_TIM_GET_COUNTER(&htim2) < us);
}

void DHT11_SetOutput(void)
{
    GPIO_InitTypeDef g = {0};
    g.Pin = DHT11_PIN_Pin;
    g.Mode = GPIO_MODE_OUTPUT_PP;
    g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT11_PIN_GPIO_Port, &g);
}

void DHT11_SetInput(void)
{
    GPIO_InitTypeDef g = {0};
    g.Pin = DHT11_PIN_Pin;
    g.Mode = GPIO_MODE_INPUT;
    g.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(DHT11_PIN_GPIO_Port, &g);
}

uint8_t DHT11_Read(uint8_t *temp, uint8_t *hum)
{
    uint8_t data[5] = {0, 0, 0, 0, 0};
    uint32_t timeout;

    DHT11_SetOutput();
    HAL_GPIO_WritePin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin, GPIO_PIN_RESET);
    HAL_Delay(18);
    HAL_GPIO_WritePin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin, GPIO_PIN_SET);
    delay_us(30);
    DHT11_SetInput();

    timeout = 0;
    while (HAL_GPIO_ReadPin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin) == GPIO_PIN_SET)
        if (++timeout > 10000) return 0;

    timeout = 0;
    while (HAL_GPIO_ReadPin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin) == GPIO_PIN_RESET)
        if (++timeout > 10000) return 0;

    timeout = 0;
    while (HAL_GPIO_ReadPin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin) == GPIO_PIN_SET)
        if (++timeout > 10000) return 0;

    for (int byte = 0; byte < 5; byte++) {
        for (int bit = 7; bit >= 0; bit--) {
            timeout = 0;
            while (HAL_GPIO_ReadPin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin) == GPIO_PIN_RESET)
                if (++timeout > 10000) return 0;

            delay_us(40);

            if (HAL_GPIO_ReadPin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin) == GPIO_PIN_SET) {
                data[byte] |= (1 << bit);

                timeout = 0;
                while (HAL_GPIO_ReadPin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin) == GPIO_PIN_SET)
                    if (++timeout > 10000) return 0;
            }
        }
    }

    if (data[4] != (uint8_t)(data[0] + data[1] + data[2] + data[3]))
        return 0;

    *hum = data[0];
    *temp = data[2];
    return 1;
}

void LCD_PulseEnable(void)
{
    HAL_GPIO_WritePin(LCD_EN_GPIO_Port, LCD_EN_Pin, GPIO_PIN_SET);
    delay_us(1);
    HAL_GPIO_WritePin(LCD_EN_GPIO_Port, LCD_EN_Pin, GPIO_PIN_RESET);
    delay_us(50);
}

void LCD_WriteNibble(uint8_t nibble)
{
    HAL_GPIO_WritePin(LCD_D4_GPIO_Port, LCD_D4_Pin, (nibble >> 0) & 1);
    HAL_GPIO_WritePin(LCD_D5_GPIO_Port, LCD_D5_Pin, (nibble >> 1) & 1);
    HAL_GPIO_WritePin(LCD_D6_GPIO_Port, LCD_D6_Pin, (nibble >> 2) & 1);
    HAL_GPIO_WritePin(LCD_D7_GPIO_Port, LCD_D7_Pin, (nibble >> 3) & 1);
    LCD_PulseEnable();
}

void LCD_SendByte(uint8_t data, uint8_t is_data)
{
    HAL_GPIO_WritePin(LCD_RS_GPIO_Port, LCD_RS_Pin, is_data);
    LCD_WriteNibble(data >> 4);
    LCD_WriteNibble(data & 0x0F);
}

void LCD_Init(void)
{
    HAL_Delay(50);

    HAL_GPIO_WritePin(LCD_RS_GPIO_Port, LCD_RS_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LCD_EN_GPIO_Port, LCD_EN_Pin, GPIO_PIN_RESET);

    LCD_WriteNibble(0x03); HAL_Delay(5);
    LCD_WriteNibble(0x03); delay_us(150);
    LCD_WriteNibble(0x03); delay_us(150);
    LCD_WriteNibble(0x02);

    LCD_SendByte(0x28, 0);
    LCD_SendByte(0x0C, 0);
    LCD_SendByte(0x06, 0);
    LCD_SendByte(0x01, 0);
    HAL_Delay(2);
}

void LCD_SetCursor(uint8_t col, uint8_t row)
{
    uint8_t addr = (row == 0) ? 0x00 : 0x40;
    LCD_SendByte(0x80 | (addr + col), 0);
}

void LCD_Print(char *str)
{
    while (*str) LCD_SendByte((uint8_t)*str++, 1);
}

void LCD_Clear(void)
{
    LCD_SendByte(0x01, 0);
    HAL_Delay(2);
}
/* USER CODE END 0 */

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_ADC1_Init();
    MX_SPI1_Init();

    HAL_GPIO_WritePin(MCP2515_CS_GPIO_Port, MCP2515_CS_Pin, GPIO_PIN_SET);
    HAL_Delay(100);

    MX_TIM1_Init();
    MX_TIM2_Init();

    HAL_TIM_Base_Start(&htim2);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);

    HAL_GPIO_WritePin(FAN_DIR1_GPIO_Port, FAN_DIR1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(FAN_DIR2_GPIO_Port, FAN_DIR2_Pin, GPIO_PIN_RESET);

    LCD_Init();

    LCD_SetCursor(0, 0);
    LCD_Print("Smart Climate   ");
    LCD_SetCursor(0, 1);
    LCD_Print("Starting...     ");
    HAL_Delay(1500);
    LCD_Clear();

    MCP2515_Init();

    while (1)
    {
        static uint32_t dht_last = 0;
        if (HAL_GetTick() - dht_last > 2000) {
            dht11_valid = DHT11_Read(&dht11_temp, &dht11_hum);
            dht_last = HAL_GetTick();
        }

        seat_occupied = (HAL_GPIO_ReadPin(IR_SENSOR_GPIO_Port, IR_SENSOR_Pin) == GPIO_PIN_RESET) ? 1 : 0;

        float temp = (dht11_valid && dht11_temp > 0) ? (float)dht11_temp : 25.0f;
        uint8_t hum = dht11_valid ? dht11_hum : 50;

        uint16_t auto_fan_pwm = 0;
        uint8_t auto_ac_mode = 0;

        if (!seat_occupied) {
            auto_fan_pwm = 0;
            auto_ac_mode = 0;
        } else if (temp >= 30.0f) {
            auto_fan_pwm = 999;
            auto_ac_mode = 1;
        } else if (temp >= 28.0f) {
            auto_fan_pwm = 800;
            auto_ac_mode = 1;
        } else if (temp >= 26.0f) {
            auto_fan_pwm = 600;
            auto_ac_mode = 1;
        } else if (temp >= 24.0f) {
            auto_fan_pwm = 400;
            auto_ac_mode = 0;
        } else if (temp >= 22.0f) {
            auto_fan_pwm = 250;
            auto_ac_mode = 0;
        } else {
            auto_fan_pwm = 0;
            auto_ac_mode = 0;
        }

        if (hum > 70 && auto_fan_pwm < 999) {
            auto_fan_pwm += 100;
        }

        if (auto_fan_pwm > 999) {
            auto_fan_pwm = 999;
        }

        uint32_t now = HAL_GetTick();

        if (seat_occupied &&
            HAL_GPIO_ReadPin(BTN_UP_GPIO_Port, BTN_UP_Pin) == GPIO_PIN_RESET &&
            now - btn_last_tick[0] > BTN_DEBOUNCE_MS) {

            btn_last_tick[0] = now;

            if (!manual_mode) {
                manual_fan_pwm = auto_fan_pwm;
            }

            manual_mode = 1;
            manual_last_action = now;

            manual_fan_pwm = (manual_fan_pwm + 200 > 999) ? 999 : manual_fan_pwm + 200;
        }

        if (seat_occupied &&
            HAL_GPIO_ReadPin(BTN_DOWN_GPIO_Port, BTN_DOWN_Pin) == GPIO_PIN_RESET &&
            now - btn_last_tick[1] > BTN_DEBOUNCE_MS) {

            btn_last_tick[1] = now;

            if (!manual_mode) {
                manual_fan_pwm = auto_fan_pwm;
            }

            manual_mode = 1;
            manual_last_action = now;

            manual_fan_pwm = (manual_fan_pwm < 200) ? 0 : manual_fan_pwm - 200;
        }

        if (seat_occupied &&
            HAL_GPIO_ReadPin(BTN_AC_GPIO_Port, BTN_AC_Pin) == GPIO_PIN_RESET &&
            now - btn_last_tick[2] > BTN_DEBOUNCE_MS) {

            btn_last_tick[2] = now;
            manual_mode = 1;
            manual_last_action = now;
            ac_mode ^= 1;
        }

        if (manual_mode && now - manual_last_action > MANUAL_TIMEOUT_MS) {
            manual_mode = 0;
        }

        if (manual_mode) {
            fan_pwm = manual_fan_pwm;
        } else {
            fan_pwm = auto_fan_pwm;
            ac_mode = auto_ac_mode;
        }

        if (!seat_occupied) {
            fan_pwm = 0;
            ac_mode = 0;
            manual_mode = 0;
            manual_fan_pwm = 0;
        }

        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, fan_pwm);

        HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, seat_occupied ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, ac_mode ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED_ORANGE_GPIO_Port, LED_ORANGE_Pin, (temp >= 28.0f) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, !dht11_valid ? GPIO_PIN_SET : GPIO_PIN_RESET);

        static uint32_t lcd_last = 0;
        if (HAL_GetTick() - lcd_last > 500) {
            lcd_last = HAL_GetTick();

            snprintf(lcd_line1, 17, "T:%dC H:%d%%      ", dht11_temp, dht11_hum);
            LCD_SetCursor(0, 0);
            LCD_Print(lcd_line1);

            snprintf(lcd_line2, 17, "%s F:%d%% %s   ",
                     manual_mode ? "MAN " : "AUTO",
                     (int)(fan_pwm * 100 / 999),
                     ac_mode ? "AC" : "OFF");

            LCD_SetCursor(0, 1);
            LCD_Print(lcd_line2);
        }

        static uint32_t can_last = 0;
        if (HAL_GetTick() - can_last > 1000) {
            can_last = HAL_GetTick();

            uint8_t can_data[5];

            can_data[0] = dht11_temp;
            can_data[1] = dht11_hum;
            can_data[2] = (fan_pwm * 100) / 999;
            can_data[3] = ac_mode;
            can_data[4] = seat_occupied;

            MCP2515_SendMessage(0x123, can_data, 5);
        }

        HAL_Delay(50);
    }
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 25;
    RCC_OscInitStruct.PLL.PLLN = 168;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 4;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;

    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }
}

void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
    }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif
