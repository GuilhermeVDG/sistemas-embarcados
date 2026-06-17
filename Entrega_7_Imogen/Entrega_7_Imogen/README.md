# 📘 Entrega 7 — Monitor Portátil de Temperatura e CO2 (STM32 + NestJS)

> Sistemas Embarcados — UPE/POLI
> Sistema embarcado para monitoramento de temperatura e concentração estimada de CO2 durante o transporte de material biológico sensível, com exibição local em LCD, transmissão de dados via UART em JSON e armazenamento histórico em backend NestJS.

## Sumário

- [Visão Geral](#visão-geral)
- [Arquitetura do Sistema](#arquitetura-do-sistema)
- [Hardware Utilizado](#hardware-utilizado)
- [Pinagem e Conexões](#pinagem-e-conexões)
- [Configuração dos Periféricos](#configuração-dos-periféricos)
- [Firmware — Fluxo de Execução](#firmware--fluxo-de-execução)
- [Formato JSON enviado pela UART](#formato-json-enviado-pela-uart)
- [Backend NestJS](#backend-nestjs)
- [Como Compilar e Gravar o Firmware](#como-compilar-e-gravar-o-firmware)
- [Como Rodar o Sistema Completo](#como-rodar-o-sistema-completo)
- [Como Testar](#como-testar)
- [Estrutura dos Diretórios](#estrutura-dos-diretórios)
- [Tecnologias e Ferramentas](#tecnologias-e-ferramentas)
- [Observação sobre co2_ppm](#observação-sobre-co2_ppm)
- [Critérios de Avaliação Atendidos](#critérios-de-avaliação-atendidos)

---

## Visão Geral

O sistema é composto por duas partes integradas:

**Firmware (STM32 NUCLEO-L476RG):**
- Lê a temperatura via I2C do sensor WCMCU-75 (LM75A)
- Lê o sensor analógico MQ-135 via ADC para estimar CO2 em ppm
- Exibe os valores em tempo real no display LCD 1602 paralelo (modo 4 bits)
- Transmite uma linha JSON por segundo via UART (115200 baud) para o computador

**Backend (NestJS + SQLite):**
- Lê continuamente a porta serial e faz parsing do JSON
- Armazena o histórico de medições em banco SQLite
- Disponibiliza API REST para consultar o histórico, a última medição e estatísticas
- Continua funcionando mesmo se a placa for desconectada temporariamente
- Swagger disponível em `http://localhost:3000/docs`

---

## Arquitetura do Sistema

```
┌──────────────────────────────────────────────────────────────────┐
│                     STM32 NUCLEO-L476RG                          │
│                                                                  │
│  WCMCU-75 (I2C) ──► Leitura temperatura                         │
│  MQ-135   (ADC) ──► Estimativa CO2                               │
│                          │                                       │
│                          ▼                                       │
│              Formata JSON + exibe no LCD 1602                    │
│                          │                                       │
│                    USART2 (115200)                               │
└──────────────────────────┼───────────────────────────────────────┘
                           │ USB / ST-LINK Virtual COM
                           ▼
┌──────────────────────────────────────────────────────────────────┐
│                   Backend NestJS (porta 3000)                    │
│                                                                  │
│  SerialService ──► parseMeasurementLine() ──► MeasurementsService│
│                                                      │           │
│                                               TypeORM │          │
│                                                      ▼           │
│                                             SQLite (data/)       │
│                                                                  │
│  GET /measurements         ← histórico completo                  │
│  GET /measurements/latest  ← última medição                      │
│  GET /measurements/stats   ← min, max, média                     │
│  POST /measurements        ← inserção manual                     │
│  GET /docs                 ← Swagger UI                          │
└──────────────────────────────────────────────────────────────────┘
```

---

## Hardware Utilizado

| Componente | Descrição |
|---|---|
| NUCLEO-L476RG | Placa STM32L476RG (Cortex-M4 @ 80 MHz) |
| WCMCU-75 | Módulo sensor de temperatura com LM75A (I2C, resolução 0,5 °C) |
| MQ-135 / FC-22 | Sensor analógico de qualidade do ar, usado para estimar CO2 |
| LCD 1602 | Display alfanumérico 16x2 paralelo, modo 4 bits |
| Resistores | 10 kΩ e 1 kΩ — divisor resistivo na saída analógica do MQ-135 |
| Cabo USB mini-B | Alimentação + ST-LINK + USART2 virtual COM |

---

## Pinagem e Conexões

### LCD 1602 — modo 4 bits

| Sinal LCD | Pino MCU | Observação |
|---|---|---|
| RS | PA8 | Seleção registrador (comando / dado) |
| E | PA9 | Enable (pulso para latch) |
| D4 | PB5 | Nibble baixo bit 0 |
| D5 | PB4 | Nibble baixo bit 1 |
| D6 | PB10 | Nibble baixo bit 2 |
| D7 | PC7 | Nibble baixo bit 3 |
| VSS | GND | — |
| VDD | 5V | — |
| VO | GND ou potenciômetro | Ajuste de contraste |
| RW | GND | Somente escrita |
| A (backlight +) | 5V com resistor ~300 Ω | — |
| K (backlight −) | GND | — |

### WCMCU-75 (LM75A) — I2C

| Sinal | Pino MCU | Observação |
|---|---|---|
| SDA | PB7 | I2C1 |
| SCL | PB6 | I2C1 |
| A0, A1, A2 | GND | Fixa endereço em 0x48 |
| VCC | 3.3V | — |
| GND | GND | — |

> O firmware detecta automaticamente o endereço do WCMCU-75 no intervalo 0x48–0x4F.

### MQ-135 — divisor resistivo para PA0

```
Saída AO do MQ-135
        │
      [10 kΩ]
        │
        ├──── PA0 (ADC1_IN5)
        │
      [1 kΩ]
        │
       GND
```

Fator do divisor: **11**. A tensão no pino PA0 fica dentro dos 3,3 V suportados pelo MCU.

### UART para o PC

| Sinal | Pino MCU | Observação |
|---|---|---|
| TX | PA2 | USART2 — ST-LINK Virtual COM |
| RX | PA3 | USART2 — ST-LINK Virtual COM |

---

## Configuração dos Periféricos

| Periférico | Configuração |
|---|---|
| System Clock | HCLK @ 80 MHz (PLL via MSI, PLLN=40, PLLR=2) |
| I2C1 | Fast Mode, PB6 (SCL) / PB7 (SDA) |
| ADC1 | Canal IN5 (PA0), resolução 12 bits, single conversion, calibração por software |
| USART2 | Asynchronous, 115200 8N1, polling (HAL_UART_Transmit) |
| GPIO saídas | PA8, PA9, PB4, PB5, PB10, PC7 — Push-Pull, Low speed (LCD) |

---

## Firmware — Fluxo de Execução

```
Inicialização
    │
    ├─► HAL_Init(), SystemClock_Config()
    ├─► MX_GPIO_Init(), MX_ADC1_Init(), MX_I2C1_Init(), MX_USART2_UART_Init()
    ├─► HAL_ADCEx_Calibration_Start()
    └─► lcd_init() → exibe "Sistema OK / Iniciando..."

Loop principal (1 vez por segundo)
    │
    ├─► wcmcu75_detect_address()       — varre 0x48–0x4F via HAL_I2C_IsDeviceReady
    ├─► wcmcu75_read_temp_x10()        — lê 2 bytes, extrai 9 bits, resultado em °C×10
    ├─► adc_read()                     — HAL_ADC_Start + PollForConversion
    ├─► mq135_vpin_mv()                — raw * 3300 / 4095
    ├─► mq135_vao_mv()                 — vpin_mv * 11 (desfaz divisor)
    ├─► mq135_estimate_co2_ppm()       — 400 + (raw − 550) × 2
    │
    ├─► Formata LCD (linha 1: temperatura, linha 2: CO2)
    ├─► lcd_set_cursor + lcd_print_padded
    │
    ├─► Formata JSON (ver seção abaixo)
    ├─► uart_print() → HAL_UART_Transmit USART2
    │
    └─► HAL_Delay(1000)
```

---

## Formato JSON enviado pela UART

Uma linha por segundo, terminada em `\r\n`.

**Medição normal:**
```json
{"temperature_c":23.5,"co2_ppm":502,"adc_raw":601,"vpin_mv":484,"vao_mv":5324,"sensor_status":"OK"}
```

**Falha no sensor de temperatura (WCMCU-75 não responde via I2C):**
```json
{"temperature_c":null,"co2_ppm":500,"adc_raw":600,"vpin_mv":483,"vao_mv":5313,"sensor_status":"ERRO_I2C"}
```

| Campo | Tipo | Descrição |
|---|---|---|
| `temperature_c` | float \| null | Temperatura em °C. Null se o WCMCU-75 falhar |
| `co2_ppm` | integer | Estimativa didática de CO2 em ppm |
| `adc_raw` | integer | Valor bruto do ADC (0–4095) |
| `vpin_mv` | integer | Tensão no pino PA0 em mV |
| `vao_mv` | integer | Tensão estimada na saída do MQ-135 antes do divisor, em mV |
| `sensor_status` | string | `"OK"` ou `"ERRO_I2C"` |

---

## Backend NestJS

O backend está em `backend/`. Documentação completa de instalação, configuração, endpoints e exemplos em:

👉 **[backend/README.md](./backend/README.md)**

Resumo rápido:

```bash
cd backend
npm install
cp .env.example .env
# edite SERIAL_PORT com a porta correta (ex: /dev/cu.usbmodem114103 no macOS)
npm run start:dev
```

Swagger UI: **http://localhost:3000/docs**

### Endpoints disponíveis

| Método | Rota | Descrição |
|---|---|---|
| GET | `/health` | Status do serviço |
| GET | `/measurements` | Histórico de medições (mais recentes primeiro) |
| GET | `/measurements/latest` | Última medição recebida |
| GET | `/measurements/stats` | Estatísticas (min, max, média de temperatura e CO2) |
| POST | `/measurements` | Inserção manual de medição |

---

## Como Compilar e Gravar o Firmware

### Pré-requisitos

- [STM32CubeIDE](https://www.st.com/en/development-tools/stm32cubeide.html) 1.x ou superior
- Drivers ST-LINK (incluídos na instalação do CubeIDE)

### Passo a passo

```
1. Abra o STM32CubeIDE
   File → Open Projects from File System
   Selecione a pasta: Entrega_7_Imogen

2. Aguarde a indexação do projeto

3. Compile:
   Project → Build Project  (Ctrl+B)

4. Conecte a NUCLEO-L476RG via cabo USB

5. Grave e depure:
   Run → Debug As → STM32 C/C++ Application

6. Após gravar, pressione Resume (F8) ou encerre o debug para rodar livremente
```

---

## Como Rodar o Sistema Completo

### 1. Descubra a porta serial

**macOS:**
```bash
ls /dev/cu.usbmodem*
```

**Linux:**
```bash
ls /dev/ttyACM*
```

**Windows:** Gerenciador de Dispositivos → Portas (COM e LPT), ex: `COM3`.

### 2. Configure e inicie o backend

```bash
cd backend
cp .env.example .env
# edite SERIAL_PORT com o valor encontrado acima
npm install
npm run start:dev
```

### 3. Verifique os logs

Você deve ver no terminal:

```
[app] Backend iniciado na porta 3000
[db] Banco conectado
[serial] Tentando conectar em /dev/cu.usbmodem114103 @ 115200
[serial] Conectado
[serial] Linha recebida: {"temperature_c":23.5,"co2_ppm":502,...}
[parser] Medição válida
[db] Medição salva id=1
```

### 4. Acesse os dados

```bash
# via curl
curl http://localhost:3000/measurements/latest
curl http://localhost:3000/measurements/stats

# via Swagger
# http://localhost:3000/docs
```

---

## Como Testar

### Verificar recepção serial diretamente

```bash
screen /dev/cu.usbmodem114103 115200
# pressione Ctrl+A, K para sair
```

### Inserir medição manualmente (sem a placa)

```bash
curl -X POST http://localhost:3000/measurements \
  -H "Content-Type: application/json" \
  -d '{"temperature_c":22.0,"co2_ppm":450,"adc_raw":580,"vpin_mv":467,"vao_mv":5137,"sensor_status":"OK","source":"manual"}'
```

### Consultar estatísticas com filtro de data

```bash
curl "http://localhost:3000/measurements/stats?start_date=2026-06-01T00:00:00"
```

### Testar resiliência

Desconecte o cabo USB da placa enquanto o backend está rodando. Após alguns segundos, você deve ver:

```
[serial] Conexão perdida. Tentando reconectar em 5s
[serial] Tentando conectar em /dev/cu.usbmodem114103 @ 115200
```

Ao reconectar o cabo, o backend retoma automaticamente sem necessidade de reiniciar.

---

## Estrutura dos Diretórios

```
Entrega_7_Imogen/
│
├── Core/
│   ├── Inc/
│   │   ├── main.h
│   │   ├── adc.h
│   │   ├── i2c.h
│   │   ├── usart.h
│   │   └── gpio.h
│   └── Src/
│       ├── main.c          ← lógica principal: LCD, sensores, UART, JSON
│       ├── adc.c           ← inicialização do ADC1
│       ├── i2c.c           ← inicialização do I2C1
│       ├── usart.c         ← inicialização do USART2
│       └── gpio.c          ← inicialização dos GPIOs do LCD
│
├── Drivers/                ← HAL e CMSIS (STM32L4)
│
├── backend/
│   ├── src/
│   │   ├── main.ts                       ← bootstrap NestJS + Swagger
│   │   ├── app.module.ts                 ← módulo raiz, TypeORM, ConfigModule
│   │   ├── app.controller.ts             ← GET /health
│   │   ├── measurements/
│   │   │   ├── measurement.entity.ts     ← entidade TypeORM
│   │   │   ├── measurements.service.ts   ← lógica de negócio e queries
│   │   │   ├── measurements.controller.ts← endpoints REST
│   │   │   ├── measurements.module.ts
│   │   │   └── dto/
│   │   │       ├── create-measurement.dto.ts
│   │   │       └── query-measurements.dto.ts
│   │   └── serial/
│   │       ├── parser.ts                 ← parse e validação das linhas JSON
│   │       ├── serial.service.ts         ← leitura serial com reconexão automática
│   │       └── serial.module.ts
│   ├── data/                             ← banco SQLite (gerado automaticamente)
│   ├── .env.example
│   ├── package.json
│   └── README.md                         ← documentação detalhada do backend
│
├── Entrega_7_Imogen.ioc    ← configuração STM32CubeMX
├── STM32L476RGTX_FLASH.ld
└── README.md               ← este arquivo
```

---

## Tecnologias e Ferramentas

### Firmware

| Tecnologia | Uso |
|---|---|
| Linguagem C (GNU C11) | Código do firmware |
| STM32CubeIDE | IDE de desenvolvimento e gravação |
| STM32CubeMX | Configuração de periféricos (.ioc) |
| STM32 HAL (STM32L4) | Abstração de hardware |
| GNU Arm Embedded Toolchain | Compilação cruzada (`arm-none-eabi-gcc`) |
| ST-LINK V2 | Gravação e depuração |

### Backend

| Tecnologia | Uso |
|---|---|
| Node.js 18+ | Runtime |
| NestJS | Framework principal |
| TypeScript | Linguagem |
| TypeORM | ORM para acesso ao banco |
| SQLite (better-sqlite3) | Banco de dados local |
| SerialPort | Leitura da porta serial |
| @nestjs/swagger | Documentação automática da API |
| class-validator | Validação de DTOs |

---

## Observação sobre `co2_ppm`

O campo `co2_ppm` é uma **estimativa didática** e não representa uma medição certificada de CO2.

O sensor MQ-135/FC-22 é um sensor resistivo de qualidade do ar, sensível a vários gases (amônia, benzeno, álcool, CO2 etc.), e **não foi calibrado laboratorialmente** para CO2 puro.

A fórmula usada é:

```
co2_ppm = 400 + (adc_raw − 550) × 2
```

Ela serve para **demonstrar tendência de variação** conforme o ambiente ao redor do sensor muda (ex: aproximar o sensor da boca, ou em ambientes mais fechados), não como valor absoluto confiável.

---

## Critérios de Avaliação Atendidos

- [x] Leitura de temperatura via I2C (WCMCU-75 / LM75A)
- [x] Leitura analógica via ADC (MQ-135) com divisor resistivo
- [x] Estimativa de CO2 em ppm com fórmula documentada
- [x] Exibição em tempo real no LCD 1602 (modo 4 bits, driver implementado em software)
- [x] Transmissão de medições via UART em formato JSON estruturado (1 linha/segundo)
- [x] Tratamento de falha no sensor de temperatura (campo `null`, status `ERRO_I2C`)
- [x] Backend persistindo histórico em banco de dados (SQLite via TypeORM)
- [x] API REST com consulta de histórico, última medição e estatísticas
- [x] Reconexão automática da porta serial sem derrubar o serviço
- [x] Documentação da API via Swagger

---

## Licença

Projeto desenvolvido para fins acadêmicos no âmbito da disciplina de Sistemas Embarcados da UPE/POLI.
