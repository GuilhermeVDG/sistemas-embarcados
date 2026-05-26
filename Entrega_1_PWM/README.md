# Lista 1 - PWM por Software em GPIO

> Sistemas Embarcados - UPE/POLI
> Implementacao de PWM por software na placa **NUCLEO-L476RG**, usando o LED LD2 conectado ao pino **PA5**.

## Sumario

- [Visao Geral](#visao-geral)
- [Arquitetura do Sistema](#arquitetura-do-sistema)
- [Hardware Utilizado](#hardware-utilizado)
- [Pinagem](#pinagem)
- [Configuracao dos Perifericos](#configuracao-dos-perifericos)
- [Fluxo de Execucao](#fluxo-de-execucao)
- [Implementacao](#implementacao)
- [Como Compilar e Gravar](#como-compilar-e-gravar)
- [Como Testar](#como-testar)
- [Estrutura dos Diretorios](#estrutura-dos-diretorios)
- [Tecnologias e Ferramentas](#tecnologias-e-ferramentas)
- [Criterios de Avaliacao Atendidos](#criterios-de-avaliacao-atendidos)

---

## Visao Geral

Este projeto demonstra a geracao de um sinal PWM (*Pulse Width Modulation*) por software usando apenas GPIO e `HAL_Delay()`. A saida digital alterna entre nivel alto e baixo no LED da placa, simulando controle de brilho por meio do duty cycle.

### Objetivos tecnicos atendidos

- Controle direto de GPIO como saida digital.
- Geracao de PWM por software.
- Calculo de periodo, tempo ligado e tempo desligado.
- Parametrizacao de frequencia e duty cycle por `#define`.
- Protecao simples contra frequencia zero e duty cycle acima de 100%.

---

## Arquitetura do Sistema

```text
+----------------------+        GPIO PA5        +------------------+
|                      |----------------------->|                  |
|  NUCLEO-L476RG       |                        |  LED LD2         |
|  STM32L476RG         |<--- alimentacao USB --->|  LED da placa    |
|                      |                        |                  |
+----------------------+                        +------------------+
        |
        | Firmware
        v
+----------------------+
| software_pwm()       |
| 1. calcula periodo   |
| 2. liga LED          |
| 3. espera on_time    |
| 4. desliga LED       |
| 5. espera off_time   |
+----------------------+
```

---

## Hardware Utilizado

| Componente | Descricao |
|---|---|
| NUCLEO-L476RG | Placa STM32 usada na entrega |
| LED LD2 | LED verde onboard da NUCLEO |
| Cabo USB | Alimentacao, gravacao e debug via ST-LINK |

---

## Pinagem

| Funcao | Pino MCU | Label da placa | Configuracao |
|---|---|---|---|
| LED LD2 | PA5 | D13 / LD2 | GPIO Output Push-Pull |

---

## Configuracao dos Perifericos

| Periferico | Configuracao |
|---|---|
| System Clock | PLL via HSI, configurado pelo CubeMX |
| GPIOA | Clock habilitado |
| PA5 | Saida digital, push-pull, sem pull-up/pull-down, baixa velocidade |

---

## Fluxo de Execucao

```text
+---------------------------------------------------------------+
| 1. HAL_Init()                                                  |
| 2. SystemClock_Config()                                        |
| 3. MX_GPIO_Init() configura PA5 como saida                     |
| 4. Loop infinito                                               |
|      +-> chama software_pwm(20 Hz, 50%)                        |
|      +-> calcula periodo = 1000 / 20 = 50 ms                  |
|      +-> calcula on_time = 25 ms e off_time = 25 ms           |
|      +-> liga LD2, espera, desliga LD2, espera                 |
+---------------------------------------------------------------+
```

---

## Implementacao

### Parametros principais

```c
#define PWM_FREQUENCY_HZ    20
#define DUTY_CYCLE_PERCENT  50

#define LED_GPIO_PORT       GPIOA
#define LED_GPIO_PIN        GPIO_PIN_5
```

Com esses valores, o LED opera com periodo de **50 ms**, ficando **25 ms ligado** e **25 ms desligado**.

### Formula usada

```c
period_ms = 1000 / frequency;
on_time   = (period_ms * duty_cycle) / 100;
off_time  = period_ms - on_time;
```

### Funcao PWM

```c
void software_pwm(uint16_t frequency, uint8_t duty_cycle)
{
  if (frequency == 0)
  {
    return;
  }

  if (duty_cycle > 100)
  {
    duty_cycle = 100;
  }

  period_ms = 1000 / frequency;
  on_time = (period_ms * duty_cycle) / 100;
  off_time = period_ms - on_time;

  HAL_GPIO_WritePin(LED_GPIO_PORT, LED_GPIO_PIN, GPIO_PIN_SET);
  HAL_Delay(on_time);

  HAL_GPIO_WritePin(LED_GPIO_PORT, LED_GPIO_PIN, GPIO_PIN_RESET);
  HAL_Delay(off_time);
}
```

### Loop principal

```c
while (1)
{
  software_pwm(PWM_FREQUENCY_HZ, DUTY_CYCLE_PERCENT);
}
```

---

## Como Compilar e Gravar

### Pre-requisitos

- STM32CubeIDE.
- Drivers ST-LINK instalados.
- Cabo USB conectado na NUCLEO-L476RG.

### Passo a passo

```text
1. Abrir o STM32CubeIDE.
2. File -> Open Projects from File System.
3. Selecionar a pasta Entrega_1_PWM.
4. Project -> Build Project.
5. Conectar a NUCLEO-L476RG via USB.
6. Run -> Debug As -> STM32 C/C++ Application.
7. Apos gravar, pressionar Resume ou encerrar o debug.
```

---

## Como Testar

### Teste funcional

1. Grave o firmware na placa.
2. Observe o LED LD2.
3. O LED deve piscar rapidamente com duty cycle de 50%.

### Casos de teste sugeridos

| Caso | Alteracao | Resultado esperado |
|---|---|---|
| Duty 10% | `DUTY_CYCLE_PERCENT 10` | Flashes curtos, LED mais apagado |
| Duty 50% | `DUTY_CYCLE_PERCENT 50` | Tempo ligado igual ao desligado |
| Duty 80% | `DUTY_CYCLE_PERCENT 80` | LED quase sempre ligado |
| 2 Hz | `PWM_FREQUENCY_HZ 2` | Piscada visivel e lenta |
| 20 Hz | `PWM_FREQUENCY_HZ 20` | Brilho aparentemente continuo |

---

## Estrutura dos Diretorios

```text
Entrega_1_PWM/
+-- Core/
|   +-- Inc/                 # Headers do projeto
|   +-- Src/                 # main.c e arquivos HAL gerados
+-- Drivers/                 # HAL e CMSIS
+-- Debug/                   # Saida de build
+-- Entrega_1_PWM.ioc        # Configuracao do STM32CubeMX
+-- STM32L476RGTX_FLASH.ld   # Linker script para Flash
+-- README.md
```

---

## Tecnologias e Ferramentas

- **Linguagem:** C.
- **IDE:** STM32CubeIDE.
- **Configurador:** STM32CubeMX.
- **HAL:** STM32Cube HAL.
- **Toolchain:** GNU Arm Embedded Toolchain.
- **Debugger:** ST-LINK.

---

## Criterios de Avaliacao Atendidos

- [x] Projeto configurado para NUCLEO-L476RG.
- [x] LED LD2 controlado por GPIO.
- [x] PWM implementado por software.
- [x] Frequencia configuravel por `PWM_FREQUENCY_HZ`.
- [x] Duty cycle configuravel por `DUTY_CYCLE_PERCENT`.
- [x] Codigo simples e direto para demonstracao do conceito.

---

## Limitacoes e Melhorias Futuras

- O uso de `HAL_Delay()` bloqueia a CPU durante o PWM.
- A precisao depende do SysTick e do tempo gasto no loop.
- Uma versao mais robusta poderia usar Timer PWM por hardware.
- Tambem seria possivel controlar o duty cycle por botao, UART ou potenciometro.

---

## Licenca

Projeto desenvolvido para fins academicos no ambito da disciplina de Sistemas Embarcados da UPE/POLI.
