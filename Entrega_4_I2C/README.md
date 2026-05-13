# Entrega 4 - Comunicacao I2C com PCF8591

## Objetivo

Implementar uma comunicacao I2C entre a placa **NUCLEO-L476RG** e o modulo **PCF8591**, usando comandos enviados pelo terminal serial para:

- Ler entradas analogicas do PCF8591.
- Enviar valores para o DAC do PCF8591.
- Exibir no terminal o resultado das operacoes.
- Usar chamadas nao bloqueantes da HAL para a comunicacao I2C.

---

## Hardware Utilizado

| Item | Descricao | Uso no projeto |
|------|-----------|----------------|
| NUCLEO-L476RG | Placa principal STM32 | Cliente/master I2C e interface com o PC |
| PCF8591 | Modulo ADC/DAC I2C | Servidor/slave I2C |
| STM32F4 Discovery | Outra placa STM32 | Nao utilizada nesta entrega |

---

## Ligacoes Fisicas

| PCF8591 | NUCLEO-L476RG |
|---------|---------------|
| VCC | 3V3 |
| GND | GND |
| SCL | SCL/D15 |
| SDA | SDA/D14 |

Use **3V3** para manter o modulo compativel com os niveis logicos da NUCLEO-L476RG.

### Observacao sobre D14 e D15

No STM32CubeMX, os pinos normalmente aparecem pelo nome real do microcontrolador, e nao pelo nome Arduino impresso na placa. Entao:

| Nome na placa | Nome no STM32CubeMX | Funcao |
|---------------|---------------------|--------|
| D15 / SCL | PB8 | I2C1_SCL |
| D14 / SDA | PB9 | I2C1_SDA |

Portanto, selecionar **PB8** e **PB9** no CubeMX esta correto.

---

## Configuracao do Modulo PCF8591

| Jumper | Funcao | Estado |
|--------|--------|--------|
| J5 | LDR no AIN0 | Conectado |
| J4 | Termistor no AIN1 | Conectado |
| J6 | Potenciometro no AIN3 | Conectado |

O canal AIN2 nao foi usado nesta entrega.

---

## Configuracao no STM32CubeMX / STM32CubeIDE

### Placa

Projeto criado para:

```text
NUCLEO-L476RG
```

### I2C1

| Parametro | Valor |
|-----------|-------|
| Periferico | I2C1 |
| Modo | I2C |
| Papel da NUCLEO | Master |
| Velocidade | Standard Mode |
| Frequencia | 100 kHz |
| SCL | PB8 / D15 |
| SDA | PB9 / D14 |

O PCF8591 trabalha no barramento I2C com endereco base `0x48`. Na HAL do STM32 o endereco de 7 bits e passado deslocado para a esquerda:

```c
#define PCF8591_ADDR (0x48 << 1)
```

### UART

O projeto possui `USART2` e `USART3` configuradas, ambas em:

| Parametro | Valor |
|-----------|-------|
| Baud rate | 115200 |
| Word length | 8 bits |
| Parity | None |
| Stop bits | 1 |
| Modo | TX/RX |

Embora a especificacao da atividade cite `USART3`, o codigo atual usa **USART2** para o terminal porque a NUCLEO-L476RG normalmente conecta a porta USB/ST-LINK do PC na `USART2`.

No codigo:

```c
HAL_UART_Receive_IT(&huart2, &uart_rx_byte, 1);
```

e as mensagens de debug sao enviadas por:

```c
HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), 100);
```

### NVIC

Interrupcoes habilitadas no projeto:

| Interrupcao | Uso |
|-------------|-----|
| I2C1_EV_IRQn | Eventos de transmissao/recepcao I2C |
| I2C1_ER_IRQn | Tratamento de erro I2C |
| USART2_IRQn | Recepcao de comandos pelo terminal |
| USART3_IRQn | Mantida configurada no projeto |

---

## Comandos do Terminal

Abra um terminal serial em:

```text
115200 baud
8 bits
No parity
1 stop bit
```

Comandos implementados:

| Comando | Acao esperada |
|---------|---------------|
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

## Resultado Esperado

Ao iniciar o sistema, o terminal deve mostrar:

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

### Teste 1 - Leitura do LDR

Comando:

```text
Read_AIN0
```

Saida esperada:

```text
Comando recebido: Read_AIN0
Lendo AIN0...
AIN0: <valor>
```

O valor deve mudar ao tampar ou iluminar o LDR.

### Teste 2 - Leitura do termistor

Comando:

```text
Read_AIN1
```

Saida esperada:

```text
Comando recebido: Read_AIN1
Lendo AIN1...
AIN1: <valor>
```

O valor deve mudar ao aquecer o termistor com o dedo por alguns segundos.

### Teste 3 - Leitura do potenciometro

Comando:

```text
Read_AIN3
```

Saida esperada:

```text
Comando recebido: Read_AIN3
Lendo AIN3...
AIN3: <valor>
```

Ao girar o trimpot branco do modulo PCF8591, o valor deve variar entre proximo de `0` e proximo de `255`.

### Teste 4 - Escrita no DAC

Comando:

```text
Set_DAC_128
```

