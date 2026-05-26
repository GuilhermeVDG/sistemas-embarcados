# Comunicação SPI com Matriz de LEDs 8x8 e Sensor PCF8591

## Objetivo

Implementar um sistema embarcado utilizando a placa **NUCLEO-L476RG** para integrar dois módulos externos:

- **PCF8591**, usando comunicação **I2C**, para leitura de entradas analógicas e escrita em saída DAC.
- **Matriz de LEDs 8x8 com MAX7219**, usando comunicação **SPI**, para exibir caracteres de acordo com os valores lidos.
- **Terminal serial**, usando **USART2**, para envio de comandos pelo computador.

O sistema recebe comandos via terminal, lê sensores no PCF8591 e exibe informações na matriz de LEDs 8x8.

---

## Hardware Utilizado

| Item | Descrição | Uso no projeto |
|------|-----------|----------------|
| NUCLEO-L476RG | Placa principal STM32 | Controlador principal |
| PCF8591 | Módulo ADC/DAC I2C | Leitura de sinais analógicos e saída DAC |
| MAX7219 8x8 LED Matrix | Matriz de LEDs 8x8 com driver SPI | Exibição de caracteres |
| Jumpers | Fios de conexão | Ligações entre os módulos |
| Cabo USB | Alimentação e comunicação serial | Conexão entre NUCLEO e PC |

---

## Visão Geral do Sistema

A placa **NUCLEO-L476RG** atua como controladora principal.

Ela se comunica com:

- O **PCF8591** via **I2C1**, para ler os canais analógicos `AIN0`, `AIN1` e `AIN3`.
- A **matriz de LEDs 8x8 MAX7219** via **SPI1**, para exibir caracteres.
- O **computador** via **USART2**, para receber comandos pelo terminal serial.

O comportamento principal é:

- O usuário envia comandos pelo terminal.
- A NUCLEO interpreta o comando.
- Se necessário, lê um canal analógico do PCF8591.
- Exibe no terminal o valor lido.
- Atualiza a matriz LED com letras e sinais.

---

## Ligações Físicas

### PCF8591 com NUCLEO-L476RG

| PCF8591 | NUCLEO-L476RG |
|---------|---------------|
| VCC | 3V3 |
| GND | GND |
| SCL | SCL/D15 |
| SDA | SDA/D14 |

A alimentação do PCF8591 foi feita em **3V3** para manter compatibilidade com os níveis lógicos da NUCLEO-L476RG.

### Correspondência dos pinos I2C

| Nome na placa | Nome no STM32CubeMX | Função |
|---------------|---------------------|--------|
| D15 / SCL | PB8 | I2C1_SCL |
| D14 / SDA | PB9 | I2C1_SDA |

---

### Matriz MAX7219 com NUCLEO-L476RG

| MAX7219 | NUCLEO-L476RG | Função |
|---------|---------------|--------|
| VCC | 5V | Alimentação da matriz |
| GND | GND | Terra comum |
| DIN | D11 / PA7 | SPI1_MOSI |
| CS | D10 / PB6 | Chip Select manual |
| CLK | D13 / PA5 | SPI1_SCK |

### Correspondência dos pinos SPI

| Nome na placa | Nome no STM32CubeMX | Função |
|---------------|---------------------|--------|
| D13 / SCK | PA5 | SPI1_SCK |
| D12 / MISO | PA6 | SPI1_MISO, não utilizado fisicamente |
| D11 / MOSI | PA7 | SPI1_MOSI |
| D10 / CS | PB6 | GPIO_Output para CS |

O pino **MISO/D12** não é ligado na matriz, pois o MAX7219 apenas recebe dados da NUCLEO.

---

## Configuração no STM32CubeIDE

O projeto foi criado para a placa:

```text
NUCLEO-L476RG