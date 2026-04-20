# Lista 3 — Sistema Cliente-Servidor em Comunicação UART

> Sistemas Embarcados — UPE/POLI
> Implementação de um sistema cliente-servidor usando as placas **NUCLEO-L476RG** e **STM32F407G-DISC1**, comunicação via UART com interrupção e DMA, controle de LED por cadência e transmissão de tabela da equipe para o PC.

## Sumário

- [Visão Geral](#visão-geral)
- [Arquitetura do Sistema](#arquitetura-do-sistema)
- [Hardware Utilizado](#hardware-utilizado)
- [Pinagem e Conexões](#pinagem-e-conexões)
- [Configuração dos Periféricos](#configuração-dos-periféricos)
- [Fluxo de Execução](#fluxo-de-execução)
- [Estrutura do Código](#estrutura-do-código)
- [Máquina de Estados do Cliente](#máquina-de-estados-do-cliente)
- [Como Compilar e Gravar](#como-compilar-e-gravar)
- [Como Testar](#como-testar)
- [Estrutura dos Diretórios](#estrutura-dos-diretórios)
- [Tecnologias e Ferramentas](#tecnologias-e-ferramentas)

---

## Visão Geral

Este projeto implementa um sistema embarcado distribuído composto por dois microcontroladores STM32 que se comunicam via UART. O **Cliente** (NUCLEO-L476RG) atua como interface do usuário, solicitando dados ao **Servidor** (Discovery F407) mediante acionamento de um botão. O servidor responde com o valor atual de um contador interno e uma tabela contendo os dados da equipe, que é retransmitida ao PC via USB.

### Objetivos técnicos atendidos

- Comunicação serial UART bidirecional entre dois MCUs distintos
- Transmissão e recepção de dados por **interrupção** (`HAL_UART_Transmit_IT` / `HAL_UART_Receive_IT`)
- Transmissão e recepção de blocos de dados via **DMA** (`HAL_UART_Transmit_DMA` / `HAL_UART_Receive_DMA`)
- Tratamento de **interrupção externa (EXTI)** para botões
- Controle de cadência de LED por software, não-bloqueante
- Bloqueio de novos comandos durante processamento (máquina de estados)
- Envio de mensagem formatada e tabela ao terminal virtual do PC

---

## Arquitetura do Sistema

```
┌─────────────────┐          ┌──────────────────┐          ┌──────────────────┐
│                 │          │                  │          │                  │
│  PC (Tera Term) │◄─USART2──┤  CLIENTE         │◄─USART1──┤  SERVIDOR        │
│                 │   USB    │  NUCLEO-L476RG   │  Jumpers │  Discovery F407  │
│                 │          │                  │          │                  │
└─────────────────┘          └──────────────────┘          └──────────────────┘
      COM5                   LED (PA5)  BTN (PC13)          BTN (PA0)  LED (PD12)
      115200 8N1
```

### Papéis

- **Cliente (NUCLEO-L476RG):** interface com usuário e PC. Solicita dados, aciona LED conforme contador recebido, e envia resultado ao terminal do PC.
- **Servidor (Discovery F407):** mantém um contador incrementado por botão, responde ao cliente quando recebe o comando `0x5A`, enviando o contador e a tabela da equipe via DMA.
- **PC (Tera Term):** terminal virtual que exibe as mensagens formatadas transmitidas pelo cliente.

---

## Hardware Utilizado

| Componente | Descrição |
|---|---|
| NUCLEO-L476RG | Placa de desenvolvimento STM32L476RG (Cortex-M4 @ 80 MHz) — **Cliente** |
| STM32F407G-DISC1 | Placa de desenvolvimento STM32F407VG (Cortex-M4 @ 168 MHz) — **Servidor** |
| Cabo USB mini-B | Conexão cliente ↔ PC (alimentação + ST-LINK + USART2 virtual) |
| 4 jumpers (macho-fêmea) | Interconexão entre cliente e servidor |

---

## Pinagem e Conexões

### Cliente (NUCLEO-L476RG)

| Função | Pino MCU | Label Arduino | Conector |
|---|---|---|---|
| USART1 TX (→ servidor) | PA9 | D8 | CN10 |
| USART1 RX (← servidor) | PA10 | D2 | CN10 |
| USART2 TX (→ PC) | PA2 | — | ST-LINK (virtual) |
| USART2 RX (← PC) | PA3 | — | ST-LINK (virtual) |
| LED do usuário | PA5 (LD2) | D13 | — |
| Botão do usuário | PC13 (B1) | — | — |

### Servidor (Discovery F407)

| Função | Pino MCU |
|---|---|
| USART2 TX (→ cliente) | PA2 |
| USART2 RX (← cliente) | PA3 |
| LED debug (verde) | PD12 |
| Botão do usuário | PA0 |

### Tabela de conexão entre placas

| Cliente L476RG | ↔ | Servidor F407 | Função |
|---|:---:|---|---|
| **PA9** (D8, USART1_TX) | → | **PA3** (USART2_RX) | Cliente envia 0x5A |
| **PA10** (D2, USART1_RX) | ← | **PA2** (USART2_TX) | Servidor envia contador + tabela |
| **GND** | ↔ | **GND** | Referência comum (obrigatório) |
| **3V3** | → | **3V** | Cliente alimenta o servidor |

> ⚠️ **Importante:** a conexão TX↔RX é sempre **cruzada**. O GND comum é obrigatório — sem ele, a comunicação não funciona.

---

## Configuração dos Periféricos

### Cliente (NUCLEO-L476RG)

| Periférico | Configuração |
|---|---|
| System Clock | HCLK @ 80 MHz (PLL via HSE bypass do ST-LINK) |
| USART1 | Asynchronous, 115200 8N1, IT + DMA TX/RX |
| USART2 | Asynchronous, 115200 8N1, IT + DMA TX |
| GPIO PA5 | Output Push-Pull (LED_USER) |
| GPIO PC13 | EXTI Falling edge, Pull-up (BTN_USER) |
| NVIC | EXTI15:10, USART1, USART2, DMA1 channels |

### Servidor (Discovery F407)

| Periférico | Configuração |
|---|---|
| System Clock | HCLK @ 168 MHz (PLL via HSE cristal 8 MHz) |
| USART2 | Asynchronous, 115200 8N1, IT + DMA TX/RX |
| GPIO PD12 | Output Push-Pull (LED_DBG) |
| GPIO PA0 | EXTI Rising edge, No pull (BTN_USER) |
| NVIC | EXTI0, USART2, DMA1 stream 5/6 |

---

## Fluxo de Execução

```
┌──────────────────────────────────────────────────────────────────────────┐
│                                                                          │
│  1. Usuário aperta botão do SERVIDOR → contador++                        │
│                                                                          │
│  2. Usuário aperta botão do CLIENTE                                      │
│        └─► Cliente envia 0x5A ao servidor (HAL_UART_Transmit_IT)         │
│        └─► Cliente arma HAL_UART_Receive_IT para 1 byte (contador)       │
│                                                                          │
│  3. Servidor recebe 0x5A na interrupção                                  │
│        └─► Envia valor do contador (1 byte) via HAL_UART_Transmit_IT     │
│        └─► Ao completar TX, envia tabela via HAL_UART_Transmit_DMA       │
│        └─► Ao completar DMA, zera contador e rearma recepção             │
│                                                                          │
│  4. Cliente recebe contador                                              │
│        └─► Arma HAL_UART_Receive_DMA para receber tabela (128 bytes)     │
│                                                                          │
│  5. Cliente recebe tabela                                                │
│        └─► Pisca LED N vezes @ 1 Hz (N = contador recebido)              │
│        └─► Durante o blink, novos apertos no botão são IGNORADOS         │
│                                                                          │
│  6. Ao terminar o blink                                                  │
│        └─► Envia "Numero de eventos = XXX\r\n" ao PC (IT)                │
│        └─► Envia tabela completa ao PC via DMA                           │
│        └─► Retorna ao estado IDLE                                        │
│                                                                          │
└──────────────────────────────────────────────────────────────────────────┘
```

---

## Estrutura do Código

### Cliente — abordagem com máquina de estados

O cliente implementa uma **máquina de estados finita (FSM)** no loop principal para evitar chamadas bloqueantes (como `HAL_Delay`) dentro de callbacks de interrupção. Isso permite que o sistema permaneça responsivo durante operações longas (como o blink do LED).

### Servidor — abordagem orientada a eventos

O servidor responde inteiramente via callbacks de interrupção. O `while(1)` permanece vazio, e todo o fluxo é disparado por:
- `HAL_GPIO_EXTI_Callback` (botão)
- `HAL_UART_RxCpltCallback` (comando recebido)
- `HAL_UART_TxCpltCallback` (encadeamento: contador → tabela → rearma RX)

---

## Máquina de Estados do Cliente

```
                      ┌─────────┐
                      │  IDLE   │◄─────────────────────────┐
                      └────┬────┘                          │
               botão       │                               │
               pressionado │                               │
                           ▼                               │
                 envia 0x5A (IT)                           │
                 arma RX contador (IT)                     │
                           │                               │
                           ▼                               │
                  ┌────────────────┐                       │
                  │ WAIT_COUNTER   │                       │
                  └───────┬────────┘                       │
                          │ RxCplt (1 byte)                │
                          ▼                                │
                  arma RX tabela (DMA)                     │
                          │                                │
                          ▼                                │
                  ┌────────────────┐                       │
                  │  WAIT_TABLE    │                       │
                  └───────┬────────┘                       │
                          │ RxCplt (DMA)                   │
                          ▼                                │
                  ┌────────────────┐                       │
                  │   BLINKING     │  N toggles @ 1 Hz     │
                  └───────┬────────┘                       │
                          │ toggles_restantes == 0         │
                          ▼                                │
                  envia msg ao PC (IT)                     │
                          │                                │
                          ▼                                │
                  ┌────────────────┐                       │
                  │ SEND_MSG_PC    │                       │
                  └───────┬────────┘                       │
                          │ TxCplt                         │
                          ▼                                │
                  envia tabela ao PC (DMA)                 │
                          │                                │
                          ▼                                │
                  ┌────────────────┐                       │
                  │ SEND_TABLE_PC  │                       │
                  └───────┬────────┘                       │
                          │ TxCplt (DMA)                   │
                          └────────────────────────────────┘
```

---

## Como Compilar e Gravar

### Pré-requisitos

- [STM32CubeIDE](https://www.st.com/en/development-tools/stm32cubeide.html) (versão 1.x ou superior)
- [STM32CubeMX](https://www.st.com/en/development-tools/stm32cubemx.html) (se quiser regenerar os arquivos `.ioc`)
- Drivers do ST-LINK (geralmente incluídos na instalação do CubeIDE)
- Terminal serial — recomendado: [Tera Term](https://ttssh2.osdn.jp/)

### Passo a passo — Cliente

```bash
# 1. Clone o repositório
git clone <url-do-repo>
cd <repo>/Cliente_L476RG

# 2. No STM32CubeIDE:
#    File → Open Projects from File System → selecione a pasta Cliente_L476RG
#    Aguarde indexação
#    Project → Build Project (Ctrl+B)

# 3. Conecte o cabo mini-USB na NUCLEO-L476RG

# 4. Run → Debug As → STM32 C/C++ Application
#    (ou clique no ícone de Debug)

# 5. Após gravar, pressione Resume (F8) ou pare o debug
```

### Passo a passo — Servidor

```bash
# 1. Abra o projeto Servidor_F407 no STM32CubeIDE
#    File → Open Projects from File System → selecione a pasta Servidor_F407

# 2. Project → Build Project (Ctrl+B)

# 3. Desconecte o cabo do Cliente e conecte-o no mini-USB do Discovery F407

# 4. Run → Debug As → STM32 C/C++ Application

# 5. Após gravar, pare o debug e desconecte o cabo
```

### Observação sobre o firmware do ST-LINK

Em placas Discovery mais antigas, o CubeIDE pode solicitar a atualização do firmware do ST-LINK V2 antes de gravar. Se isso ocorrer:

1. Aceite a atualização (`Help → ST-LINK Upgrade → Open in update mode → Upgrade`)
2. Se falhar, desconecte e reconecte o cabo segurando o botão RESET, depois tente novamente

---

## Como Testar

### 1. Montagem física

Com ambos os projetos gravados, faça as conexões dos jumpers conforme a tabela de [Pinagem e Conexões](#pinagem-e-conexões). Em seguida, conecte o cabo USB apenas no **Cliente (L476RG)** — ele alimentará o servidor via 3V3.

### 2. Abrir o terminal serial

1. Abra o **Tera Term**
2. Selecione **Serial → COMx** (porta identificada como "STMicroelectronics STLink Virtual COM Port" no Gerenciador de Dispositivos)
3. Configure: `Setup → Serial port...`
    - Speed: **115200**
    - Data: **8 bit**
    - Parity: **none**
    - Stop: **1 bit**
    - Flow control: **none**

### 3. Teste funcional

1. Pressione **RESET** na placa L476RG → o terminal deve exibir:
   ```
   Cliente L476RG pronto. Aperte o botao azul.
   ```
2. Pressione o **botão azul (USER)** da Discovery F407 um número **N** de vezes (ex: 5)
   - A cada pressionamento, o LED verde da Discovery alterna
3. Pressione o **botão azul (USER)** da NUCLEO-L476RG **uma vez**
4. Observe o LED verde (LD2) da L476RG piscando **N vezes** a 1 Hz
5. Após o blink, o terminal deve exibir:
   ```
   Numero de eventos = 5
   === Equipe ===
   Guilherme Galindo, 14498024435
   Bruna Cruz,        70907682448
   Vitor Queiroz,     10363931430
   ```
6. Tente pressionar o botão do cliente durante o blink — o comando deve ser **ignorado** (comportamento esperado)

### 4. Testes de robustez

- Repita com diferentes valores de N (1, 3, 10)
- Verifique que o contador é **zerado** no servidor após cada ciclo
- Verifique que novos ciclos funcionam corretamente sem necessidade de reset

---

## Estrutura dos Diretórios

```
.
├── Cliente_L476RG/
│   ├── Core/
│   │   ├── Inc/              # Headers (main.h, stm32l4xx_it.h, etc.)
│   │   └── Src/              # Código fonte (main.c, usart.c, dma.c, gpio.c, etc.)
│   ├── Drivers/              # HAL e CMSIS (STM32L4)
│   ├── Startup/              # startup_stm32l476rgtx.s
│   ├── Cliente_L476RG.ioc    # Configuração do STM32CubeMX
│   └── STM32L476RGTX_FLASH.ld
│
├── Servidor_F407/
│   ├── Core/
│   │   ├── Inc/
│   │   └── Src/
│   ├── Drivers/              # HAL e CMSIS (STM32F4)
│   ├── Startup/              # startup_stm32f407vgtx.s
│   ├── Servidor_F407.ioc
│   └── STM32F407VGTX_FLASH.ld
│
├── docs/                     # (opcional) diagramas, prints, vídeos de demonstração
└── README.md
```

---

## Tecnologias e Ferramentas

- **Linguagem:** C (GNU C11)
- **IDE:** STM32CubeIDE
- **Configurador:** STM32CubeMX
- **HAL:** STM32Cube HAL (STM32L4 e STM32F4)
- **Toolchain:** GNU Arm Embedded Toolchain (`arm-none-eabi-gcc`)
- **Debugger:** ST-LINK V2 (embarcado nas placas)
- **Terminal:** Tera Term (Windows)

---

## Critérios de Avaliação Atendidos

- [x] Funcionamento correto da comunicação entre cliente e servidor via UART
- [x] Implementação correta do incremento e envio do contador no servidor
- [x] Implementação correta da tabela de membros e sua transmissão via DMA
- [x] Funcionamento correto da lógica de piscar o LED e envio de mensagens no cliente
- [x] Implementação do bloqueio de envio enquanto o LED estiver piscando
- [x] Utilização de interrupção externa EXTI com callback
- [x] Utilização de `HAL_UART_Transmit_IT` / `HAL_UART_Receive_IT`
- [x] Utilização de `HAL_UART_Transmit_DMA` / `HAL_UART_Receive_DMA`
- [x] Máquina de estados no cliente para evitar delays em callbacks
- [x] Periféricos não inicializados em modo default

---

## Licença

Projeto desenvolvido para fins acadêmicos no âmbito da disciplina de Sistemas Embarcados da UPE/POLI.