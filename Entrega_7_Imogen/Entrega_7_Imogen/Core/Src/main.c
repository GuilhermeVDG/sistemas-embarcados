/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Projeto completo - LCD paralelo + WCMCU-75 + MQ-135 CO2 estimado
  ******************************************************************************
  */
/* USER CODE END Header */

#include "main.h"
#include "adc.h"
#include "i2c.h"
#include "usart.h"
#include "gpio.h"

/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

void SystemClock_Config(void);

/* USER CODE BEGIN PV */

/* =========================================================
   LCD 1602 PARALELO - MODO 4 BITS

   Ligações:
   LCD RS -> PA8
   LCD E  -> PA9
   LCD D4 -> PB5
   LCD D5 -> PB4
   LCD D6 -> PB10
   LCD D7 -> PC7

   LCD VSS -> GND
   LCD VDD -> 5V
   LCD VO  -> GND ou potenciômetro de contraste
   LCD RW  -> GND
   LCD A   -> 5V com resistor, pode ser 300 ohms
   LCD K   -> GND
   ========================================================= */

#define LCD_RS_PORT GPIOA
#define LCD_RS_PIN  GPIO_PIN_8

#define LCD_E_PORT GPIOA
#define LCD_E_PIN  GPIO_PIN_9

#define LCD_D4_PORT GPIOB
#define LCD_D4_PIN  GPIO_PIN_5

#define LCD_D5_PORT GPIOB
#define LCD_D5_PIN  GPIO_PIN_4

#define LCD_D6_PORT GPIOB
#define LCD_D6_PIN  GPIO_PIN_10

#define LCD_D7_PORT GPIOC
#define LCD_D7_PIN  GPIO_PIN_7

/* =========================================================
   WCMCU-75

   O WCMCU-75 normalmente usa LM75/LM75A internamente.
   Endereços possíveis: 0x48 até 0x4F.
   Os pinos A0/A1/A2 definem o endereço.

   Recomendado:
   A0 -> GND
   A1 -> GND
   A2 -> GND
   para fixar em 0x48.
   ========================================================= */

#define WCMCU75_ADDR_MIN 0x48
#define WCMCU75_ADDR_MAX 0x4F

static uint16_t wcmcu75_addr = (0x48 << 1);
static uint8_t  wcmcu75_found = 0;

/* =========================================================
   MQ-135 / CO2 estimado via ADC

   Ligação usada:
   AO do MQ-135 -> resistor 10k -> nó central -> resistor 1k -> GND
   nó central -> PA0

   Fator teórico do divisor = 11.

   Fórmula didática usada:
   co2_ppm = 400 + (adc_raw - 550) * 2

   Observação:
   O valor é uma estimativa didática baseada na variação analógica.
   Não é medição calibrada/laboratorial real de CO2.
   ========================================================= */

#define ADC_MAX_COUNTS 4095UL
#define VREF_MV        3300UL
#define DIVIDER_FACTOR 11UL

/* USER CODE END PV */

/* USER CODE BEGIN PFP */

/* UART */
static void uart_print(const char *s);

/* LCD */
static void lcd_pulse_enable(void);
static void lcd_write4(uint8_t nibble);
static void lcd_send(uint8_t value, uint8_t rs);
static void lcd_cmd(uint8_t cmd);
static void lcd_data(uint8_t data);
static void lcd_init(void);
static void lcd_clear(void);
static void lcd_set_cursor(uint8_t row, uint8_t col);
static void lcd_print(const char *s);
static void lcd_print_padded(const char *s);

/* WCMCU-75 */
static uint8_t wcmcu75_detect_address(void);
static int16_t wcmcu75_read_temp_x10(void);

/* ADC / MQ-135 */
static uint32_t adc_read(void);
static uint32_t mq135_vpin_mv(uint32_t raw);
static uint32_t mq135_vao_mv(uint32_t raw);
static uint32_t mq135_estimate_co2_ppm(uint32_t raw);

/* USER CODE END PFP */

/* USER CODE BEGIN 0 */

static void uart_print(const char *s)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)s, strlen(s), HAL_MAX_DELAY);
}

/* =========================================================
   LCD 1602 PARALELO - DRIVER 4 BITS
   ========================================================= */

