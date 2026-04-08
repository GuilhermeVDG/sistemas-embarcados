x/* USER CODE BEGIN Includes */
/* USER CODE END Includes */

/* USER CODE BEGIN PD */
#define PWM_FREQUENCY_HZ     10      // Frequência do PWM em Hz
#define NUM_LEDS             3       // Número de LEDs controlados
/* USER CODE END PD */

/* USER CODE BEGIN PTD */
typedef struct {
    GPIO_TypeDef* port;
    uint16_t      pin;
    uint8_t       duty_cycle; // 0–100 (%)
} PWM_Channel_t;
/* USER CODE END PTD */

/* USER CODE BEGIN PV */
// Configuração de cada LED com seu próprio Duty Cycle
// NUCLEO-L476RG: LD2 = PA5 (onboard), extensões em PB14, PC7
PWM_Channel_t pwm_channels[NUM_LEDS] = {
    { GPIOA, GPIO_PIN_5,  25 },   // LED 1 – Duty Cycle 25%
    { GPIOB, GPIO_PIN_14, 50 },   // LED 2 – Duty Cycle 50%
    { GPIOC, GPIO_PIN_7,  75 },   // LED 3 – Duty Cycle 75%
};
/* USER CODE END PV */

/* USER CODE BEGIN 0 */

/**
 * @brief  Gera PWM por software para múltiplos canais (LEDs).
 *         Cada canal possui seu próprio Duty Cycle.
 *         Os canais são processados sequencialmente dentro do período.
 *
 * @param  frequency   Frequência do PWM em Hz (comum a todos os canais)
 * @param  channels    Array de canais PWM (GPIO + duty cycle)
 * @param  num_channels Número de canais no array
 */
void software_pwm_multi(uint16_t frequency,
                        PWM_Channel_t* channels,
                        uint8_t num_channels)
{
    // Período total dividido igualmente entre os canais
    uint32_t period_ms = 1000 / frequency;

    for (uint8_t i = 0; i < num_channels; i++)
    {
        // Garante duty cycle no intervalo válido
        uint8_t dc = channels[i].duty_cycle;
        if (dc > 100) dc = 100;

        uint32_t on_time  = (period_ms * dc) / 100;
        uint32_t off_time = period_ms - on_time;

        // Liga o LED durante o tempo ON
        HAL_GPIO_WritePin(channels[i].port, channels[i].pin, GPIO_PIN_SET);
        HAL_Delay(on_time);

        // Desliga o LED durante o tempo OFF
        HAL_GPIO_WritePin(channels[i].port, channels[i].pin, GPIO_PIN_RESET);
        HAL_Delay(off_time);
    }
}

/* USER CODE END 0 */

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    // Inicialização dos GPIOs (gerada pelo CubeMX ou manual)
    MX_GPIO_Init();

    while (1)
    {
        software_pwm_multi(PWM_FREQUENCY_HZ, pwm_channels, NUM_LEDS);
    }
}
