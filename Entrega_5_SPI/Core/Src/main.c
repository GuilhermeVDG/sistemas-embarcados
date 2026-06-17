/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Caso de teste - NUCLEO-L476RG + PCF8591 + MAX7219
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
#include <ctype.h>
#include <stdarg.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

typedef enum
{
  DISPLAY_NONE = 0,
  DISPLAY_TEMP,
  DISPLAY_VOLT,
  DISPLAY_LDR
} DisplayMode_t;

typedef enum
{
  PCF_OP_NONE = 0,
  PCF_OP_READ_DIRECT,
  PCF_OP_READ_DISPLAY
} PcfOperation_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define PCF8591_ADDR          (0x48 << 1)
#define PCF8591_CH0           0x00
#define PCF8591_CH1           0x01
#define PCF8591_CH3           0x03
#define PCF8591_DAC_ENABLE    0x40

/*
  MAPEAMENTO CORRIGIDO:
  LDR  = AIN0
  TEMP = AIN1
  VOLT = AIN3
*/
#define PCF8591_CH_LDR        PCF8591_CH0
#define PCF8591_CH_TEMP       PCF8591_CH1
#define PCF8591_CH_VOLT       PCF8591_CH3

/* MAX7219 registers */
#define MAX7219_REG_NOOP      0x00
#define MAX7219_REG_DIGIT0    0x01
#define MAX7219_REG_DECODE    0x09
#define MAX7219_REG_INTENSITY 0x0A
#define MAX7219_REG_SCANLIMIT 0x0B
#define MAX7219_REG_SHUTDOWN  0x0C
#define MAX7219_REG_TEST      0x0F

#define UART_RX_BUFFER_SIZE   64

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi1;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

static uint8_t uart_rx_byte;
static char uart_rx_buffer[UART_RX_BUFFER_SIZE];
static volatile uint8_t uart_rx_index = 0;
static volatile uint8_t uart_cmd_ready = 0;

static uint8_t i2c_tx_buffer[2];
static uint8_t i2c_rx_buffer[2];

static uint8_t last_dac_value = 0;

static DisplayMode_t current_mode = DISPLAY_NONE;
static uint8_t current_sensor_value = 0;
static uint32_t last_toggle_time = 0;
static uint8_t toggle_state = 0;

static volatile uint8_t current_channel = 0;
static volatile PcfOperation_t pending_pcf_operation = PCF_OP_NONE;
static volatile DisplayMode_t pending_display_mode = DISPLAY_NONE;

static const uint8_t CHAR_T[8] =
{
  0b11111111,
  0b00011000,
  0b00011000,
  0b00011000,
  0b00011000,
  0b00011000,
  0b00011000,
  0b00000000
};

static const uint8_t CHAR_V[8] =
{
  0b10000001,
  0b10000001,
  0b01000010,
  0b01000010,
  0b00100100,
  0b00100100,
  0b00011000,
  0b00000000
};

static const uint8_t CHAR_L[8] =
{
  0b10000000,
  0b10000000,
  0b10000000,
  0b10000000,
  0b10000000,
  0b10000000,
  0b11111111,
  0b00000000
};

static const uint8_t CHAR_PLUS[8] =
{
  0b00000000,
  0b00011000,
  0b00011000,
  0b01111110,
  0b01111110,
  0b00011000,
  0b00011000,
  0b00000000
};

static const uint8_t CHAR_MINUS[8] =
{
  0b00000000,
  0b00000000,
  0b00000000,
  0b01111110,
  0b01111110,
  0b00000000,
  0b00000000,
  0b00000000
};

static const uint8_t CHAR_CHECKER[8] =
{
  0b10101010,
  0b01010101,
  0b10101010,
  0b01010101,
  0b10101010,
  0b01010101,
  0b10101010,
  0b01010101
};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI1_Init(void);

/* USER CODE BEGIN PFP */

static void UART_Print(const char *msg);
static void UART_Printf(const char *fmt, ...);

static void MAX7219_Write(uint8_t reg, uint8_t data);
static void MAX7219_Init(void);
static void MAX7219_Clear(void);
static void MAX7219_DisplayBitmap(const uint8_t bitmap[8]);
static void MAX7219_TestPattern(void);

static void PCF8591_StartRead(uint8_t channel, DisplayMode_t display_mode);
static const char *PCF8591_ChannelName(uint8_t channel);

static void Process_Command(char *cmd);
static void Display_UpdateTask(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

static void UART_Print(const char *msg)
{
  HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), 100);
}