static void lcd_pulse_enable(void)
{
    HAL_GPIO_WritePin(LCD_E_PORT, LCD_E_PIN, GPIO_PIN_SET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(LCD_E_PORT, LCD_E_PIN, GPIO_PIN_RESET);
    HAL_Delay(1);
}

static void lcd_write4(uint8_t nibble)
{
    HAL_GPIO_WritePin(LCD_D4_PORT, LCD_D4_PIN, (nibble & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LCD_D5_PORT, LCD_D5_PIN, (nibble & 0x02) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LCD_D6_PORT, LCD_D6_PIN, (nibble & 0x04) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LCD_D7_PORT, LCD_D7_PIN, (nibble & 0x08) ? GPIO_PIN_SET : GPIO_PIN_RESET);

    lcd_pulse_enable();
}

static void lcd_send(uint8_t value, uint8_t rs)
{
    HAL_GPIO_WritePin(LCD_RS_PORT, LCD_RS_PIN, rs ? GPIO_PIN_SET : GPIO_PIN_RESET);

    lcd_write4(value >> 4);
    lcd_write4(value & 0x0F);

    HAL_Delay(2);
}

static void lcd_cmd(uint8_t cmd)
{
    lcd_send(cmd, 0);
}

static void lcd_data(uint8_t data)
{
    lcd_send(data, 1);
}

static void lcd_init(void)
{
    HAL_Delay(50);

    HAL_GPIO_WritePin(LCD_RS_PORT, LCD_RS_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LCD_E_PORT, LCD_E_PIN, GPIO_PIN_RESET);

    lcd_write4(0x03);
    HAL_Delay(5);

    lcd_write4(0x03);
    HAL_Delay(5);

    lcd_write4(0x03);
    HAL_Delay(5);

    lcd_write4(0x02);
    HAL_Delay(5);

    lcd_cmd(0x28); /* 4 bits, 2 linhas, fonte 5x8 */
    lcd_cmd(0x0C); /* display ON, cursor OFF */
    lcd_cmd(0x06); /* incremento automático */
    lcd_cmd(0x01); /* limpa display */
    HAL_Delay(5);
}

static void lcd_clear(void)
{
    lcd_cmd(0x01);
    HAL_Delay(5);
}

static void lcd_set_cursor(uint8_t row, uint8_t col)
{
    uint8_t addr;

    if (row == 0)
        addr = 0x00 + col;
    else
        addr = 0x40 + col;

    lcd_cmd(0x80 | addr);
}

static void lcd_print(const char *s)
{
    while (*s)
    {
        lcd_data((uint8_t)*s);
        s++;
    }
}

static void lcd_print_padded(const char *s)
{
    uint8_t count = 0;

    while (*s && count < 16)
    {
        lcd_data((uint8_t)*s);
        s++;
        count++;
    }

    while (count < 16)
    {
        lcd_data(' ');
        count++;
    }
}

/* =========================================================
   WCMCU-75 / LM75A
   ========================================================= */

static uint8_t wcmcu75_detect_address(void)
{
    for (uint8_t addr = WCMCU75_ADDR_MIN; addr <= WCMCU75_ADDR_MAX; addr++)
    {
        if (HAL_I2C_IsDeviceReady(&hi2c1, (addr << 1), 3, 100) == HAL_OK)
        {
            wcmcu75_addr = (addr << 1);
            wcmcu75_found = 1;
            return 1;
        }
    }

    wcmcu75_found = 0;
    return 0;
}

static int16_t wcmcu75_read_temp_x10(void)
{
    uint8_t buf[2];

    if (!wcmcu75_found)
    {
        return -9999;
    }

    if (HAL_I2C_Master_Receive(&hi2c1, wcmcu75_addr, buf, 2, 100) != HAL_OK)
    {
        return -9999;
    }

    /*
       WCMCU-75 / LM75A:
       9 bits de temperatura.
       Resolução: 0.5 C.
       temp_x10 = raw * 5.
    */
    int16_t raw = (int16_t)((buf[0] << 8) | buf[1]);

    raw >>= 7;

    if (raw & 0x0100)
    {
        raw |= 0xFE00;
    }

    return raw * 5;
}

/* =========================================================
   ADC / MQ-135 / CO2 ESTIMADO
   ========================================================= */

static uint32_t adc_read(void)
{
    uint32_t value = 0;

    HAL_ADC_Start(&hadc1);

    if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK)
    {
        value = HAL_ADC_GetValue(&hadc1);
    }

    HAL_ADC_Stop(&hadc1);

    return value;
}

static uint32_t mq135_vpin_mv(uint32_t raw)
{
    /*
       Vpin = raw * 3300 / 4095
       Resultado em milivolts.
    */
    return (raw * VREF_MV) / ADC_MAX_COUNTS;
}

static uint32_t mq135_vao_mv(uint32_t raw)
{
    /*
       Vao estimado desfazendo o divisor resistivo.
       Como o divisor teórico é 10k/1k, fator = 11.
    */
    return mq135_vpin_mv(raw) * DIVIDER_FACTOR;
}

static uint32_t mq135_estimate_co2_ppm(uint32_t raw)
{
    /*
       Estimativa didática de CO2 baseada na leitura analógica.

       Fórmula inicial:
       co2_ppm = 400 + (raw - 550) * 2

       Exemplos:
       raw=550 -> 400 ppm
       raw=600 -> 500 ppm
       raw=700 -> 700 ppm
       raw=800 -> 900 ppm

       Observação:
       Isso NÃO é uma calibração real de CO2.
       É uma escala didática para visualizar tendência de variação.
    */

    int32_t base_raw = 550;
    int32_t diff = (int32_t)raw - base_raw;

    int32_t ppm = 400 + (diff * 2);

    if (ppm < 0)
    {
        ppm = 0;
    }

    if (ppm > 9999)
    {
        ppm = 9999;
    }

    return (uint32_t)ppm;
}

/* USER CODE END 0 */

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_USART2_UART_Init();

  /* USER CODE BEGIN 2 */

  HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);

  lcd_init();
  lcd_clear();

  lcd_set_cursor(0, 0);
  lcd_print("Sistema OK");

  lcd_set_cursor(1, 0);
  lcd_print("Iniciando...");
  HAL_Delay(1200);

  char linha1[17];
  char linha2[17];
  char json[240];

  /* USER CODE END 2 */

  while (1)
  {
    /* USER CODE BEGIN 3 */

    /*
       Detecta o WCMCU-75 antes de cada leitura.
       Isso deixa o sistema robusto se o endereço variar.
       Ideal: fixar A0/A1/A2 no GND.
    */
    wcmcu75_detect_address();

    int16_t temp_x10 = wcmcu75_read_temp_x10();

    uint32_t raw = adc_read();
    uint32_t vpin_mv = mq135_vpin_mv(raw);
    uint32_t vao_mv = mq135_vao_mv(raw);
    uint32_t co2_ppm = mq135_estimate_co2_ppm(raw);

    if (temp_x10 == -9999)
    {
        snprintf(linha1, sizeof(linha1), "Temp: ERRO I2C");
        snprintf(linha2, sizeof(linha2), "CO2~%4lu ppm", (unsigned long)co2_ppm);

        snprintf(json, sizeof(json),
                 "{\"temperature_c\":null,\"co2_ppm\":%lu,\"adc_raw\":%lu,\"vpin_mv\":%lu,\"vao_mv\":%lu,\"sensor_status\":\"ERRO_I2C\"}\r\n",
                 (unsigned long)co2_ppm,
                 (unsigned long)raw,
                 (unsigned long)vpin_mv,
                 (unsigned long)vao_mv);
    }
    else
    {
        int16_t temp_abs;
        int16_t temp_int;
        int16_t temp_dec;
        char sinal = '+';

        if (temp_x10 < 0)
        {
            sinal = '-';
            temp_abs = -temp_x10;
        }
        else
        {
            temp_abs = temp_x10;
        }

        temp_int = temp_abs / 10;
        temp_dec = temp_abs % 10;

        if (sinal == '-')
        {
            snprintf(linha1, sizeof(linha1), "Temp:-%02d.%d C", temp_int, temp_dec);

            snprintf(json, sizeof(json),
                     "{\"temperature_c\":-%d.%d,\"co2_ppm\":%lu,\"adc_raw\":%lu,\"vpin_mv\":%lu,\"vao_mv\":%lu,\"sensor_status\":\"OK\"}\r\n",
                     temp_int,
                     temp_dec,
                     (unsigned long)co2_ppm,
                     (unsigned long)raw,
                     (unsigned long)vpin_mv,
                     (unsigned long)vao_mv);
        }
        else
        {
            snprintf(linha1, sizeof(linha1), "Temp:%02d.%d C", temp_int, temp_dec);

            snprintf(json, sizeof(json),
                     "{\"temperature_c\":%d.%d,\"co2_ppm\":%lu,\"adc_raw\":%lu,\"vpin_mv\":%lu,\"vao_mv\":%lu,\"sensor_status\":\"OK\"}\r\n",
                     temp_int,
                     temp_dec,
                     (unsigned long)co2_ppm,
                     (unsigned long)raw,
                     (unsigned long)vpin_mv,
                     (unsigned long)vao_mv);
        }

        snprintf(linha2, sizeof(linha2), "CO2~%4lu ppm", (unsigned long)co2_ppm);
    }

    lcd_set_cursor(0, 0);
    lcd_print_padded(linha1);

    lcd_set_cursor(1, 0);
    lcd_print_padded(linha2);

    uart_print(json);

    HAL_Delay(1000);

    /* USER CODE END 3 */
  }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 40;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                              | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;

  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
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
