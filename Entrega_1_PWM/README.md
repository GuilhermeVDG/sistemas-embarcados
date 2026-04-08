README – GPIO e PWM por Software com STM32 NUCLEO-L476RG
Objetivo

Implementar um PWM por software usando GPIO na placa NUCLEO-L476RG, controlando o LED onboard da placa. O sinal PWM é gerado manualmente no código, sem usar periférico de PWM por hardware.

Descrição do projeto

Neste projeto, o LED onboard da placa é acionado por software com dois parâmetros principais:

Frequência
Duty Cycle

O funcionamento é:

o código calcula o período total do PWM em milissegundos
calcula quanto tempo o LED ficará ligado
calcula quanto tempo o LED ficará desligado
repete isso continuamente dentro do while(1)
Conceitos usados
PWM

PWM significa Pulse Width Modulation.

É uma técnica em que um sinal alterna entre ligado e desligado rapidamente, controlando:

o tempo em nível alto
o tempo em nível baixo

Isso permite simular, por exemplo:

brilho de LED
velocidade de motor
potência média entregue a uma carga
Frequência

A frequência define quantos ciclos completos acontecem por segundo.

Exemplo:

1 Hz → 1 ciclo por segundo
2 Hz → 2 ciclos por segundo
20 Hz → 20 ciclos por segundo

Fórmula:

periodo_ms = 1000 / frequencia
Duty Cycle

O duty cycle define a porcentagem do período em que o sinal fica ligado.

Exemplo:

50% → metade do tempo ligado, metade desligado
80% → quase sempre ligado
10% → quase sempre desligado

Fórmula:

tempo_on = (periodo_ms * duty_cycle) / 100
tempo_off = periodo_ms - tempo_on
Hardware utilizado
Placa: STM32 NUCLEO-L476RG
LED usado: LED onboard da placa
Pino do LED: PA5
Software utilizado
STM32CubeIDE
STM32 HAL
Linguagem C
Criação do projeto
1. Criar um novo projeto

Na STM32CubeIDE:

File > New > STM32 Project
2. Selecionar a placa

Na aba Board Selector, pesquisar:

NUCLEO-L476RG

Selecionar a placa e clicar em Next.

3. Nomear o projeto

Exemplo:

PWM_SOFTWARE_LED

Depois clicar em Finish.

Configuração inicial
1. Abrir o arquivo .ioc

No painel esquerdo, abrir o arquivo do projeto:

PWM_SOFTWARE_LED.ioc
2. Verificar o pino do LED

O LED onboard da NUCLEO-L476RG está ligado ao:

PA5

Se quiser usar a configuração gráfica, esse pino deve estar como:

GPIO_Output
Estratégia adotada

Neste projeto, foi usada uma solução simples e direta:

definir o pino do LED diretamente no código:
GPIOA
GPIO_PIN_5
gerar o PWM inteiramente por software usando:
HAL_GPIO_WritePin()
HAL_Delay()

Isso evita depender de aliases como LD2_Pin e LD2_GPIO_Port, que podem não existir em todos os projetos.

Estrutura da lógica

O projeto usa os seguintes defines:

#define PWM_FREQUENCY_HZ    2
#define DUTY_CYCLE_PERCENT  25

#define LED_GPIO_PORT       GPIOA
#define LED_GPIO_PIN        GPIO_PIN_5
Funcionamento da função software_pwm

A função recebe:

frequência
duty cycle

E executa os seguintes passos:

calcula o período total
calcula o tempo ligado
calcula o tempo desligado
liga o LED
espera o tempo ligado
desliga o LED
espera o tempo desligado

Trecho lógico:

period_ms = 1000 / frequency;
on_time = (period_ms * duty_cycle) / 100;
off_time = period_ms - on_time;
Lógica principal

Dentro do while(1), o programa chama continuamente:

software_pwm(PWM_FREQUENCY_HZ, DUTY_CYCLE_PERCENT);

Isso faz o LED repetir o padrão PWM continuamente.

Caso de teste principal

Configuração usada:

#define PWM_FREQUENCY_HZ    2
#define DUTY_CYCLE_PERCENT  25
Cálculo
período = 1000 / 2 = 500 ms
tempo ligado = 25% de 500 = 125 ms
tempo desligado = 375 ms
Resultado esperado

O LED:

acende rapidamente
fica apagado por mais tempo
repete esse padrão continuamente
Casos de teste adicionais
Caso 1 – LED quase sempre ligado
#define PWM_FREQUENCY_HZ    2
#define DUTY_CYCLE_PERCENT  80

Esperado:

LED parece quase sempre aceso
apenas pequenas pausas apagado
Caso 2 – LED quase sempre apagado
#define PWM_FREQUENCY_HZ    2
#define DUTY_CYCLE_PERCENT  10

Esperado:

LED dá flashes rápidos
permanece apagado na maior parte do tempo
Caso 3 – Frequência maior
#define PWM_FREQUENCY_HZ    20
#define DUTY_CYCLE_PERCENT  50

Esperado:

piscamento muito rápido
visualmente o LED parece com brilho contínuo/intermediário
Passos para compilação

Na STM32CubeIDE:

Project > Clean
Project > Build Project
Passos para gravação na placa

Conectar a placa via USB e executar:

Run > Run As > STM32 C/C++ Application
Resultado esperado

Após gravar o código na placa:

o LED onboard deve piscar conforme a frequência definida
a razão entre tempo ligado e desligado deve seguir o duty cycle configurado