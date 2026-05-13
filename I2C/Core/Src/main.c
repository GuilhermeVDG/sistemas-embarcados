/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define PCF8591_ADDR       (0x48 << 1)

#define PCF8591_CH0        0x00
#define PCF8591_CH1        0x01
#define PCF8591_CH3        0x03
#define PCF8591_DAC_ENABLE 0x40

#define RX_BUFFER_SIZE     64
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;

/* USER CODE BEGIN PV */
uint8_t uart_rx_byte;

char uart_cmd_buffer[RX_BUFFER_SIZE];
volatile uint8_t uart_cmd_index = 0;
volatile uint8_t uart_cmd_ready = 0;

uint8_t i2c_tx_buffer[2];
uint8_t i2c_rx_buffer[2];

volatile uint8_t current_channel = 0;
volatile uint8_t waiting_adc_read = 0;
volatile uint8_t waiting_dac_write = 0;
volatile uint8_t current_dac_value = 0;

char uart_tx_buffer[128];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART3_UART_Init(void);

/* USER CODE BEGIN PFP */
void process_command(char *cmd);
void pcf8591_read_channel(uint8_t channel);
void pcf8591_set_dac(uint8_t value);
void debug_print(const char *msg);
void blink_startup(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void debug_print(const char *msg)
{
  HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), 100);
}

void blink_startup(void)
{
  for (int i = 0; i < 3; i++)
  {
    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
    HAL_Delay(150);
    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
    HAL_Delay(150);
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
  MX_USART3_UART_Init();

  /* USER CODE BEGIN 2 */

  blink_startup();

  debug_print("\r\n====================================\r\n");
  debug_print("Sistema iniciado na USART2\r\n");
  debug_print("Terminal: Tera Term / COM3 / 115200\r\n");
  debug_print("I2C1: PCF8591 endereco 0x48\r\n");
  debug_print("====================================\r\n");
  debug_print("Comandos disponiveis:\r\n");
  debug_print("Read_AIN0\r\n");
  debug_print("Read_AIN1\r\n");
  debug_print("Read_AIN3\r\n");
  debug_print("Set_DAC_0 ate Set_DAC_255\r\n");
  debug_print("Exemplo: Set_DAC_125\r\n");
  debug_print("====================================\r\n\r\n");

  HAL_UART_Receive_IT(&huart2, &uart_rx_byte, 1);

  /* USER CODE END 2 */

  /* Infinite loop */
  while (1)
  {
    /* USER CODE BEGIN WHILE */

    if (uart_cmd_ready)
    {
      uart_cmd_ready = 0;

      process_command(uart_cmd_buffer);

      memset(uart_cmd_buffer, 0, RX_BUFFER_SIZE);
    }

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

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{
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
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{
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
}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
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
}

/* USER CODE BEGIN 4 */

void process_command(char *cmd)
{
  int len = strlen(cmd);

  while (len > 0 &&
         (cmd[len - 1] == '\r' ||
          cmd[len - 1] == '\n' ||
          cmd[len - 1] == ' '  ||
          cmd[len - 1] == '\t'))
  {
    cmd[len - 1] = '\0';
    len--;
  }

  snprintf(uart_tx_buffer, sizeof(uart_tx_buffer), "\r\nComando recebido: %s\r\n", cmd);
  debug_print(uart_tx_buffer);

  if (strcmp(cmd, "Read_AIN0") == 0)
  {
    debug_print("Lendo AIN0...\r\n");
    pcf8591_read_channel(PCF8591_CH0);
  }
  else if (strcmp(cmd, "Read_AIN1") == 0)
  {
    debug_print("Lendo AIN1...\r\n");
    pcf8591_read_channel(PCF8591_CH1);
  }
  else if (strcmp(cmd, "Read_AIN3") == 0)
  {
    debug_print("Lendo AIN3...\r\n");
    pcf8591_read_channel(PCF8591_CH3);
  }
  else if (strncmp(cmd, "Set_DAC_", 8) == 0)
  {
    int dac_value = -1;
    char extra_char = '\0';

    if (sscanf(cmd, "Set_DAC_%d%c", &dac_value, &extra_char) >= 1)
    {
      if (dac_value >= 0 && dac_value <= 255)
      {
        snprintf(uart_tx_buffer, sizeof(uart_tx_buffer), "Enviando DAC: %d\r\n", dac_value);
        debug_print(uart_tx_buffer);

        pcf8591_set_dac((uint8_t)dac_value);
      }
      else
      {
        debug_print("Valor invalido. Use um numero de 0 a 255\r\n");
      }
    }
    else
    {
      debug_print("Formato invalido. Exemplo correto: Set_DAC_125\r\n");
    }
  }
  else
  {
    debug_print("Comando invalido\r\n");
    debug_print("Use: Read_AIN0, Read_AIN1, Read_AIN3 ou Set_DAC_125\r\n");
  }
}

void pcf8591_read_channel(uint8_t channel)
{
  current_channel = channel;
  waiting_adc_read = 1;
  waiting_dac_write = 0;

  i2c_tx_buffer[0] = channel;

  if (HAL_I2C_Master_Transmit_IT(&hi2c1, PCF8591_ADDR, i2c_tx_buffer, 1) != HAL_OK)
  {
    waiting_adc_read = 0;
    debug_print("Erro ao iniciar transmissao I2C\r\n");
  }
}

void pcf8591_set_dac(uint8_t value)
{
  current_dac_value = value;
  waiting_adc_read = 0;
  waiting_dac_write = 1;

  i2c_tx_buffer[0] = PCF8591_DAC_ENABLE;
  i2c_tx_buffer[1] = value;

  if (HAL_I2C_Master_Transmit_IT(&hi2c1, PCF8591_ADDR, i2c_tx_buffer, 2) != HAL_OK)
  {
    waiting_dac_write = 0;
    debug_print("Erro ao iniciar escrita DAC via I2C\r\n");
  }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART2)
  {
    HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);

    if ((uart_rx_byte == '\r') || (uart_rx_byte == '\n'))
    {
      if ((uart_cmd_index > 0) && (uart_cmd_ready == 0))
      {
        uart_cmd_buffer[uart_cmd_index] = '\0';
        uart_cmd_ready = 1;
        uart_cmd_index = 0;
      }
    }
    else if (uart_cmd_ready == 0)
    {
      if (uart_cmd_index < (RX_BUFFER_SIZE - 1))
      {
        uart_cmd_buffer[uart_cmd_index++] = (char)uart_rx_byte;
      }
      else
      {
        uart_cmd_index = 0;
        memset(uart_cmd_buffer, 0, RX_BUFFER_SIZE);
        debug_print("\r\nBuffer cheio. Digite novamente.\r\n");
      }
    }

    HAL_UART_Receive_IT(&huart2, &uart_rx_byte, 1);
  }
}

void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
  if (hi2c->Instance == I2C1)
  {
    if (waiting_adc_read)
    {
      if (HAL_I2C_Master_Receive_IT(&hi2c1, PCF8591_ADDR, i2c_rx_buffer, 2) != HAL_OK)
      {
        waiting_adc_read = 0;
        debug_print("Erro ao iniciar leitura I2C\r\n");
      }
    }
    else if (waiting_dac_write)
    {
      waiting_dac_write = 0;

      snprintf(uart_tx_buffer,
               sizeof(uart_tx_buffer),
               "Valor do DAC confirmado: %u\r\n",
               current_dac_value);

      debug_print(uart_tx_buffer);
    }
  }
}

