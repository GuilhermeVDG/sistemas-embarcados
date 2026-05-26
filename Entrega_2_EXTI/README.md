# Lista 2 - Controle de Cadencia com EXTI

> Sistemas Embarcados - UPE/POLI
> Implementacao de controle de frequencia do LED da **NUCLEO-L476RG** usando o botao azul da placa e interrupcao externa **EXTI**.

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

Este projeto usa uma interrupcao externa para alternar a frequencia de piscagem do LED LD2. O LED inicia piscando em **1 Hz** e, a cada pressionamento valido do botao azul, alterna entre **1 Hz** e **2 Hz**.

### Objetivos tecnicos atendidos

- Configuracao de GPIO como saida para o LED LD2.
- Configuracao do botao B1 como entrada com interrupcao EXTI.
- Uso do callback `HAL_GPIO_EXTI_Callback()`.
- Alternancia de estado por interrupcao.
- Debounce simples usando `HAL_GetTick()`.
- Uso da NVIC para habilitar `EXTI15_10_IRQn`.

---

## Arquitetura do Sistema

```text
+----------------------+       PC13 / EXTI       +------------------+
|                      |<------------------------|                  |
|  NUCLEO-L476RG       |                         |  Botao B1        |
|  STM32L476RG         |                         |  USER azul       |
|                      |                         |                  |
|              PA5 LD2 |------------------------>|  LED onboard     |
+----------------------+                         +------------------+
        |
        v
+----------------------+
| blink_delay          |
| 500 ms -> 1 Hz       |
| 250 ms -> 2 Hz       |
+----------------------+
```

---

## Hardware Utilizado

| Componente | Descricao |
|---|---|
| NUCLEO-L476RG | Placa STM32 usada na entrega |
| LED LD2 | LED onboard ligado ao PA5 |
| Botao B1 | Botao azul da placa ligado ao PC13 |
| Cabo USB | Alimentacao, gravacao e debug via ST-LINK |

---

## Pinagem

| Funcao | Pino MCU | Label da placa | Configuracao |
|---|---|---|---|
| LED LD2 | PA5 | D13 / LD2 | GPIO Output Push-Pull |
| Botao B1 | PC13 | USER / B1 | GPIO EXTI Falling Edge |
| USART2 TX | PA2 | ST-LINK VCP | Configurada pelo projeto |
| USART2 RX | PA3 | ST-LINK VCP | Configurada pelo projeto |

---

## Configuracao dos Perifericos

| Periferico | Configuracao |
|---|---|
| System Clock | PLL via HSI, configurado pelo CubeMX |
| GPIO PA5 | Saida push-pull, baixa velocidade |
| GPIO PC13 | External Interrupt, borda de descida |
| USART2 | 115200 8N1, mantida configurada no projeto |
| NVIC | `EXTI15_10_IRQn` habilitada |

---

## Fluxo de Execucao

```text
+--------------------------------------------------------------+
| 1. Sistema inicializa GPIO, USART2 e NVIC                    |
| 2. LED comeca alternando a cada 500 ms                       |
| 3. Usuario pressiona o botao B1                              |
| 4. PC13 gera interrupcao EXTI                                |
| 5. HAL chama HAL_GPIO_EXTI_Callback(GPIO_PIN_13)             |
| 6. Callback aplica debounce de 200 ms                        |
| 7. blink_delay alterna: 500 ms <-> 250 ms                    |
| 8. Loop principal continua piscando o LED com o novo delay   |
+--------------------------------------------------------------+
```

---

## Implementacao

### Variaveis principais

```c
volatile uint32_t blink_delay = 500;
uint32_t last_interrupt_time = 0;
```

### Loop principal

```c
while (1)
{
  HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
  HAL_Delay(blink_delay);
}
```

### Callback EXTI

```c
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == GPIO_PIN_13)
  {
    uint32_t current_time = HAL_GetTick();

    if ((current_time - last_interrupt_time) > 200)
    {
      if (blink_delay == 500)
      {
        blink_delay = 250;
      }
      else
      {
        blink_delay = 500;
      }

      last_interrupt_time = current_time;
    }
  }
}
```

### Logica de frequencia

| Estado | Delay usado | Frequencia percebida |
|---|---:|---:|
| Inicial | 500 ms | 1 Hz |
| Apos clique | 250 ms | 2 Hz |
| Clique seguinte | 500 ms | 1 Hz |

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
3. Selecionar a pasta Entrega_2_EXTI.
4. Project -> Build Project.
5. Conectar a NUCLEO-L476RG via USB.
6. Run -> Debug As -> STM32 C/C++ Application.
7. Apos gravar, pressionar Resume ou encerrar o debug.
```

---

## Como Testar

### Teste funcional

1. Grave o firmware na placa.
2. Observe o LED LD2 piscando em 1 Hz.
3. Pressione o botao azul B1.
4. O LED deve passar a piscar em 2 Hz.
5. Pressione novamente.
6. O LED deve voltar para 1 Hz.

### Problemas comuns

| Problema | Possivel causa | Verificacao |
|---|---|---|
| LED nao pisca | PA5 nao configurado como saida | Conferir `MX_GPIO_Init()` |
| Botao nao altera frequencia | EXTI/NVIC nao habilitada | Conferir `EXTI15_10_IRQn` |
| Muda varias vezes em um clique | Bounce mecanico | Conferir debounce de 200 ms |
| Frequencia parece errada | Delay bloqueante no loop | Medir o periodo completo ligado/desligado |

---

## Estrutura dos Diretorios

```text
Entrega_2_EXTI/
+-- Core/
|   +-- Inc/                 # Headers do projeto
|   +-- Src/                 # main.c, interrupcoes e HAL MSP
+-- Drivers/                 # HAL e CMSIS
+-- Debug/                   # Saida de build
+-- Entrega_2_EXTI.ioc       # Configuracao do STM32CubeMX
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
- [x] Botao B1 configurado como interrupcao externa.
- [x] Callback `HAL_GPIO_EXTI_Callback()` implementado.
- [x] Alternancia entre duas frequencias de piscagem.
- [x] Debounce simples implementado por software.
- [x] `EXTI15_10_IRQn` habilitada na NVIC.

---

## Limitacoes e Melhorias Futuras

- O loop ainda usa `HAL_Delay()`, portanto a CPU fica bloqueada entre alternancias.
- O debounce e simples e baseado apenas em tempo.
- Uma versao mais robusta poderia usar Timer para piscar o LED sem bloqueio.
- Tambem seria possivel enviar o estado atual pela UART.

---

## Licenca

Projeto desenvolvido para fins academicos no ambito da disciplina de Sistemas Embarcados da UPE/POLI.
