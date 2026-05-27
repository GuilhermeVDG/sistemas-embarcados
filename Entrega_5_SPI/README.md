# Lista 5 - Comunicacao SPI com Matriz 8x8 e PCF8591

> Sistemas Embarcados - UPE/POLI
> Integracao da **NUCLEO-L476RG** com o modulo **PCF8591** via I2C, uma matriz de LEDs **8x8 com MAX7219** via SPI e terminal serial via USART2.

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

Este projeto implementa um sistema embarcado que integra tres interfaces de comunicacao:

- **UART**, para receber comandos pelo terminal serial.
- **I2C**, para ler sensores analogicos do PCF8591.
- **SPI**, para controlar uma matriz de LEDs 8x8 com driver MAX7219.

O usuario envia comandos como `Temp`, `Volt` e `LDR`. A placa le o canal correspondente no PCF8591, mostra o valor no terminal e atualiza a matriz alternando entre uma letra e um sinal `+` ou `-`.

### Objetivos tecnicos atendidos

- Comunicacao SPI com o MAX7219.
- Comunicacao I2C com o PCF8591.
- Interface de comandos via USART2.
- Leitura dos canais AIN0, AIN1 e AIN3.
- Exibicao de letras e sinais em matriz 8x8.
- Atualizacao periodica do display usando `HAL_GetTick()`.

---

## Arquitetura do Sistema

```text
+-------------------+        USART2 / USB        +-------------------+
|                   |<-------------------------->|                   |
|  PC / Terminal    |                            |  NUCLEO-L476RG    |
|  115200 8N1       |                            |  Controlador      |
+-------------------+                            +----+---------+----+
                                                     |         |
                                      I2C1 PB8/PB9   |         | SPI1 PA5/PA7 + PB6
                                                     v         v
                                             +-------+--+   +--+----------------+
                                             | PCF8591  |   | MAX7219 8x8 LED   |
                                             | ADC/DAC  |   | Matriz de LEDs    |
                                             +----------+   +-------------------+
```

### Regra de exibicao

```text
+-------------------+----------------------+
| Valor lido        | Sinal exibido        |
+-------------------+----------------------+
| 0 a 127           | '-'                  |
| 128 a 255         | '+'                  |
+-------------------+----------------------+
```

---

## Hardware Utilizado

| Componente | Descricao | Uso no projeto |
|---|---|---|
| NUCLEO-L476RG | Placa STM32 principal | Controla todo o sistema |
| PCF8591 | Modulo ADC/DAC I2C | Le sensores analogicos |
| MAX7219 8x8 LED Matrix | Matriz de LEDs com driver SPI | Exibe letras e sinais |
| Cabo USB | Alimentacao, gravacao e terminal serial | Conexao com o PC |
| Jumpers | Fios de conexao | Ligacao entre os modulos |

---

## Pinagem e Conexoes

### PCF8591 via I2C

| PCF8591 | NUCLEO-L476RG | Funcao |
|---|---|---|
| VCC | 3V3 | Alimentacao |
| GND | GND | Referencia comum |
| SCL | D15 / PB8 | I2C1_SCL |
| SDA | D14 / PB9 | I2C1_SDA |

### MAX7219 via SPI

| MAX7219 | NUCLEO-L476RG | Funcao |
|---|---|---|
| VCC | 5V | Alimentacao da matriz |
| GND | GND | Referencia comum |
| DIN | D11 / PA7 | SPI1_MOSI |
| CS | D10 / PB6 | Chip Select manual |
| CLK | D13 / PA5 | SPI1_SCK |

> O pino MISO/D12 nao e usado fisicamente, pois o MAX7219 apenas recebe dados.

### Mapeamento dos comandos de grandeza

| Grandeza | Comando | Canal lido | Exibicao |
|---|---|---|---|
| Temperatura | `Temp` | AIN0 | `T` alternando com `+` ou `-` |
| Tensao | `Volt` | AIN3 | `V` alternando com `+` ou `-` |
| Luminosidade | `LDR` | AIN1 | `L` alternando com `+` ou `-` |

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

### SPI1

| Parametro | Valor |
|---|---|
| Periferico | SPI1 |
| Modo | Full-Duplex Master |
| Hardware NSS | Disable |
| Data Size | 8 bits |
| First Bit | MSB First |
| Clock Polarity | Low |
| Clock Phase | 1 Edge |
| Prescaler | 32 |
| CS manual | PB6 / D10 |

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

## Comandos do Terminal

Configure o terminal em `115200 8N1` e envie os comandos com Enter.

### Exibicao na matriz

| Comando | Acao |
|---|---|
| `Temp` | Le AIN0 e alterna `T` com `+` ou `-` |
| `Volt` | Le AIN3 e alterna `V` com `+` ou `-` |
| `LDR` | Le AIN1 e alterna `L` com `+` ou `-` |
| `Test` | Exibe padrao quadriculado na matriz |
| `Clear` | Apaga a matriz |

---

## Fluxo de Execucao

```text
+---------------------------------------------------------------------+
| 1. Sistema inicializa GPIO, USART2, I2C1 e SPI1                     |
| 2. CS do MAX7219 inicia em nivel alto                               |
| 3. MAX7219 e configurado e a matriz e limpa                         |
| 4. Menu de comandos e enviado ao terminal                           |
| 5. Padrao de teste aparece por 1 segundo e depois a matriz apaga    |
| 6. Loop principal executa duas tarefas:                             |
|      +-> UART_CheckCommand(): recebe comandos do terminal           |
|      +-> Display_UpdateTask(): alterna caractere a cada 500 ms      |
| 7. Comandos Temp/Volt/LDR acessam o PCF8591 via I2C                 |
| 8. A matriz e atualizada com a letra da grandeza e o sinal via SPI  |
+---------------------------------------------------------------------+
```

