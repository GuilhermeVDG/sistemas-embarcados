# Lista 4 - Comunicacao I2C com PCF8591

> Sistemas Embarcados - UPE/POLI
> Implementacao de comunicacao **I2C** entre a **NUCLEO-L476RG** e o modulo **PCF8591**, com comandos enviados pelo terminal serial via **USART2**.

## Sumario

- [Visao Geral](#visao-geral)
- [Arquitetura do Sistema](#arquitetura-do-sistema)
- [Hardware Utilizado](#hardware-utilizado)
- [Pinagem e Conexoes](#pinagem-e-conexoes)
- [Configuracao dos Perifericos](#configuracao-dos-perifericos)
- [Comandos do Terminal](#comandos-do-terminal)
- [Fluxo de Execucao](#fluxo-de-execucao)
- [Estrutura do Codigo](#estrutura-do-codigo)
- [Como Compilar e Gravar](#como-compilar-e-gravar)
- [Como Testar](#como-testar)
- [Estrutura dos Diretorios](#estrutura-dos-diretorios)
- [Tecnologias e Ferramentas](#tecnologias-e-ferramentas)
- [Criterios de Avaliacao Atendidos](#criterios-de-avaliacao-atendidos)

---

## Visao Geral

Este projeto implementa um sistema embarcado em que a NUCLEO-L476RG atua como **master I2C** e conversa com o PCF8591, um conversor ADC/DAC de 8 bits. O usuario envia comandos pelo terminal serial para ler sensores analogicos ou escrever um valor no DAC.

### Objetivos tecnicos atendidos

- Comunicacao I2C entre STM32 e PCF8591.
- Leitura dos canais analogicos AIN0, AIN1 e AIN3.
- Escrita na saida analogica DAC do PCF8591.
- Recepcao de comandos via UART por interrupcao.
- Operacoes I2C nao bloqueantes usando callbacks da HAL.
- Mensagens de resposta e diagnostico no terminal serial.

---

## Arquitetura do Sistema

```text
+-------------------+        USART2 / USB        +-------------------+
|                   |<-------------------------->|                   |
|  PC / Tera Term   |                            |  NUCLEO-L476RG    |
|  115200 8N1       |                            |  Master I2C       |
+-------------------+                            +---------+---------+
                                                        |
                                                        | I2C1
                                                        | PB8/PB9
                                                        v
                                               +--------+---------+
                                               |                  |
                                               |  PCF8591         |
                                               |  ADC/DAC 8 bits  |
                                               |                  |
                                               +------------------+
```

---

## Hardware Utilizado

| Componente | Descricao | Uso no projeto |
|---|---|---|
| NUCLEO-L476RG | Placa STM32 principal | Master I2C e interface com o PC |
| PCF8591 | Modulo ADC/DAC I2C | Slave I2C |
| Cabo USB | Alimentacao, gravacao e terminal serial | USART2 via ST-LINK |
| Jumpers | Fios de conexao | Barramento I2C e alimentacao |

---

## Pinagem e Conexoes

### PCF8591 para NUCLEO-L476RG

| PCF8591 | NUCLEO-L476RG | Funcao |
|---|---|---|
| VCC | 3V3 | Alimentacao |
| GND | GND | Referencia comum |
| SCL | D15 / PB8 | I2C1_SCL |
| SDA | D14 / PB9 | I2C1_SDA |

> Importante: use **3V3** para manter compatibilidade com os niveis logicos da NUCLEO-L476RG.

### Mapeamento dos sensores do modulo

| Jumper | Funcao | Canal |
|---|---|---|
| J5 | LDR | AIN0 |
| J4 | Termistor | AIN1 |
| J6 | Potenciometro | AIN3 |

O canal AIN2 nao foi usado nesta entrega.

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

Na HAL do STM32, o endereco de 7 bits deve ser deslocado para a esquerda:

```c
#define PCF8591_ADDR (0x48 << 1)
```

### UART

| Parametro | Valor |
|---|---|
| Terminal usado | USART2 |
| Baud rate | 115200 |
| Word length | 8 bits |
| Parity | None |
| Stop bits | 1 |
| Modo | TX/RX |

A `USART3` tambem esta configurada no projeto, mas o `main.c` usa a **USART2** porque ela e a porta ligada ao ST-LINK/USB da NUCLEO-L476RG.

### NVIC

| Interrupcao | Uso |
|---|---|
| I2C1_EV_IRQn | Eventos de transmissao e recepcao I2C |
| I2C1_ER_IRQn | Tratamento de erro I2C |
| USART2_IRQn | Recepcao de comandos do terminal |
| USART3_IRQn | Mantida configurada no projeto |

---

## Comandos do Terminal

Configure o terminal em:

```text
115200 baud
8 data bits
No parity
1 stop bit
Line ending: CR/LF ou Enter
```

| Comando | Acao esperada |
|---|---|
| `Read_AIN0` | Le o LDR conectado ao AIN0 |
| `Read_AIN1` | Le o termistor conectado ao AIN1 |
| `Read_AIN3` | Le o potenciometro conectado ao AIN3 |
| `Set_DAC_0` | Escreve 0 no DAC |
| `Set_DAC_128` | Escreve 128 no DAC |
| `Set_DAC_255` | Escreve 255 no DAC |

Tambem funciona qualquer valor no formato:

```text
Set_DAC_<valor>
```

com `<valor>` entre `0` e `255`.

---

## Fluxo de Execucao

```text
+------------------------------------------------------------------+
| 1. Sistema inicializa GPIO, USART2, I2C1 e USART3                |
| 2. LED LD2 pisca tres vezes como indicacao de startup            |
| 3. Menu de comandos e enviado ao terminal                        |
| 4. USART2 fica armada com HAL_UART_Receive_IT()                  |
| 5. Usuario digita comando e pressiona Enter                      |
| 6. Callback UART monta a string recebida                         |
| 7. Loop principal chama process_command()                        |
| 8. Se for leitura ADC:                                           |
|      +-> envia byte de controle via I2C IT                       |
|      +-> callback Tx completa inicia Receive IT                  |
|      +-> callback Rx imprime AINx no terminal                    |
| 9. Se for escrita DAC:                                           |
|      +-> envia enable DAC + valor via I2C IT                     |
|      +-> callback Tx confirma valor no terminal                  |
+------------------------------------------------------------------+
```

---

## Estrutura do Codigo

### Defines principais

```c
#define PCF8591_ADDR       (0x48 << 1)

#define PCF8591_CH0        0x00
#define PCF8591_CH1        0x01
#define PCF8591_CH3        0x03
#define PCF8591_DAC_ENABLE 0x40

#define RX_BUFFER_SIZE     64
```

### Recepcao UART

A recepcao de comandos e feita byte a byte por interrupcao:

```c
HAL_UART_Receive_IT(&huart2, &uart_rx_byte, 1);
```

Quando chega `\r` ou `\n`, o comando e marcado como pronto e processado no loop principal.

### Leitura ADC

Para ler um canal analogico, o codigo:

1. Envia o byte do canal desejado.
2. Inicia recepcao I2C de 2 bytes no callback de transmissao.
3. Usa o segundo byte como valor valido.

```c
HAL_I2C_Master_Transmit_IT(&hi2c1, PCF8591_ADDR, i2c_tx_buffer, 1);
HAL_I2C_Master_Receive_IT(&hi2c1, PCF8591_ADDR, i2c_rx_buffer, 2);
```

### Escrita DAC

Para escrever no DAC, sao enviados dois bytes:

```c
i2c_tx_buffer[0] = PCF8591_DAC_ENABLE;
i2c_tx_buffer[1] = value;
```

---

## Como Compilar e Gravar

```text
1. Abrir o STM32CubeIDE.
2. File -> Open Projects from File System.
3. Selecionar a pasta Entrega_4_I2C.
4. Project -> Build Project.
5. Conectar a NUCLEO-L476RG via USB.
6. Run -> Debug As -> STM32 C/C++ Application.
7. Apos gravar, abrir o terminal serial na COM do ST-LINK.
```

---

## Como Testar

### Inicializacao

Ao resetar a placa, o terminal deve exibir:

```text
Sistema iniciado na USART2
Terminal: Tera Term / COM3 / 115200
I2C1: PCF8591 endereco 0x48
Comandos disponiveis:
Read_AIN0
Read_AIN1
Read_AIN3
Set_DAC_0 ate Set_DAC_255
```

### Testes principais

| Teste | Comando | Resultado esperado |
|---|---|---|
| Leitura do LDR | `Read_AIN0` | Valor muda ao iluminar/tampar o LDR |
| Leitura do termistor | `Read_AIN1` | Valor muda ao aquecer o sensor |
| Leitura do potenciometro | `Read_AIN3` | Valor varia ao girar o trimpot |
| Escrita no DAC | `Set_DAC_128` | Terminal confirma valor 128 |

Com alimentacao em `3V3`, `Set_DAC_128` deve produzir aproximadamente:

```text
AOUT ~= 1,65 V
```

### Problemas comuns

| Problema | Possivel causa | Verificacao |
|---|---|---|
| Nada aparece no terminal | Porta COM errada | Usar COM do ST-LINK |
| LED pisca ao digitar, mas nao responde | Enter nao enviado | Configurar CR/LF |
| Erro I2C | SDA/SCL invertidos | Conferir PB8/PB9 |
| Leitura sempre igual | Sensor sem variacao ou jumper ausente | Conferir J4, J5 e J6 |
| DAC nao altera AOUT | Valor invalido ou medicao incorreta | Medir AOUT e usar `Set_DAC_128` |

---

## Estrutura dos Diretorios

```text
Entrega_4_I2C/
+-- Core/
|   +-- Inc/                 # Headers do projeto
|   +-- Src/                 # main.c, callbacks e inicializacao
+-- Drivers/                 # HAL e CMSIS
+-- Debug/                   # Saida de build
+-- Entrega_4_I2C.ioc        # Configuracao do STM32CubeMX
+-- STM32L476RGTX_FLASH.ld   # Linker script para Flash
+-- README.md
```

---

## Tecnologias e Ferramentas

- **Linguagem:** C.
- **IDE:** STM32CubeIDE.
- **Configurador:** STM32CubeMX.
- **HAL:** STM32Cube HAL.
- **Comunicacao:** I2C, UART e callbacks de interrupcao.
- **Terminal:** Tera Term, PuTTY ou similar.
- **Debugger:** ST-LINK.

---

## Criterios de Avaliacao Atendidos

- [x] NUCLEO-L476RG configurada como master I2C.
- [x] PCF8591 conectado e enderecado em `0x48`.
- [x] Leitura dos canais AIN0, AIN1 e AIN3.
- [x] Escrita de valores entre 0 e 255 no DAC.
- [x] Interface por terminal serial.
- [x] Recepcao UART por interrupcao.
- [x] Comunicacao I2C com funcoes `_IT`.
- [x] Callbacks de transmissao, recepcao e erro I2C implementados.

---

## Licenca

Projeto desenvolvido para fins academicos no ambito da disciplina de Sistemas Embarcados da UPE/POLI.