Saida esperada:

```text
Comando recebido: Set_DAC_128
Enviando DAC: 128
Valor do DAC confirmado: 128
```

Com alimentacao em `3V3`, a saida `AOUT` deve ficar aproximadamente em metade da alimentacao:

```text
AOUT ~= 1,65 V
```

---

## Implementacao

### Defines principais

```c
#define PCF8591_ADDR       (0x48 << 1)

#define PCF8591_CH0        0x00
#define PCF8591_CH1        0x01
#define PCF8591_CH3        0x03
#define PCF8591_DAC_ENABLE 0x40

#define RX_BUFFER_SIZE     64
```

### Inicializacao

No inicio da aplicacao, o codigo:

- Inicializa GPIO, USART2, I2C1 e USART3.
- Pisca o LED `LD2` tres vezes.
- Envia um menu de comandos pelo terminal.
- Inicia a recepcao por interrupcao na `USART2`.

```c
blink_startup();
HAL_UART_Receive_IT(&huart2, &uart_rx_byte, 1);
```

### Processamento de comandos

A funcao `process_command()` remove quebras de linha e espacos finais, imprime o comando recebido e executa a acao correspondente:

```c
if (strcmp(cmd, "Read_AIN0") == 0)
{
  pcf8591_read_channel(PCF8591_CH0);
}
else if (strncmp(cmd, "Set_DAC_", 8) == 0)
{
  pcf8591_set_dac((uint8_t)dac_value);
}
```

### Leitura ADC do PCF8591

Para ler um canal analogico:

1. O codigo envia o byte de controle com o canal desejado.
2. Ao terminar a transmissao I2C, o callback inicia a recepcao.
3. Sao lidos dois bytes.
4. O segundo byte e usado como valor valido.

```c
HAL_I2C_Master_Transmit_IT(&hi2c1, PCF8591_ADDR, i2c_tx_buffer, 1);
HAL_I2C_Master_Receive_IT(&hi2c1, PCF8591_ADDR, i2c_rx_buffer, 2);
```

O segundo byte e usado porque o PCF8591 pode retornar primeiro o resultado da conversao anterior:

```c
uint8_t value = i2c_rx_buffer[1];
```

### Escrita DAC

Para escrever no DAC:

1. O primeiro byte habilita o DAC.
2. O segundo byte contem o valor de saida.
3. O valor aceito vai de `0` a `255`.

```c
i2c_tx_buffer[0] = PCF8591_DAC_ENABLE;
i2c_tx_buffer[1] = value;

HAL_I2C_Master_Transmit_IT(&hi2c1, PCF8591_ADDR, i2c_tx_buffer, 2);
```

---

## Decisoes Tecnicas

- Foi usado `I2C1` como master porque o PCF8591 trabalha como dispositivo slave no barramento I2C.
- O endereco `0x48` foi deslocado com `<< 1` para seguir o formato esperado pela HAL.
- A leitura ADC usa dois bytes e considera o segundo para evitar usar uma conversao antiga do PCF8591.
- As operacoes I2C usam funcoes com final `_IT`, como `HAL_I2C_Master_Transmit_IT()` e `HAL_I2C_Master_Receive_IT()`, para atender ao requisito de comunicacao nao bloqueante.
- A comunicacao de terminal foi feita pela `USART2`, pois ela e a UART ligada ao ST-LINK/USB da NUCLEO-L476RG e permite testar diretamente pelo cabo USB.
- A `USART3` foi mantida configurada no projeto, mas nao e a UART usada pelo `main.c` atual para comandos.
- O LED `LD2` pisca na inicializacao e tambem alterna a cada byte recebido por UART, servindo como prova visual de que o firmware esta rodando e recebendo dados.
- O tratamento de erro I2C imprime o codigo retornado por `HAL_I2C_GetError()`, facilitando diagnostico de fiacao, endereco ou ausencia do modulo.

---

## Problemas Comuns

| Problema | Possivel causa | Verificacao |
|----------|----------------|-------------|
| Nada aparece no terminal | Porta COM errada ou UART errada | Usar COM do ST-LINK, 115200 8N1 |
| LED nao pisca ao iniciar | Firmware nao esta executando | Conferir gravacao/debug |
| LED pisca ao digitar, mas nao responde | Enter nao esta sendo enviado | Configurar terminal para enviar CR/LF |
| Erro I2C no terminal | Fios SDA/SCL invertidos ou modulo sem alimentacao | Conferir PB8/PB9, GND e 3V3 |
| Leitura sempre igual | Jumpers do modulo ou sensor sem variacao | Conferir J4, J5 e J6 |
| DAC nao altera AOUT | Medicao no ponto errado ou valor invalido | Medir AOUT e usar `Set_DAC_128` |

---

## Conclusao

Este projeto demonstra:

- Comunicacao entre STM32 e PCF8591 via I2C.
- Controle por comandos enviados pelo terminal serial.
- Leitura de sensores analogicos pelo ADC do PCF8591.
- Escrita em saida analogica pelo DAC do PCF8591.
- Uso de interrupcoes e callbacks da HAL para comunicacao nao bloqueante.
