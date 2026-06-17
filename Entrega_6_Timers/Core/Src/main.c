/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body - PCF8591 + PWM + DAC + Timer + UART
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum
{
  MODO_RGB = 1,
  MODO_DAC = 2,
  MODO_RGB_DAC = 3
} ModoOperacao;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define PCF8591_ADDR        (0x48 << 1)

/*
  MAPEAMENTO FINAL DOS CANAIS:

  CH0 = LDR
  CH3 = POTENCIOMETRO
*/
#define PCF8591_CH_LDR      0
#define PCF8591_CH_POT      3

#define PWM_MAX_VALUE       999
#define SINE_TABLE_SIZE     10

#define I2C_TIMEOUT_MS      100

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim6;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

volatile ModoOperacao modo_atual = MODO_RGB;

uint8_t pot_value = 0;
uint8_t ldr_value = 0;
uint8_t dac_value_atual = 0;

uint8_t ch0_value = 0;
uint8_t ch1_value = 0;
uint8_t ch2_value = 0;
uint8_t ch3_value = 0;

uint16_t pwm_value = 0;

volatile uint8_t sine_index = 0;
volatile uint8_t flag_timer_dac = 0;
volatile uint8_t flag_botao_pressionado = 0;

uint8_t i2c_ok = 0;

const uint8_t sine_table[SINE_TABLE_SIZE] =
{
  128, 203, 249, 249, 203,
  128, 52, 6, 6, 52
};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM6_Init(void);
/* USER CODE BEGIN PFP */

uint8_t PCF8591_ReadADC(uint8_t channel);
uint8_t PCF8591_WriteDAC(uint8_t value);

uint16_t Converter_ADC_Para_PWM(uint8_t adc_value);

void Atualizar_PWM_RGB(uint16_t value);
void Desligar_PWM_RGB(void);

void UART_Print(char *msg);
void UART_Print_Status(void);

void Piscar_LD2(uint8_t quantidade);
void Processar_Botao(void);
void Processar_DAC(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void UART_Print(char *msg)
{
  HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), 100);
}

void UART_Print_Status(void)
{
  char buffer[240];

  snprintf(buffer, sizeof(buffer),
           "Modo:%d | POT:%3d | LDR:%3d | PWM:%4d | DAC:%3d | I2C:%s | CH0:%3d CH1:%3d CH2:%3d CH3:%3d\r\n",
           modo_atual,
           pot_value,
           ldr_value,
           pwm_value,
           dac_value_atual,
           i2c_ok ? "OK" : "ERRO",
           ch0_value,
           ch1_value,
           ch2_value,
           ch3_value);

  UART_Print(buffer);
}

uint8_t PCF8591_ReadADC(uint8_t channel)
{
  uint8_t control_byte;
  uint8_t adc_value = 0;
  HAL_StatusTypeDef status;

  control_byte = 0x40 | (channel & 0x03);

  status = HAL_I2C_Master_Transmit(&hi2c1, PCF8591_ADDR, &control_byte, 1, I2C_TIMEOUT_MS);

  if (status != HAL_OK)
  {
    i2c_ok = 0;
    return 0;
  }

  HAL_I2C_Master_Receive(&hi2c1, PCF8591_ADDR, &adc_value, 1, I2C_TIMEOUT_MS);

  status = HAL_I2C_Master_Receive(&hi2c1, PCF8591_ADDR, &adc_value, 1, I2C_TIMEOUT_MS);

  if (status != HAL_OK)
  {
    i2c_ok = 0;
    return 0;
  }

  i2c_ok = 1;
  return adc_value;
}

uint8_t PCF8591_WriteDAC(uint8_t value)
{
  uint8_t data[2];
  HAL_StatusTypeDef status;

  data[0] = 0x40;
  data[1] = value;

  status = HAL_I2C_Master_Transmit(&hi2c1, PCF8591_ADDR, data, 2, I2C_TIMEOUT_MS);

  if (status != HAL_OK)
  {
    i2c_ok = 0;
    return 0;
  }

  i2c_ok = 1;
  return 1;
}

uint16_t Converter_ADC_Para_PWM(uint8_t adc_value)
{
  return (uint16_t)(((uint32_t)adc_value * PWM_MAX_VALUE) / 255);
}

void Atualizar_PWM_RGB(uint16_t value)
{
  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, value);
  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, value);
  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, value);
}

void Desligar_PWM_RGB(void)
{
  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 0);
  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 0);
  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, 0);
}