static void UART_Printf(const char *fmt, ...)
{
  char buffer[160];

  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);

  UART_Print(buffer);
}

static void MAX7219_Write(uint8_t reg, uint8_t data)
{
  uint8_t tx_data[2];

  tx_data[0] = reg;
  tx_data[1] = data;

  HAL_GPIO_WritePin(MAX7219_CS_GPIO_Port, MAX7219_CS_Pin, GPIO_PIN_RESET);
  HAL_SPI_Transmit(&hspi1, tx_data, 2, HAL_MAX_DELAY);
  HAL_GPIO_WritePin(MAX7219_CS_GPIO_Port, MAX7219_CS_Pin, GPIO_PIN_SET);
}

static void MAX7219_Init(void)
{
  HAL_Delay(100);

  MAX7219_Write(MAX7219_REG_TEST, 0x00);
  MAX7219_Write(MAX7219_REG_DECODE, 0x00);
  MAX7219_Write(MAX7219_REG_SCANLIMIT, 0x07);
  MAX7219_Write(MAX7219_REG_INTENSITY, 0x03);
  MAX7219_Write(MAX7219_REG_SHUTDOWN, 0x01);

  MAX7219_Clear();
}

static void MAX7219_Clear(void)
{
  for (uint8_t i = 1; i <= 8; i++)
  {
    MAX7219_Write(i, 0x00);
  }
}

static void MAX7219_DisplayBitmap(const uint8_t bitmap[8])
{
  for (uint8_t row = 0; row < 8; row++)
  {
    MAX7219_Write(row + 1, bitmap[row]);
  }
}

static void MAX7219_TestPattern(void)
{
  MAX7219_DisplayBitmap(CHAR_CHECKER);
}

static HAL_StatusTypeDef __attribute__((unused)) PCF8591_ReadChannel(uint8_t channel, uint8_t *value)
{
  if (channel > 3 || value == NULL)
  {
    return HAL_ERROR;
  }

  uint8_t control_byte = PCF8591_DAC_ENABLE | (channel & 0x03);
  uint8_t rx_data[2] = {0};

  if (HAL_I2C_Master_Transmit(&hi2c1, PCF8591_ADDR, &control_byte, 1, 100) != HAL_OK)
  {
    return HAL_ERROR;
  }

  if (HAL_I2C_Master_Receive(&hi2c1, PCF8591_ADDR, rx_data, 2, 100) != HAL_OK)
  {
    return HAL_ERROR;
  }

  *value = rx_data[1];
  return HAL_OK;
}

static HAL_StatusTypeDef __attribute__((unused)) PCF8591_SetDAC(uint8_t value)
{
  uint8_t tx_data[2];

  tx_data[0] = PCF8591_DAC_ENABLE;
  tx_data[1] = value;

  if (HAL_I2C_Master_Transmit(&hi2c1, PCF8591_ADDR, tx_data, 2, 100) != HAL_OK)
  {
    return HAL_ERROR;
  }

  last_dac_value = value;
  return HAL_OK;
}

static const char *PCF8591_ChannelName(uint8_t channel)
{
  if (channel == PCF8591_CH0)
  {
    return "AIN0";
  }
  if (channel == PCF8591_CH1)
  {
    return "AIN1";
  }
  if (channel == PCF8591_CH3)
  {
    return "AIN3";
  }

  return "AIN?";
}

static void PCF8591_StartRead(uint8_t channel, DisplayMode_t display_mode)
{
  if (pending_pcf_operation != PCF_OP_NONE)
  {
    UART_Print("I2C ocupado. Aguarde a operacao atual terminar.\r\n");
    return;
  }

  current_channel = channel;
  pending_display_mode = display_mode;
  pending_pcf_operation = (display_mode == DISPLAY_NONE) ? PCF_OP_READ_DIRECT : PCF_OP_READ_DISPLAY;

  i2c_tx_buffer[0] = PCF8591_DAC_ENABLE | (channel & 0x03);

  if (HAL_I2C_Master_Transmit_IT(&hi2c1, PCF8591_ADDR, i2c_tx_buffer, 1) != HAL_OK)
  {
    pending_pcf_operation = PCF_OP_NONE;
    pending_display_mode = DISPLAY_NONE;
    UART_Print("Erro ao iniciar transmissao I2C\r\n");
  }
}

static void Process_Command(char *cmd)
{
  while (*cmd == ' ' || *cmd == '\t')
  {
    cmd++;
  }

  for (int i = 0; cmd[i] != '\0'; i++)
  {
    if (cmd[i] == '\r' || cmd[i] == '\n')
    {
      cmd[i] = '\0';
      break;
    }
  }

  UART_Printf("\r\nComando recebido: %s\r\n", cmd);

  if (strcmp(cmd, "Temp") == 0)
  {
    UART_Print("Lendo temperatura/AIN1...\r\n");
    PCF8591_StartRead(PCF8591_CH_TEMP, DISPLAY_TEMP);
  }
  else if (strcmp(cmd, "Volt") == 0)
  {
    UART_Print("Lendo tensao/AIN3...\r\n");
    PCF8591_StartRead(PCF8591_CH_VOLT, DISPLAY_VOLT);
  }
  else if (strcmp(cmd, "LDR") == 0)
  {
    UART_Print("Lendo luminosidade/AIN0...\r\n");
    PCF8591_StartRead(PCF8591_CH_LDR, DISPLAY_LDR);
  }
  else if (strcmp(cmd, "Clear") == 0)
  {
    current_mode = DISPLAY_NONE;
    MAX7219_Clear();
    UART_Print("Matriz apagada\r\n");
  }
  else if (strcmp(cmd, "Test") == 0)
  {
    current_mode = DISPLAY_NONE;
    MAX7219_TestPattern();
    UART_Print("Padrao de teste enviado para matriz\r\n");
  }
  else
  {
    UART_Print("Comando desconhecido\r\n");
    UART_Print("Comandos disponiveis:\r\n");
    UART_Print("  Temp\r\n");
    UART_Print("  Volt\r\n");
    UART_Print("  LDR\r\n");
    UART_Print("  Clear\r\n");
    UART_Print("  Test\r\n");
  }
}

static void __attribute__((unused)) UART_CheckCommand(void)
{
  uint8_t rx_char;

  if (HAL_UART_Receive(&huart2, &rx_char, 1, 1) == HAL_OK)
  {
    if (rx_char == '\r' || rx_char == '\n')
    {
      if (uart_rx_index > 0)
      {
        uart_rx_buffer[uart_rx_index] = '\0';
        Process_Command(uart_rx_buffer);
        uart_rx_index = 0;
        memset(uart_rx_buffer, 0, sizeof(uart_rx_buffer));
      }
    }
    else
    {
      if (uart_rx_index < UART_RX_BUFFER_SIZE - 1)
      {
        uart_rx_buffer[uart_rx_index++] = (char)rx_char;
      }
      else
      {
        uart_rx_index = 0;
        memset(uart_rx_buffer, 0, sizeof(uart_rx_buffer));
        UART_Print("\r\nBuffer cheio. Digite o comando novamente.\r\n");
      }
    }
  }
}

static void Display_UpdateTask(void)
{
  if (current_mode == DISPLAY_NONE)
  {
    return;
  }

  if (HAL_GetTick() - last_toggle_time >= 500)
  {
    last_toggle_time = HAL_GetTick();
    toggle_state = !toggle_state;

    if (toggle_state == 0)
    {
      if (current_mode == DISPLAY_TEMP)
      {
        MAX7219_DisplayBitmap(CHAR_T);
      }
      else if (current_mode == DISPLAY_VOLT)
      {
        MAX7219_DisplayBitmap(CHAR_V);
      }
      else if (current_mode == DISPLAY_LDR)
      {
        MAX7219_DisplayBitmap(CHAR_L);
      }
    }
    else
    {
      if (current_sensor_value < 128)
      {
        MAX7219_DisplayBitmap(CHAR_MINUS);
      }
      else
      {
        MAX7219_DisplayBitmap(CHAR_PLUS);
      }
    }
  }
}

/* USER CODE END 0 */