---

## Estrutura do Codigo

### Estados de exibicao

```c
typedef enum
{
  DISPLAY_NONE = 0,
  DISPLAY_TEMP,
  DISPLAY_VOLT,
  DISPLAY_LDR
} DisplayMode_t;
```

### Escrita no MAX7219

O envio para a matriz e feito por dois bytes: registrador e dado.

```c
HAL_GPIO_WritePin(MAX7219_CS_GPIO_Port, MAX7219_CS_Pin, GPIO_PIN_RESET);
HAL_SPI_Transmit(&hspi1, tx_data, 2, HAL_MAX_DELAY);
HAL_GPIO_WritePin(MAX7219_CS_GPIO_Port, MAX7219_CS_Pin, GPIO_PIN_SET);
```

### Inicializacao do MAX7219

| Registrador | Valor | Funcao |
|---|---|---|
| Display Test | `0x00` | Desliga modo de teste |
| Decode Mode | `0x00` | Modo no-decode |
| Scan Limit | `0x07` | Usa 8 linhas |
| Intensity | `0x03` | Brilho baixo/medio |
| Shutdown | `0x01` | Liga o display |

### Leitura do PCF8591

A leitura usa dois bytes e considera o segundo como valor valido:

```c
HAL_I2C_Master_Transmit(&hi2c1, PCF8591_ADDR, &control_byte, 1, 100);
HAL_I2C_Master_Receive(&hi2c1, PCF8591_ADDR, rx_data, 2, 100);
*value = rx_data[1];
```

### Atualizacao da matriz

```text
+---------------------------+
| Display_UpdateTask()      |
+---------------------------+
| Se nao ha modo ativo      |
|   retorna                 |
| A cada 500 ms             |
|   alterna toggle_state    |
|   mostra letra ou sinal   |
+---------------------------+
```

---

## Como Compilar e Gravar

```text
1. Abrir o STM32CubeIDE.
2. File -> Open Projects from File System.
3. Selecionar a pasta Entrega_5_SPI.
4. Project -> Build Project.
5. Conectar a NUCLEO-L476RG via USB.
6. Run -> Debug As -> STM32 C/C++ Application.
7. Apos gravar, abrir o terminal serial na COM do ST-LINK.
```

---

## Como Testar

### Inicializacao

Ao resetar, o terminal deve exibir:

```text
NUCLEO-L476RG + PCF8591 + MAX7219
Caso de teste iniciado
Baud: 115200
Digite um comando e pressione ENTER
```

A matriz deve exibir um padrao quadriculado por 1 segundo e apagar.

### Casos de teste

| Caso | Comando | Resultado esperado |
|---|---|---|
| SPI da matriz | `Test` | Padrao quadriculado aparece |
| Limpeza da matriz | `Clear` | Todos os LEDs apagam |
| Temperatura | `Temp` | Matriz alterna `T` e sinal |
| Tensao | `Volt` | Matriz alterna `V` e sinal |
| Luminosidade | `LDR` | Matriz alterna `L` e sinal |

### Problemas comuns

| Problema | Possivel causa | Verificacao |
|---|---|---|
| Nada aparece no terminal | Porta COM errada | Usar COM do ST-LINK |
| Caracteres estranhos | Baud rate incorreto | Usar 115200 |
| PCF8591 nao responde | SDA/SCL invertidos ou sem GND | Conferir D14, D15 e GND |
| Matriz apagada | VCC/GND incorretos | Ligar VCC da matriz em 5V |
| Matriz nao responde | DIN/CLK/CS errados | Conferir D11, D13 e D10 |
| Padrao invertido | Orientacao fisica da matriz | Ajustar bitmaps no codigo |

---

## Estrutura dos Diretorios

```text
Entrega_5_SPI/
+-- Core/
|   +-- Inc/                 # Headers do projeto
|   +-- Src/                 # main.c, MAX7219, PCF8591 e comandos
+-- Drivers/                 # HAL e CMSIS
+-- Debug/                   # Saida de build
+-- Entrega_5_SPI.ioc        # Configuracao do STM32CubeMX
+-- STM32L476RGTX_FLASH.ld   # Linker script para Flash
+-- README.md
```

---

## Tecnologias e Ferramentas

- **Linguagem:** C.
- **IDE:** STM32CubeIDE.
- **Configurador:** STM32CubeMX.
- **HAL:** STM32Cube HAL.
- **Comunicacao:** UART, I2C e SPI.
- **Terminal:** Tera Term, PuTTY ou similar.
- **Debugger:** ST-LINK.

---

## Criterios de Avaliacao Atendidos

- [x] NUCLEO-L476RG configurada como controladora principal.
- [x] PCF8591 integrado via I2C.
- [x] MAX7219 integrado via SPI.
- [x] USART2 usada como terminal serial.
- [x] Comandos de leitura analogica implementados.
- [x] Matriz 8x8 exibe letras e sinais.
- [x] Comandos `Test` e `Clear` implementados.
- [x] Atualizacao da matriz sem `HAL_Delay()` no loop principal.

---

## Licenca

Projeto desenvolvido para fins academicos no ambito da disciplina de Sistemas Embarcados da UPE/POLI.