void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
  if (hi2c->Instance == I2C1)
  {
    if (waiting_adc_read)
    {
      waiting_adc_read = 0;

      uint8_t value = i2c_rx_buffer[1];

      if (current_channel == PCF8591_CH0)
      {
        snprintf(uart_tx_buffer,
                 sizeof(uart_tx_buffer),
                 "AIN0: %u\r\n",
                 value);
      }
      else if (current_channel == PCF8591_CH1)
      {
        snprintf(uart_tx_buffer,
                 sizeof(uart_tx_buffer),
                 "AIN1: %u\r\n",
                 value);
      }
      else if (current_channel == PCF8591_CH3)
      {
        snprintf(uart_tx_buffer,
                 sizeof(uart_tx_buffer),
                 "AIN3: %u\r\n",
                 value);
      }
      else
      {
        snprintf(uart_tx_buffer,
                 sizeof(uart_tx_buffer),
                 "Canal invalido\r\n");
      }

      debug_print(uart_tx_buffer);
    }
  }
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
  if (hi2c->Instance == I2C1)
  {
    waiting_adc_read = 0;
    waiting_dac_write = 0;

    snprintf(uart_tx_buffer,
             sizeof(uart_tx_buffer),
             "Erro I2C. Codigo HAL: %lu\r\n",
             HAL_I2C_GetError(&hi2c1));

    debug_print(uart_tx_buffer);
  }
}

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
    HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
    HAL_Delay(200);
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif
