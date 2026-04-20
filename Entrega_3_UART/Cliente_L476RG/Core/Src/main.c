/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

// Estados da maquina de estados do cliente
typedef enum {
    STATE_IDLE,           // Aguardando botao
    STATE_WAIT_COUNTER,   // Esperando contador do servidor
    STATE_WAIT_TABLE,     // Esperando tabela do servidor
    STATE_BLINKING,       // Piscando LED
    STATE_SEND_MSG_PC,    // Enviando "Numero de eventos = XXX" ao PC
    STATE_SEND_TABLE_PC   // Enviando tabela ao PC via DMA
} client_state_t;

volatile client_state_t estado = STATE_IDLE;

#define TABELA_SIZE 128         // Tamanho fixo acordado entre cliente e servidor

uint8_t cmd_tx = 0x5A;          // Comando enviado ao servidor
uint8_t contador_rx = 0;        // Contador recebido do servidor
uint8_t tabela_rx[TABELA_SIZE]; // Buffer para tabela recebida via DMA
char    msg_pc[64];             // Buffer pra mensagem ao PC

volatile uint8_t tabela_rx_done = 0;  // Flag: tabela recebida via DMA
volatile uint8_t msg_pc_done    = 0;  // Flag: msg ao PC enviada

// Controle do blink
uint32_t ultimo_toggle = 0;
uint8_t  toggles_restantes = 0;  // Cada pisca = 2 toggles (ON e OFF)

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// Callback da interrupcao externa (botao B1 - PC13)
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == BTN_USER_Pin) {
        if (estado == STATE_IDLE) {
            // Envia 0x5A ao servidor via USART1
            HAL_UART_Transmit_IT(&huart1, &cmd_tx, 1);
            // Ja arma recepcao do contador (1 byte)
            HAL_UART_Receive_IT(&huart1, &contador_rx, 1);
            estado = STATE_WAIT_COUNTER;
        }
        // Se nao esta IDLE, ignora (bloqueio durante blink/processamento)
    }
}

// Callback de UART RX completa
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        if (estado == STATE_WAIT_COUNTER) {
            // Recebeu o contador. Arma recepcao da tabela via DMA
            HAL_UART_Receive_DMA(&huart1, tabela_rx, TABELA_SIZE);
            estado = STATE_WAIT_TABLE;
        }
        else if (estado == STATE_WAIT_TABLE) {
            // Tabela recebida via DMA
            tabela_rx_done = 1;
        }
    }
}

// Callback de UART TX completa
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2) {
        if (estado == STATE_SEND_MSG_PC) {
            msg_pc_done = 1;
        }
        else if (estado == STATE_SEND_TABLE_PC) {
            // Terminou de enviar tabela ao PC → volta ao idle
            estado = STATE_IDLE;
        }
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

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  const char *banner = "Cliente L476RG pronto. Aperte o botao azul.\r\n";
  HAL_UART_Transmit(&huart2, (uint8_t*)banner, strlen(banner), HAL_MAX_DELAY);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

	  while (1)
	    {
	      /* USER CODE END WHILE */

	      /* USER CODE BEGIN 3 */

	      switch (estado) {

	          case STATE_WAIT_TABLE:
	              if (tabela_rx_done) {
	                  tabela_rx_done = 0;
	                  // Setup para piscar LED: contador_rx vezes, 1Hz
	                  toggles_restantes = contador_rx * 2;
	                  ultimo_toggle = HAL_GetTick();
	                  HAL_GPIO_WritePin(LED_USER_GPIO_Port, LED_USER_Pin, GPIO_PIN_SET);
	                  estado = STATE_BLINKING;
	              }
	              break;

	          case STATE_BLINKING:
	              if (toggles_restantes > 0) {
	                  if (HAL_GetTick() - ultimo_toggle >= 500) {
	                      HAL_GPIO_TogglePin(LED_USER_GPIO_Port, LED_USER_Pin);
	                      ultimo_toggle = HAL_GetTick();
	                      toggles_restantes--;
	                  }
	              } else {
	                  // Garante LED apagado
	                  HAL_GPIO_WritePin(LED_USER_GPIO_Port, LED_USER_Pin, GPIO_PIN_RESET);
	                  // Formata mensagem pro PC
	                  int n = snprintf(msg_pc, sizeof(msg_pc),
	                                   "Numero de eventos = %u\r\n", contador_rx);
	                  estado = STATE_SEND_MSG_PC;
	                  msg_pc_done = 0;
	                  HAL_UART_Transmit_IT(&huart2, (uint8_t*)msg_pc, n);
	              }
	              break;

	          case STATE_SEND_MSG_PC:
	              if (msg_pc_done) {
	                  msg_pc_done = 0;
	                  estado = STATE_SEND_TABLE_PC;
	                  // Envia tabela inteira ao PC via DMA
	                  HAL_UART_Transmit_DMA(&huart2, tabela_rx, TABELA_SIZE);
	              }
	              break;

	          default:
	              // IDLE, WAIT_COUNTER, SEND_TABLE_PC nao precisam polling aqui
	              break;
	      }
	    }
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

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
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

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
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
  /* User can add his own implementation to report the HAL error return state */
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
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
