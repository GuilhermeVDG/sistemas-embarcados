# Lista 6 - Timers, PWM e DAC com PCF8591

> Sistemas Embarcados - UPE/POLI
> Integracao da **NUCLEO-L476RG** com o modulo **PCF8591** via I2C, usando **TIM2** para PWM, **TIM6** para cadencia do DAC, botao azul por EXTI e monitoramento serial via USART2.

## Sumario

- [Visao Geral](#visao-geral)
- [Arquitetura do Sistema](#arquitetura-do-sistema)
- [Hardware Utilizado](#hardware-utilizado)
- [Pinagem e Conexoes](#pinagem-e-conexoes)
- [Configuracao dos Perifericos](#configuracao-dos-perifericos)
- [Modos de Operacao](#modos-de-operacao)
- [Fluxo de Execucao](#fluxo-de-execucao)
- [Estrutura do Codigo](#estrutura-do-codigo)
- [Como Compilar e Gravar](#como-compilar-e-gravar)
- [Como Testar](#como-testar)
- [Estrutura dos Diretorios](#estrutura-dos-diretorios)
- [Tecnologias e Ferramentas](#tecnologias-e-ferramentas)
- [Criterios de Avaliacao Atendidos](#criterios-de-avaliacao-atendidos)

---

## Visao Geral

Este projeto implementa um sistema embarcado que integra leitura analogica, geracao de PWM, geracao de sinal analogico por DAC e interrupcoes por timer e botao.

A NUCLEO-L476RG se comunica com o PCF8591 por I2C para ler dois sinais analogicos:

- **LDR**, usado para controlar a amplitude do sinal gerado no DAC.
- **Potenciometro**, usado para controlar o duty cycle do PWM.

O sistema possui tres modos de operacao, alternados pelo botao azul da placa:

- **Modo 1:** controle PWM pelo potenciometro.
- **Modo 2:** geracao de sinal DAC com amplitude controlada pelo LDR.
- **Modo 3:** PWM e DAC funcionando simultaneamente.

O terminal serial via USART2 exibe os valores lidos e calculados em tempo real, incluindo status da comunicacao I2C e diagnostico dos quatro canais do PCF8591.

### Objetivos tecnicos atendidos

- Comunicacao I2C entre STM32 e PCF8591.
- Leitura dos canais analogicos do PCF8591.
- Mapeamento final do LDR em `CH0` e do potenciometro em `CH3`.
- Geracao de PWM com tres canais do `TIM2`.
- Controle do duty cycle do PWM pelo valor do potenciometro.
- Escrita no DAC do PCF8591.
- Geracao de forma de onda senoidal por tabela de pontos.
- Atualizacao do DAC por interrupcao periodica do `TIM6`.
- Alternancia de modos com botao azul em `PC13` por EXTI.
- Monitoramento dos valores via USART2 em `115200 8N1`.

---

## Arquitetura do Sistema

```text
+-------------------+        USART2 / USB        +-------------------+
|                   |<-------------------------->|                   |
|  PC / Terminal    |                            |  NUCLEO-L476RG    |
|  115200 8N1       |                            |  Controlador      |
+-------------------+                            +----+----+-----+---+
                                                     |    |     |
                                      I2C1 PB8/PB9   |    |     | TIM2 PWM
                                                     v    |     v
                                             +-------+--+ |  +---------+
                                             | PCF8591  | |  | PWM out |
                                             | ADC/DAC  | |  | PA0/PA1 |
                                             +----+-----+ |  | PB10    |
                                                  |       |  +---------+
                                                  |       |
                                                  v       |
                                             AOUT / DAC   |
                                                          |
                                             TIM6 interrupcao
                                             atualiza senoide
```

### Relacao entre entradas e saidas

```text
+----------------+----------------+-----------------------------+
| Entrada        | Origem         | Acao                        |
+----------------+----------------+-----------------------------+
| Potenciometro  | PCF8591 CH3    | Controla duty cycle do PWM  |
| LDR            | PCF8591 CH0    | Controla amplitude do DAC   |
| Botao azul     | PC13 / EXTI13  | Alterna os modos 1, 2 e 3   |
+----------------+----------------+-----------------------------+
```

---

## Hardware Utilizado

| Componente | Descricao | Uso no projeto |
|---|---|---|
| NUCLEO-L476RG | Placa STM32 principal | Controle geral do sistema |
| PCF8591 | Modulo ADC/DAC via I2C | Leitura dos sensores e saida DAC |
| Cabo USB | Alimentacao, gravacao e terminal serial | Conexao com o PC |
| Jumpers | Fios de conexao | Ligacao entre NUCLEO e PCF8591 |
| LED externo | Opcional | Visualizacao fisica do PWM |
| Resistor 220 ohm ou 330 ohm | Opcional | Limitacao de corrente do LED |

---

## Pinagem e Conexoes

### PCF8591 via I2C

| PCF8591 | NUCLEO-L476RG | Funcao |
|---|---|---|
| VCC | 3V3 | Alimentacao |
| GND | GND | Referencia comum |
| SCL | D15 / PB8 | I2C1_SCL |
| SDA | D14 / PB9 | I2C1_SDA |
| AOUT | Medicao externa | Saida analogica do DAC |

> Importante: alimente o PCF8591 com **3V3** para manter compatibilidade com os niveis logicos da NUCLEO-L476RG.

### Mapeamento final dos canais do PCF8591

Durante os testes, os quatro canais foram lidos para diagnostico. O mapeamento final usado no codigo foi:

| Sensor | Canal PCF8591 | Uso |
|---|---|---|
| LDR | `CH0 / AIN0` | Controle da amplitude do DAC |
| Potenciometro | `CH3 / AIN3` | Controle do duty cycle PWM |

```c
#define PCF8591_CH_LDR      0
#define PCF8591_CH_POT      3
```

### Saidas PWM do TIM2

| Pino | Funcao | Uso |
|---|---|---|
| PA0 | TIM2_CH1 | PWM canal 1 |
| PA1 | TIM2_CH2 | PWM canal 2 |
| PB10 | TIM2_CH3 | PWM canal 3 |

Para visualizar o PWM com LED externo:

```text
PA0 -> resistor -> anodo do LED
catodo do LED -> GND
```

### Botao azul

| Pino | Funcao |
|---|---|
| PC13 | GPIO_EXTI13 / B1 Blue PushButton |

O botao azul foi configurado como interrupcao externa em borda de descida.

### USART2

| Pino | Funcao |
|---|---|
| PA2 | USART2_TX |
| PA3 | USART2_RX |

Configuracao do terminal:

| Parametro | Valor |
|---|---|
| Baud rate | 115200 |
| Data bits | 8 |
| Parity | None |
| Stop bits | 1 |
| Flow control | None |

---

## Configuracao dos Perifericos

### I2C1

| Parametro | Valor |
|---|---|
| Periferico | I2C1 |
| Papel da NUCLEO | Master |
| Velocidade | Standard Mode |
| Frequencia | 100 kHz |
| SCL | PB8 / D15 |
| SDA | PB9 / D14 |
| Endereco PCF8591 | `0x48 << 1` |

Na HAL do STM32, o endereco I2C de 7 bits e usado deslocado para a esquerda:

```c
#define PCF8591_ADDR (0x48 << 1)
```

### TIM2 - PWM

| Parametro | Valor |
|---|---|
| Timer | TIM2 |
| Clock source | Internal Clock |
| Canal 1 | PWM Generation CH1 |
| Canal 2 | PWM Generation CH2 |
| Canal 3 | PWM Generation CH3 |
| Prescaler | 79 |
| Counter period | 999 |
| Counter mode | Up |
| Clock division | No Division |

Considerando clock do timer em 80 MHz:

```text
80 MHz / (79 + 1) = 1 MHz
1 MHz / (999 + 1) = 1 kHz
```

Assim, a frequencia aproximada do PWM e **1 kHz**, com duty cycle representado por valores de `0` a `999`.

### TIM6 - Temporizador da senoide

| Parametro | Valor |
|---|---|
| Timer | TIM6 |
| Prescaler | 7999 |
| Counter period | 999 |
| Counter mode | Up |
| Interrupcao | Habilitada |
| Frequencia de interrupcao | 10 Hz |
| Periodo | 100 ms |

Calculo:

```text
80 MHz / (7999 + 1) = 10 kHz
10 kHz / (999 + 1) = 10 Hz
```

Como a tabela senoidal possui 10 pontos, o DAC completa um ciclo por segundo:

```text
10 pontos / 10 pontos por segundo = 1 Hz
```

### GPIO / EXTI

| Parametro | Valor |
|---|---|
| Pino | PC13 |
| Modo | External Interrupt |
| Trigger | Falling Edge |
| NVIC | EXTI line[15:10] enabled |

### USART2

| Parametro | Valor |
|---|---|
| Uso | Terminal serial via ST-LINK/USB |
| Baud rate | 115200 |
| Word length | 8 bits |
| Parity | None |
| Stop bits | 1 |
| Modo | TX/RX |

---

## Modos de Operacao

### Modo 1 - Controle PWM

No Modo 1, apenas o PWM e controlado pelo potenciometro.

```text
Potenciometro -> PCF8591 CH3 -> I2C -> STM32 -> TIM2 PWM
```

O valor do potenciometro varia de `0` a `255` e e convertido para o intervalo do PWM:

```c
pwm_value = (pot_value * 999) / 255;
```

| POT | PWM aproximado |
|---|---|
| 0 | 0 |
| 128 | 501 |
| 255 | 999 |

Resultado esperado:

- POT varia.
- PWM varia proporcionalmente.
- DAC permanece em `0`.

### Modo 2 - Geracao de sinal DAC

No Modo 2, o sistema gera uma forma de onda senoidal na saida AOUT do PCF8591.

A senoide e representada por uma tabela de 10 pontos:

```c
const uint8_t sine_table[10] =
{
  128, 203, 249, 249, 203,
  128, 52, 6, 6, 52
};
```

A amplitude e ajustada pelo valor do LDR:

```c
dac_value_atual = (sine_value * ldr_value) / 255;
```

Resultado esperado:

- DAC varia periodicamente.
- LDR altera a amplitude da variacao do DAC.
- PWM fica desligado.

### Modo 3 - Operacao simultanea

No Modo 3, PWM e DAC funcionam ao mesmo tempo:

```text
Potenciometro -> PWM
LDR -> amplitude da senoide no DAC
```

Resultado esperado:

- POT varia e altera o PWM.
- LDR varia e altera a amplitude do DAC.
- DAC continua variando periodicamente pela interrupcao do TIM6.

---

## Fluxo de Execucao

```text
+--------------------------------------------------------------------------+
| 1. Sistema inicializa HAL, clock, GPIO, USART2, I2C1, TIM2 e TIM6        |
| 2. PWM e iniciado nos canais TIM2_CH1, TIM2_CH2 e TIM2_CH3               |
| 3. TIM6 e iniciado com interrupcao                                       |
| 4. Sistema inicia no Modo 1                                              |
| 5. Terminal serial exibe mensagem inicial                                |
| 6. Loop principal processa botao, sensores, PWM, DAC e UART              |
| 7. Os canais CH0, CH1, CH2 e CH3 do PCF8591 sao lidos para diagnostico   |
| 8. LDR e obtido de CH0 e potenciometro de CH3                            |
| 9. Valor do potenciometro e convertido para PWM                          |
| 10. TIM6 gera flag para atualizar o DAC a cada 100 ms                    |
| 11. Botao azul alterna os modos 1, 2 e 3                                 |
| 12. Terminal serial imprime os valores do sistema a cada 500 ms          |
+--------------------------------------------------------------------------+
```

---

## Estrutura do Codigo

### Modos de operacao

```c
typedef enum
{
  MODO_RGB = 1,
  MODO_DAC = 2,
  MODO_RGB_DAC = 3
} ModoOperacao;
```

### Leitura do PCF8591

A funcao seleciona o canal desejado, faz a leitura e descarta a primeira amostra retornada, pois o PCF8591 pode entregar o valor anterior apos a troca de canal.

```c
uint8_t PCF8591_ReadADC(uint8_t channel)
{
  uint8_t control_byte;
  uint8_t adc_value = 0;

  control_byte = 0x40 | (channel & 0x03);

  HAL_I2C_Master_Transmit(&hi2c1, PCF8591_ADDR, &control_byte, 1, I2C_TIMEOUT_MS);
  HAL_I2C_Master_Receive(&hi2c1, PCF8591_ADDR, &adc_value, 1, I2C_TIMEOUT_MS);
  HAL_I2C_Master_Receive(&hi2c1, PCF8591_ADDR, &adc_value, 1, I2C_TIMEOUT_MS);

  return adc_value;
}
```

### Escrita no DAC

A escrita envia dois bytes ao PCF8591:

```text
byte 0: 0x40, habilita a saida DAC
byte 1: valor analogico de 0 a 255
```

```c
uint8_t PCF8591_WriteDAC(uint8_t value)
{
  uint8_t data[2];

  data[0] = 0x40;
  data[1] = value;

  HAL_I2C_Master_Transmit(&hi2c1, PCF8591_ADDR, data, 2, I2C_TIMEOUT_MS);

  return 1;
}
```

### Conversao ADC para PWM

```c
uint16_t Converter_ADC_Para_PWM(uint8_t adc_value)
{
  return (uint16_t)(((uint32_t)adc_value * PWM_MAX_VALUE) / 255);
}
```

### Atualizacao do PWM

Os tres canais do TIM2 recebem o mesmo valor de compare:

```c
void Atualizar_PWM_RGB(uint16_t value)
{
  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, value);
  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, value);
  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, value);
}
```

### Interrupcao do botao azul

A interrupcao apenas ativa uma flag. A troca de modo e processada no loop principal.

```c
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == B1_Pin)
  {
    flag_botao_pressionado = 1;
  }
}
```

Sequencia de modos:

```text
Modo 1 -> Modo 2 -> Modo 3 -> Modo 1
```

### Interrupcao do TIM6

O callback do timer ativa a flag de atualizacao do DAC:

```c
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM6)
  {
    flag_timer_dac = 1;
  }
}
```

### Saida serial de diagnostico

O terminal imprime periodicamente:

```text
Modo:1 | POT:132 | LDR:218 | PWM: 517 | DAC:  0 | I2C:OK | CH0:218 CH1:000 CH2:000 CH3:132
```

---

## Como Compilar e Gravar

```text
1. Abrir o STM32CubeIDE.
2. File -> Open Projects from File System.
3. Selecionar a pasta Entrega_6_Timers.
4. Conferir se a configuracao esta em Debug.
5. Project -> Build Project.
6. Conectar a NUCLEO-L476RG ao computador via USB.
7. Run -> Debug As -> STM32 C/C++ Application.
8. Apos gravar, abrir o terminal serial na COM do ST-LINK.
```

---

## Como Testar

### Inicializacao

Ao resetar a placa, o terminal deve exibir:

```text
========================================
Projeto PCF8591 + PWM + DAC + Timer
Modo inicial: 1 - RGB/PWM
Botao azul alterna: 1 -> 2 -> 3 -> 1
UART: 115200 baud, 8N1
Mapeamento final: LDR=CH0 | POT=CH3
========================================
```

Depois, devem aparecer linhas semelhantes a:

```text
Modo:1 | POT:132 | LDR:218 | PWM: 517 | DAC:  0 | I2C:OK | CH0:218 CH1:000 CH2:000 CH3:132
```

### Casos de teste

| Caso | Procedimento | Resultado esperado |
|---|---|---|
| Comunicacao I2C | Conectar PCF8591 e observar o terminal | Campo `I2C` deve exibir `OK` |
| Modo 1 - PWM | Deixar no Modo 1 e girar o potenciometro | `POT` e `PWM` variam proporcionalmente |
| Modo 2 - DAC | Pressionar o botao azul uma vez | `DAC` varia periodicamente e PWM fica desligado |
| LDR no DAC | No Modo 2 ou 3, iluminar/cobrir o LDR | `LDR` varia e a faixa do `DAC` muda |
| Troca de modo | Pressionar o botao azul repetidamente | Terminal indica modos 1, 2 e 3 em sequencia |
| Modo 3 | Entrar no Modo 3, girar POT e variar luz no LDR | PWM acompanha POT e DAC acompanha LDR |
| Visualizacao PWM | Ligar LED em PA0 com resistor | Brilho varia com o potenciometro |
| Visualizacao DAC | Medir AOUT com osciloscopio ou multimetro | AOUT apresenta variacao periodica no Modo 2 ou 3 |

### Exemplos de saida

Modo 1:

```text
Modo:1 | POT: 40 | LDR:210 | PWM: 156 | DAC:  0 | I2C:OK | CH0:210 CH1:000 CH2:000 CH3: 40
Modo:1 | POT:128 | LDR:210 | PWM: 501 | DAC:  0 | I2C:OK | CH0:210 CH1:000 CH2:000 CH3:128
Modo:1 | POT:230 | LDR:210 | PWM: 901 | DAC:  0 | I2C:OK | CH0:210 CH1:000 CH2:000 CH3:230
```

Modo 2:

```text
Modo:2 | POT:230 | LDR:220 | PWM: 901 | DAC:110 | I2C:OK | CH0:220 CH1:000 CH2:000 CH3:230
Modo:2 | POT:230 | LDR:220 | PWM: 901 | DAC:175 | I2C:OK | CH0:220 CH1:000 CH2:000 CH3:230
Modo:2 | POT:230 | LDR:220 | PWM: 901 | DAC:214 | I2C:OK | CH0:220 CH1:000 CH2:000 CH3:230
```

Modo 3:

```text
Modo:3 | POT: 80 | LDR:200 | PWM: 313 | DAC:100 | I2C:OK | CH0:200 CH1:000 CH2:000 CH3: 80
Modo:3 | POT:160 | LDR:200 | PWM: 626 | DAC:159 | I2C:OK | CH0:200 CH1:000 CH2:000 CH3:160
Modo:3 | POT:230 | LDR: 70 | PWM: 901 | DAC: 35 | I2C:OK | CH0: 70 CH1:000 CH2:000 CH3:230
```

### Problemas comuns

| Problema | Possivel causa | Verificacao |
|---|---|---|
| Nada aparece no terminal | Porta COM errada | Usar a COM do ST-LINK |
| Caracteres estranhos | Baud rate incorreto | Usar `115200 8N1` |
| `I2C:ERRO` | SDA/SCL invertidos, sem GND comum ou endereco incorreto | Conferir PB8, PB9, GND e `0x48 << 1` |
| POT nao varia | Canal do potenciometro incorreto | Confirmar leitura em `CH3` |
| LDR nao varia | Canal do LDR incorreto ou pouca variacao de luz | Confirmar leitura em `CH0` |
| PWM parece fixo | LED ligado em pino errado ou potenciometro sem variacao | Testar PA0/PA1/PB10 e observar `PWM` no terminal |
| DAC fica em zero | Sistema esta no Modo 1 | Pressionar botao para entrar no Modo 2 ou 3 |

---

## Estrutura dos Diretorios

```text
Entrega_6_Timers/
+-- Core/
|   +-- Inc/                 # Headers do projeto
|   +-- Src/                 # main.c, callbacks e inicializacao
+-- Drivers/                 # HAL e CMSIS
+-- Debug/                   # Saida de build
+-- Entrega_6_Timers.ioc     # Configuracao do STM32CubeMX
+-- STM32L476RGTX_FLASH.ld   # Linker script para Flash
+-- README.md
```

---

## Tecnologias e Ferramentas

- **Linguagem:** C.
- **IDE:** STM32CubeIDE.
- **Configurador:** STM32CubeMX.
- **HAL:** STM32Cube HAL.
- **Comunicacao:** I2C e UART.
- **Temporizadores:** TIM2 para PWM e TIM6 para interrupcao periodica.
- **Terminal:** Tera Term, PuTTY ou similar.
- **Debugger:** ST-LINK.

---

## Criterios de Avaliacao Atendidos

- [x] NUCLEO-L476RG configurada como controladora principal.
- [x] PCF8591 integrado via I2C.
- [x] Potenciometro lido pelo PCF8591.
- [x] LDR lido pelo PCF8591.
- [x] PWM gerado com TIM2.
- [x] Duty cycle ajustado pelo potenciometro.
- [x] DAC do PCF8591 usado para gerar sinal analogico.
- [x] Senoide de 1 Hz implementada por tabela de pontos.
- [x] TIM6 usado para controlar a cadencia de envio ao DAC.
- [x] LDR usado para controlar a amplitude da senoide.
- [x] Botao azul configurado com interrupcao EXTI.
- [x] Modos 1, 2 e 3 implementados.
- [x] USART2 usada para monitoramento dos valores.
- [x] Diagnostico dos canais `CH0`, `CH1`, `CH2` e `CH3` exibido no terminal.

---

## Licenca

Projeto desenvolvido para fins academicos no ambito da disciplina de Sistemas Embarcados da UPE/POLI.