int main(void)
{
  HAL_Init();

  SystemClock_Config();

  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  MX_SPI1_Init();

  /* USER CODE BEGIN 2 */

  HAL_GPIO_WritePin(MAX7219_CS_GPIO_Port, MAX7219_CS_Pin, GPIO_PIN_SET);

  MAX7219_Init();

  UART_Print("\r\n========================================\r\n");
  UART_Print("NUCLEO-L476RG + PCF8591 + MAX7219\r\n");
  UART_Print("Caso de teste iniciado\r\n");
  UART_Print("Baud: 115200\r\n");
  UART_Print("Digite um comando e pressione ENTER\r\n");
  UART_Print("Mapeamento corrigido:\r\n");
  UART_Print("  Temp -> AIN1\r\n");
  UART_Print("  Volt -> AIN3\r\n");
  UART_Print("  LDR  -> AIN0\r\n");
  UART_Print("Comandos:\r\n");
  UART_Print("  Temp\r\n");
  UART_Print("  Volt\r\n");
  UART_Print("  LDR\r\n");
  UART_Print("  Clear\r\n");
  UART_Print("  Test\r\n");
  UART_Print("========================================\r\n\r\n");

  MAX7219_TestPattern();
  HAL_Delay(1000);
  MAX7219_Clear();

  HAL_UART_Receive_IT(&huart2, &uart_rx_byte, 1);

  /* USER CODE END 2 */

  while (1)
  {
    if (uart_cmd_ready)
    {
      uart_cmd_ready = 0;

      Process_Command(uart_rx_buffer);

      memset(uart_rx_buffer, 0, sizeof(uart_rx_buffer));
    }

    Display_UpdateTask();
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

static void MX_SPI1_Init(void)
{
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;

  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
}

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

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  HAL_GPIO_WritePin(MAX7219_CS_GPIO_Port, MAX7219_CS_Pin, GPIO_PIN_SET);

  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = MAX7219_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(MAX7219_CS_GPIO_Port, &GPIO_InitStruct);
}

/* USER CODE BEGIN 4 */

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART2)
  {
    if ((uart_rx_byte == '\r') || (uart_rx_byte == '\n'))
    {
      if ((uart_rx_index > 0) && (uart_cmd_ready == 0))
      {
        uart_rx_buffer[uart_rx_index] = '\0';
        uart_cmd_ready = 1;
        uart_rx_index = 0;
      }
    }
    else if (uart_cmd_ready == 0)
    {
      if (uart_rx_index < (UART_RX_BUFFER_SIZE - 1))
      {
        uart_rx_buffer[uart_rx_index++] = (char)uart_rx_byte;
      }
      else
      {
        uart_rx_index = 0;
        memset(uart_rx_buffer, 0, sizeof(uart_rx_buffer));
        UART_Print("\r\nBuffer cheio. Digite novamente.\r\n");
      }
    }

    HAL_UART_Receive_IT(&huart2, &uart_rx_byte, 1);
  }
}

void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
  if (hi2c->Instance == I2C1)
  {
    if ((pending_pcf_operation == PCF_OP_READ_DIRECT) ||
        (pending_pcf_operation == PCF_OP_READ_DISPLAY))
    {
      if (HAL_I2C_Master_Receive_IT(&hi2c1, PCF8591_ADDR, i2c_rx_buffer, 2) != HAL_OK)
      {
        pending_pcf_operation = PCF_OP_NONE;
        pending_display_mode = DISPLAY_NONE;
        UART_Print("Erro ao iniciar leitura I2C\r\n");
      }
    }
  }
}

void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
  if (hi2c->Instance == I2C1)
  {
    if ((pending_pcf_operation == PCF_OP_READ_DIRECT) ||
        (pending_pcf_operation == PCF_OP_READ_DISPLAY))
    {
      uint8_t value = i2c_rx_buffer[1];
      DisplayMode_t display_mode = (DisplayMode_t)pending_display_mode;
      const char *channel_name = PCF8591_ChannelName((uint8_t)current_channel);

      pending_pcf_operation = PCF_OP_NONE;
      pending_display_mode = DISPLAY_NONE;

      if (display_mode == DISPLAY_NONE)
      {
        UART_Printf("%s: %u\r\n", channel_name, value);
      }
      else
      {
        current_sensor_value = value;
        current_mode = display_mode;
        toggle_state = 1;
        last_toggle_time = HAL_GetTick() - 500U;

        if (display_mode == DISPLAY_TEMP)
        {
          UART_Printf("Temperatura/AIN1 = %u\r\n", value);
          UART_Printf("Matriz: T alternando com %c\r\n", (value < 128U) ? '-' : '+');
        }
        else if (display_mode == DISPLAY_VOLT)
        {
          UART_Printf("Tensao/AIN3 = %u\r\n", value);
          UART_Printf("Matriz: V alternando com %c\r\n", (value < 128U) ? '-' : '+');
        }
        else if (display_mode == DISPLAY_LDR)
        {
          UART_Printf("Luminosidade/AIN0 = %u\r\n", value);
          UART_Printf("Matriz: L alternando com %c\r\n", (value < 128U) ? '-' : '+');
        }
      }
    }
  }
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
  if (hi2c->Instance == I2C1)
  {
    pending_pcf_operation = PCF_OP_NONE;
    pending_display_mode = DISPLAY_NONE;

    UART_Printf("Erro I2C. Codigo HAL: %lu\r\n", HAL_I2C_GetError(&hi2c1));
  }
}

/* USER CODE END 4 */

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
