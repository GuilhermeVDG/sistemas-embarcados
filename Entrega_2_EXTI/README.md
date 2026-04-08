📘 README – Controle de Cadência de LED com EXTI (STM32)
🎯 Objetivo

Desenvolver um sistema embarcado utilizando a placa NUCLEO-L476RG e o periférico EXTI (External Interrupt), onde um LED pisca com frequência variável controlada por um botão.

🧠 Descrição do Funcionamento
O sistema inicia com o LED piscando a 1 Hz (1 vez por segundo)
Ao pressionar o botão azul:
uma interrupção EXTI é gerada
a frequência muda para 2 Hz
Cada novo clique alterna entre:
1 Hz ↔ 2 Hz
🧰 Tecnologias Utilizadas
STM32CubeIDE
STM32 HAL
Linguagem C
Placa: NUCLEO-L476RG
⚙️ Configuração do Projeto
1. Criar o Projeto
Abrir STM32CubeIDE

Ir em:

File → New → STM32 Project
Selecionar:
Aba: Board Selector
Buscar: NUCLEO-L476RG
Next → Nome do projeto → Finish
2. Configuração dos Pinos (CubeMX)

Abrir o arquivo .ioc

🔵 LED (LD2)
Pino: PA5

Configuração:

GPIO_Output
🔘 Botão (B1)
Pino: PC13

Configuração:

GPIO_EXTI13
3. Configurar GPIO

Ir em:

System Core → GPIO
PC13 (Botão)
Mode: External Interrupt Mode with Falling Edge
Pull: No Pull
PA5 (LED)
Mode: Output Push Pull
Speed: Low
4. Habilitar Interrupção (NVIC)

Ir em:

System Core → NVIC

Ativar:

EXTI line[15:10] interrupts
5. Gerar Código

Clicar em:

Project → Generate Code
💻 Implementação
📌 Variáveis Globais
volatile uint32_t blink_delay = 500;
uint32_t last_interrupt_time = 0;
🔁 Loop Principal
while (1)
{
  HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
  HAL_Delay(blink_delay);
}
⚡ Interrupção EXTI
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == GPIO_PIN_13)
  {
    uint32_t current_time = HAL_GetTick();

    if ((current_time - last_interrupt_time) > 200)
    {
      if (blink_delay == 500)
        blink_delay = 250;
      else
        blink_delay = 500;

      last_interrupt_time = current_time;
    }
  }
}
⏱️ Lógica de Frequência
Frequência	Delay
1 Hz	500 ms
2 Hz	250 ms
🧪 Execução
Conectar a placa via USB

Compilar:

Project → Build Project

Gravar:

Run → Run As → STM32 Application
✅ Resultado Esperado
LED inicia piscando em 1 Hz
Ao pressionar o botão:
alterna para 2 Hz
Ao pressionar novamente:
retorna para 1 Hz
⚠️ Problemas Comuns
❌ LED não muda frequência
Verificar se PC13 está como EXTI
Verificar NVIC habilitado
❌ Botão não responde
Verificar se está usando GPIO_PIN_13
❌ Erro de compilação
Não duplicar funções
Não apagar código gerado pelo Cube
🧠 Observações
Foi utilizado debounce via software (200 ms)
A variável volatile garante atualização correta em interrupções
O controle de frequência é feito via delay no loop principal
🚀 Possíveis Melhorias
Substituir HAL_Delay por Timer (TIM)
Usar RTOS (FreeRTOS)
Adicionar mais níveis de frequência
Controle via UART