void Piscar_LD2(uint8_t quantidade)
{
  for (uint8_t i = 0; i < quantidade; i++)
  {
    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
    HAL_Delay(120);

    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
    HAL_Delay(120);
  }
}

void Processar_Botao(void)
{
  if (flag_botao_pressionado == 0)
  {
    return;
  }

  flag_botao_pressionado = 0;

  if (modo_atual == MODO_RGB)
  {
    modo_atual = MODO_DAC;
    UART_Print("\r\nBotao pressionado -> Modo 2: DAC\r\n");
    Piscar_LD2(2);
  }
  else if (modo_atual == MODO_DAC)
  {
    modo_atual = MODO_RGB_DAC;
    UART_Print("\r\nBotao pressionado -> Modo 3: RGB + DAC\r\n");
    Piscar_LD2(3);
  }
  else
  {
    modo_atual = MODO_RGB;
    UART_Print("\r\nBotao pressionado -> Modo 1: RGB/PWM\r\n");
    Piscar_LD2(1);
  }
}

void Processar_DAC(void)
{
  if (flag_timer_dac == 0)
  {
    return;
  }

  flag_timer_dac = 0;

  if ((modo_atual == MODO_DAC) || (modo_atual == MODO_RGB_DAC))
  {
    uint8_t sine_value;

    sine_value = sine_table[sine_index];

    /*
      Amplitude controlada pelo LDR.
      Quanto maior o valor do LDR, maior o valor enviado ao DAC.
    */
    dac_value_atual = (uint8_t)(((uint16_t)sine_value * ldr_value) / 255);

    PCF8591_WriteDAC(dac_value_atual);

    sine_index++;

    if (sine_index >= SINE_TABLE_SIZE)
    {
      sine_index = 0;
    }
  }
  else
  {
    dac_value_atual = 0;
    PCF8591_WriteDAC(0);
    sine_index = 0;
  }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == B1_Pin)
  {
    flag_botao_pressionado = 1;
  }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM6)
  {
    flag_timer_dac = 1;
  }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  MX_TIM2_Init();
  MX_TIM6_Init();

  /* USER CODE BEGIN 2 */

  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);

  HAL_TIM_Base_Start_IT(&htim6);

  modo_atual = MODO_RGB;

  Desligar_PWM_RGB();
  PCF8591_WriteDAC(0);

  UART_Print("\r\n========================================\r\n");
  UART_Print("Projeto PCF8591 + PWM + DAC + Timer\r\n");
  UART_Print("Modo inicial: 1 - RGB/PWM\r\n");
  UART_Print("Botao azul alterna: 1 -> 2 -> 3 -> 1\r\n");
  UART_Print("UART: 115200 baud, 8N1\r\n");
  UART_Print("Mapeamento final: LDR=CH0 | POT=CH3\r\n");
  UART_Print("========================================\r\n\r\n");

  Piscar_LD2(1);

  uint32_t last_print = 0;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    Processar_Botao();

    /*
      Leitura dos quatro canais para diagnóstico.
    */
    ch0_value = PCF8591_ReadADC(0);
    ch1_value = PCF8591_ReadADC(1);
    ch2_value = PCF8591_ReadADC(2);
    ch3_value = PCF8591_ReadADC(3);

    /*
      Mapeamento final:
      LDR = CH0
      POT = CH3
    */
    ldr_value = ch0_value;
    pot_value = ch3_value;

    pwm_value = Converter_ADC_Para_PWM(pot_value);

    if ((modo_atual == MODO_RGB) || (modo_atual == MODO_RGB_DAC))
    {
      Atualizar_PWM_RGB(pwm_value);
    }
    else
    {
      Desligar_PWM_RGB();
    }

    Processar_DAC();

    if (HAL_GetTick() - last_print >= 500)
    {
      last_print = HAL_GetTick();
      UART_Print_Status();
    }

    HAL_Delay(20);

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
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

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 10;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK |
                                RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 |
                                RCC_CLOCKTYPE_PCLK2;

  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{
  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */

  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x10909CEC;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */
}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{
  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */

  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 79;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;

  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;

  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }

  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;

  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

  HAL_TIM_MspPostInit(&htim2);
}

/**
  * @brief TIM6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM6_Init(void)
{
  /* USER CODE BEGIN TIM6_Init 0 */

  /* USER CODE END TIM6_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM6_Init 1 */

  /* USER CODE END TIM6_Init 1 */

  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 7999;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = 999;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    Error_Handler();
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;

  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN TIM6_Init 2 */

  /* USER CODE END TIM6_Init 2 */
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{
  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */

  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  __disable_irq();

  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */

  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
