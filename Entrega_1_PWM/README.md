📘 README — PWM por Software
🔷 GPIO e PWM por Software (STM32)
🎯 Objetivo

Implementar um PWM por software utilizando GPIO para controlar o LED da placa NUCLEO-L476RG.

🧠 Conceito

PWM (Pulse Width Modulation) controla o tempo ligado/desligado de um sinal:

altera o brilho do LED
simula saída analógica
⚙️ Parâmetros
Frequência
freq = ciclos por segundo
Duty Cycle
% do tempo ligado
🧮 Fórmulas
period_ms = 1000 / frequency;
on_time = (period_ms * duty) / 100;
off_time = period_ms - on_time;
🔌 Hardware
Item	Valor
Placa	NUCLEO-L476RG
LED	LD2
Pino	PA5
⚙️ Configuração
PA5 → GPIO_Output
💻 Implementação
🔧 Defines
#define PWM_FREQUENCY_HZ    2
#define DUTY_CYCLE_PERCENT  25
⚡ Função PWM
void software_pwm(uint16_t frequency, uint8_t duty_cycle)
{
  uint32_t period = 1000 / frequency;
  uint32_t on = (period * duty_cycle) / 100;
  uint32_t off = period - on;

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
  HAL_Delay(on);

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
  HAL_Delay(off);
}
🔁 Loop principal
while (1)
{
  software_pwm(PWM_FREQUENCY_HZ, DUTY_CYCLE_PERCENT);
}
🧪 Casos de Teste
🔥 Caso 1 — 25%
Estado	Tempo
ON	125 ms
OFF	375 ms

👉 LED pisca rápido e fica mais apagado

🔥 Caso 2 — 80%
DUTY_CYCLE_PERCENT = 80

👉 LED quase sempre ligado

🔥 Caso 3 — 10%
DUTY_CYCLE_PERCENT = 10

👉 LED dá flashes rápidos

⚡ Caso 4 — Alta frequência
PWM_FREQUENCY_HZ = 20

👉 LED parece brilho contínuo

⚠️ Limitações
Usa HAL_Delay → bloqueia CPU
Baixa precisão
Não recomendado para sistemas reais
🚀 Melhorias
PWM por hardware (TIM)
Controle de múltiplos LEDs
Uso de RTOS
Ajuste via botão
🧠 Conclusão

O projeto demonstra claramente:

controle de sinal digital
impacto do duty cycle
comportamento visual do PWM