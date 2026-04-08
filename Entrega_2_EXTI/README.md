📘 README — LED com EXTI (Controle de Frequência)
🔷 Controle de Cadência de LED com EXTI (STM32)
🎯 Objetivo

Desenvolver um sistema embarcado utilizando a placa NUCLEO-L476RG e o periférico EXTI (External Interrupt), onde um LED pisca com frequência variável controlada por um botão.

🧠 Funcionamento
O LED inicia piscando em 1 Hz
Ao pressionar o botão azul:
uma interrupção EXTI é gerada
a frequência muda para 2 Hz
Cada clique alterna entre:
1 Hz ↔ 2 Hz
🛠️ Tecnologias
STM32CubeIDE
STM32 HAL
Linguagem C
Placa: NUCLEO-L476RG
⚙️ Configuração do Projeto
1️⃣ Criar o projeto
File → New → STM32 Project
Board Selector → NUCLEO-L476RG
2️⃣ Configurar os pinos (CubeMX)
Componente	Pino	Configuração
LED (LD2)	PA5	GPIO_Output
Botão (B1)	PC13	GPIO_EXTI13
3️⃣ Configurar GPIO
System Core → GPIO
PC13 (Botão)
Mode: External Interrupt (Falling Edge)
Pull: No Pull
PA5 (LED)
Mode: Output Push Pull
Speed: Low
4️⃣ Habilitar interrupção
System Core → NVIC

Ativar:

EXTI line[15:10] interrupts
5️⃣ Gerar código
Project → Generate Code
💻 Implementação
🔁 Loop principal
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
    if (blink_delay == 500)
      blink_delay = 250;
    else
      blink_delay = 500;
  }
}
⏱️ Lógica de Frequência
Frequência	Delay
1 Hz	500 ms
2 Hz	250 ms
🧪 Resultado Esperado
LED começa em 1 Hz
Ao pressionar botão:
muda para 2 Hz
Pressionar novamente:
volta para 1 Hz
⚠️ Problemas comuns
Problema	Solução
Botão não funciona	Verificar EXTI
LED não pisca	Verificar PA5
Não muda frequência	Conferir callback
🚀 Melhorias futuras
Debounce mais robusto
Uso de Timer ao invés de delay
Controle via UART