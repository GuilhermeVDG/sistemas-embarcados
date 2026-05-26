# Projeto - Comunicação SPI com Matriz de LEDs 8x8 e Sensores PCF8591

## 1. Objetivo do Projeto

O objetivo deste projeto é implementar um sistema embarcado usando a placa **NUCLEO-L476RG** para integrar dois módulos externos:

- Um módulo **PCF8591**, responsável pela leitura de sinais analógicos via **I2C**.
- Uma matriz de LEDs **8x8 com driver MAX7219**, controlada via **SPI**.
- Um terminal serial no computador, usado para enviar comandos para a placa via **USART2**.

A proposta é que a NUCLEO funcione como o controlador principal do sistema. Ela recebe comandos pelo terminal, lê os valores analógicos no PCF8591 e exibe informações na matriz de LEDs 8x8.

O projeto simula a leitura de grandezas como:

- Temperatura;
- Tensão;
- Luminosidade.

A partir do valor lido, a matriz deve mostrar a letra correspondente à grandeza e um sinal `+` ou `-`, indicando se o valor está acima ou abaixo do limite definido.

---

## 2. Descrição Geral do Funcionamento

O sistema funciona da seguinte forma:

1. O usuário abre um terminal serial no computador.
2. O usuário envia comandos para a NUCLEO.
3. A NUCLEO interpreta o comando recebido.
4. Caso seja um comando de leitura, a NUCLEO acessa o PCF8591 via I2C.
5. O valor lido é enviado de volta para o terminal.
6. Dependendo do comando, a NUCLEO também atualiza a matriz de LEDs via SPI.
7. A matriz alterna entre uma letra e um sinal a cada 500 ms.

A regra usada para exibição na matriz é:

| Valor lido | Sinal exibido |
|-----------|---------------|
| Menor que 128 | `-` |
| Maior ou igual a 128 | `+` |

Exemplo:

Se o comando enviado for:

```text
Temp
```

E o valor lido for `90`, a matriz alterna entre:

```text
T
-
```

Se o valor lido for `180`, a matriz alterna entre:

```text
T
+
```

---

## 3. Hardware Utilizado

| Item | Descrição | Função no projeto |
|------|-----------|-------------------|
| NUCLEO-L476RG | Placa STM32 principal | Controla todo o sistema |
| PCF8591 | Módulo ADC/DAC I2C | Lê entradas analógicas e gera saída DAC |
| MAX7219 8x8 LED Matrix | Matriz de LEDs com driver SPI | Exibe letras e sinais |
| Jumpers | Fios de conexão | Ligação entre os módulos |
| Cabo USB | Alimentação e comunicação | Conecta a NUCLEO ao computador |
| Terminal serial | Tera Term, PuTTY ou similar | Envio de comandos |

---

## 4. Interfaces de Comunicação Utilizadas

| Comunicação | Periférico na NUCLEO | Módulo conectado | Uso |
|------------|----------------------|------------------|-----|
| I2C | I2C1 | PCF8591 | Leitura ADC e escrita DAC |
| SPI | SPI1 | MAX7219 | Controle da matriz LED |
| UART | USART2 | Computador | Terminal serial |

---

## 5. Ligações Físicas

## 5.1 Ligação do PCF8591

O módulo PCF8591 foi conectado à NUCLEO usando o barramento **I2C1**.

| PCF8591 | NUCLEO-L476RG | Função |
|---------|---------------|--------|
| VCC | 3V3 | Alimentação |
| GND | GND | Terra |
| SCL | D15 / PB8 | Clock I2C |
| SDA | D14 / PB9 | Dados I2C |

### Observação importante

O PCF8591 foi alimentado com **3V3** para manter compatibilidade com os níveis lógicos da STM32.

No CubeIDE, os pinos aparecem pelo nome real do microcontrolador:

| Nome impresso na placa | Nome no CubeIDE | Função |
|------------------------|-----------------|--------|
| D15 / SCL | PB8 | I2C1_SCL |
| D14 / SDA | PB9 | I2C1_SDA |

---

## 5.2 Ligação da Matriz MAX7219

A matriz de LEDs foi conectada usando o barramento **SPI1**.

| MAX7219 | NUCLEO-L476RG | Função |
|---------|---------------|--------|
| VCC | 5V | Alimentação da matriz |
| GND | GND | Terra |
| DIN | D11 / PA7 | SPI1_MOSI |
| CS | D10 / PB6 | Chip Select manual |
| CLK | D13 / PA5 | SPI1_SCK |

### Observação importante

O pino **MISO/D12** não é utilizado, pois o MAX7219 apenas recebe dados da NUCLEO.

A matriz foi alimentada em **5V**, pois módulos com MAX7219 normalmente trabalham nessa tensão.

---

## 6. Configuração no STM32CubeIDE

O projeto foi criado para a placa:

```text
NUCLEO-L476RG
```

---

## 6.1 Configuração do I2C1

Usado para comunicação com o PCF8591.

| Parâmetro | Valor |
|----------|-------|
| Periférico | I2C1 |
| Modo | I2C |
| Papel da NUCLEO | Master |
| Velocidade | Standard Mode |
| Frequência | 100 kHz |
| SCL | PB8 / D15 |
| SDA | PB9 / D14 |

Endereço usado no código:

```c
#define PCF8591_ADDR (0x48 << 1)
```

O endereço base do PCF8591 é `0x48`. Na HAL do STM32, esse endereço precisa ser deslocado para a esquerda com `<< 1`.

---

## 6.2 Configuração do SPI1

Usado para controlar a matriz MAX7219.

| Parâmetro | Valor |
|----------|-------|
| Periférico | SPI1 |
| Modo | Full-Duplex Master |
| Hardware NSS Signal | Disable |
| Data Size | 8 Bits |
| First Bit | MSB First |
| Clock Polarity | Low |
| Clock Phase | 1 Edge |
| Prescaler | 32 |
| NSS Signal Type | Software |
| NSSP Mode | Disabled |

Pinos usados:

| Pino | Função |
|------|--------|
| PA5 | SPI1_SCK |
| PA7 | SPI1_MOSI |
| PA6 | SPI1_MISO, não usado fisicamente |

---

## 6.3 Configuração do CS do MAX7219

O pino **PB6 / D10** foi configurado como GPIO de saída.

| Parâmetro | Valor |
|----------|-------|
| Pino | PB6 |
| Label | MAX7219_CS |
| Modo | GPIO_Output |
| Output Level | High |
| Output Type | Push Pull |
| Pull-up/Pull-down | No pull-up and no pull-down |
| Speed | Low |

O CS é controlado manualmente no código.

Fluxo de envio para a matriz:

```c
HAL_GPIO_WritePin(MAX7219_CS_GPIO_Port, MAX7219_CS_Pin, GPIO_PIN_RESET);
HAL_SPI_Transmit(&hspi1, tx_data, 2, HAL_MAX_DELAY);
HAL_GPIO_WritePin(MAX7219_CS_GPIO_Port, MAX7219_CS_Pin, GPIO_PIN_SET);
```

Ou seja:

| CS | Significado |
|----|-------------|
| LOW | Inicia envio |
| HIGH | Finaliza e grava os dados no MAX7219 |

---

## 6.4 Configuração da USART2

A USART2 foi usada para comunicação com o terminal pelo USB da NUCLEO.

| Parâmetro | Valor |
|----------|-------|
| Periférico | USART2 |
| Modo | Asynchronous |
| Baud Rate | 115200 |
| Word Length | 8 Bits |
| Parity | None |
| Stop Bits | 1 |
| Data Direction | Receive and Transmit |
| TX | PA2 |
| RX | PA3 |

A atividade original cita USART3, porém neste projeto foi usada a **USART2**, pois na NUCLEO-L476RG ela é a UART conectada ao ST-LINK/USB da placa.

---

# 7. Comandos Implementados

O sistema aceita comandos enviados pelo terminal serial.

Configuração do terminal:

```text
Baud rate: 115200
Data bits: 8
Parity: None
Stop bits: 1
Line ending: CR/LF ou Enter
```

## 7.1 Comandos de leitura direta

| Comando | Função |
|---------|--------|
| `Read_AIN0` | Lê o canal AIN0 do PCF8591 |
| `Read_AIN1` | Lê o canal AIN1 do PCF8591 |
| `Read_AIN3` | Lê o canal AIN3 do PCF8591 |

Exemplo:

```text
Read_AIN0
```

Saída esperada:

```text
Comando recebido: Read_AIN0
AIN0 = 127
```

---

## 7.2 Comandos de exibição na matriz

| Comando | Canal lido | Exibição |
|---------|------------|----------|
| `Temp` | AIN0 | Alterna `T` com `+` ou `-` |
| `Volt` | AIN3 | Alterna `V` com `+` ou `-` |
| `LDR` | AIN1 | Alterna `L` com `+` ou `-` |

Mapeamento usado no caso de teste:

| Grandeza | Canal |
|----------|-------|
| Temperatura | AIN0 |
| Luminosidade | AIN1 |
| Tensão | AIN3 |

---

## 7.3 Comandos do DAC

O DAC do PCF8591 pode ser configurado com comandos no formato:

```text
Set_DAC_<valor>
```

O valor deve estar entre `0` e `255`.

Exemplos:

```text
Set_DAC_0
Set_DAC_128
Set_DAC_255
```

Com alimentação em 3V3, a saída aproximada esperada é:

| Comando | Valor DAC | Tensão aproximada |
|---------|-----------|-------------------|
| `Set_DAC_0` | 0 | 0 V |
| `Set_DAC_128` | 128 | 1,65 V |
| `Set_DAC_255` | 255 | 3,3 V |

---

## 7.4 Comandos auxiliares

| Comando | Função |
|---------|--------|
| `Test` | Exibe um padrão quadriculado na matriz |
| `Clear` | Apaga todos os LEDs da matriz |

---

# 8. Casos de Teste

Esta seção descreve os testes usados para validar o projeto.

---

## Caso de Teste 1 - Inicialização do sistema

### Objetivo

Verificar se a NUCLEO inicializa corretamente, se a UART está funcionando e se o menu inicial aparece no terminal.

### Passos

1. Conectar a NUCLEO ao computador via USB.
2. Abrir o terminal serial.
3. Configurar o terminal em `115200 8N1`.
4. Resetar a placa.

### Resultado esperado

O terminal deve exibir:

```text
========================================
NUCLEO-L476RG + PCF8591 + MAX7219
Caso de teste iniciado
Baud: 115200
Digite um comando e pressione ENTER
Comandos:
  Read_AIN0
  Read_AIN1
  Read_AIN3
  Set_DAC_128
  Temp
  Volt
  LDR
  Clear
  Test
========================================
```

### Critério de sucesso

O teste é considerado aprovado se o menu aparecer corretamente no terminal.

---

## Caso de Teste 2 - Teste da comunicação SPI com MAX7219

### Objetivo

Validar se a matriz LED está corretamente conectada e se a comunicação SPI está funcionando.

### Comando

```text
Test
```

### Resultado esperado no terminal

```text
Comando recebido: Test
Padrao de teste enviado para matriz
```

### Resultado esperado na matriz

A matriz deve exibir um padrão quadriculado.

### Critério de sucesso

O teste é aprovado se a matriz acender com o padrão de teste.

### Possíveis falhas

| Falha | Possível causa |
|------|----------------|
| Matriz apagada | VCC/GND incorretos |
| Matriz não responde | DIN, CLK ou CS ligados errado |
| Matriz pisca estranho | SPI muito rápido ou mau contato |
| Padrão aparece invertido | Orientação física da matriz |

---

## Caso de Teste 3 - Apagar matriz LED

### Objetivo

Validar se a NUCLEO consegue limpar todos os LEDs da matriz.

### Comando

```text
Clear
```

### Resultado esperado no terminal

```text
Comando recebido: Clear
Matriz apagada
```

### Resultado esperado na matriz

Todos os LEDs devem apagar.

### Critério de sucesso

O teste é aprovado se a matriz ficar completamente apagada.

---

## Caso de Teste 4 - Leitura do canal AIN0

### Objetivo

Validar a leitura do canal analógico AIN0 do PCF8591.

### Comando

```text
Read_AIN0
```

### Resultado esperado

```text
Comando recebido: Read_AIN0
AIN0 = <valor>
```

O valor deve estar entre:

```text
0 e 255
```

### Critério de sucesso

O teste é aprovado se o terminal exibir um valor numérico válido.

---

## Caso de Teste 5 - Leitura do canal AIN1

### Objetivo

Validar a leitura do canal analógico AIN1 do PCF8591.

### Comando

```text
Read_AIN1
```

### Resultado esperado

```text
Comando recebido: Read_AIN1
AIN1 = <valor>
```

O valor deve estar entre:

```text
0 e 255
```

### Critério de sucesso

O teste é aprovado se o terminal exibir um valor numérico válido.

---

## Caso de Teste 6 - Leitura do canal AIN3

### Objetivo

Validar a leitura do canal analógico AIN3 do PCF8591.

### Comando

```text
Read_AIN3
```

### Resultado esperado

```text
Comando recebido: Read_AIN3
AIN3 = <valor>
```

O valor deve estar entre:

```text
0 e 255
```

### Critério de sucesso

O teste é aprovado se o terminal exibir um valor numérico válido.

---

## Caso de Teste 7 - Escrita no DAC

### Objetivo

Validar se a NUCLEO consegue enviar um valor para a saída analógica DAC do PCF8591.

### Comando

```text
Set_DAC_128
```

### Resultado esperado no terminal

```text
Comando recebido: Set_DAC_128
DAC configurado para 128
```

### Resultado esperado no hardware

A saída `AOUT` do PCF8591 deve ficar próxima de:

```text
1,65 V
```

considerando alimentação em 3V3.

### Critério de sucesso

O teste é aprovado se o terminal confirmar a configuração do DAC e, caso medido com multímetro, `AOUT` apresentar valor próximo da tensão esperada.

---

## Caso de Teste 8 - Comando Temp

### Objetivo

Validar a integração entre:

- UART;
- I2C;
- Leitura do PCF8591;
- SPI;
- Exibição na matriz MAX7219.

### Comando

```text
Temp
```

### Ação esperada

A NUCLEO deve:

1. Receber o comando `Temp`.
2. Ler o canal `AIN0`.
3. Mostrar o valor no terminal.
4. Comparar o valor com `128`.
5. Exibir na matriz a letra `T`.
6. Alternar a cada 500 ms entre `T` e `+` ou `-`.

### Resultado esperado no terminal

```text
Comando recebido: Temp
Temperatura/AIN0 = <valor>
Matriz: T alternando com + ou -
```

### Resultado esperado na matriz

Se `<valor> < 128`:

```text
T
-
```

Se `<valor> >= 128`:

```text
T
+
```

### Critério de sucesso

O teste é aprovado se o valor for exibido no terminal e a matriz alternar corretamente entre a letra `T` e o sinal esperado.

---

## Caso de Teste 9 - Comando Volt

### Objetivo

Validar a leitura da tensão no PCF8591 e a exibição correspondente na matriz.

### Comando

```text
Volt
```

### Ação esperada

A NUCLEO deve:

1. Receber o comando `Volt`.
2. Ler o canal `AIN3`.
3. Mostrar o valor no terminal.
4. Comparar o valor com `128`.
5. Exibir na matriz a letra `V`.
6. Alternar a cada 500 ms entre `V` e `+` ou `-`.

### Resultado esperado no terminal

```text
Comando recebido: Volt
Tensao/AIN3 = <valor>
Matriz: V alternando com + ou -
```

### Resultado esperado na matriz

Se `<valor> < 128`:

```text
V
-
```

Se `<valor> >= 128`:

```text
V
+
```

### Critério de sucesso

O teste é aprovado se o valor for exibido no terminal e a matriz alternar corretamente entre `V` e o sinal esperado.

---

## Caso de Teste 10 - Comando LDR

### Objetivo

Validar a leitura de luminosidade no PCF8591 e a exibição correspondente na matriz.

### Comando

```text
LDR
```

### Ação esperada

A NUCLEO deve:

1. Receber o comando `LDR`.
2. Ler o canal `AIN1`.
3. Mostrar o valor no terminal.
4. Comparar o valor com `128`.
5. Exibir na matriz a letra `L`.
6. Alternar a cada 500 ms entre `L` e `+` ou `-`.

### Resultado esperado no terminal

```text
Comando recebido: LDR
Luminosidade/AIN1 = <valor>
Matriz: L alternando com + ou -
```

### Resultado esperado na matriz

Se `<valor> < 128`:

```text
L
-
```

Se `<valor> >= 128`:

```text
L
+
```

### Critério de sucesso

O teste é aprovado se o valor for exibido no terminal e a matriz alternar corretamente entre `L` e o sinal esperado.

---

# 9. Explicação da Implementação

## 9.1 Inicialização do sistema

No início do `main`, são inicializados:

```c
MX_GPIO_Init();
MX_USART2_UART_Init();
MX_I2C1_Init();
MX_SPI1_Init();
```

Depois disso, a matriz é inicializada:

```c
MAX7219_Init();
```

E o menu inicial é enviado para o terminal.

---

## 9.2 Inicialização do MAX7219

O MAX7219 precisa ser configurado antes de exibir qualquer coisa.

A função `MAX7219_Init()` faz:

| Registrador | Valor | Função |
|-------------|-------|--------|
| Display Test | `0x00` | Desliga teste |
| Decode Mode | `0x00` | Modo no-decode |
| Scan Limit | `0x07` | Usa 8 linhas |
| Intensity | `0x03` | Brilho baixo/médio |
| Shutdown | `0x01` | Liga o display |

Depois disso, a matriz é limpa.

---

## 9.3 Escrita no MAX7219

A escrita no MAX7219 é feita por SPI, enviando dois bytes:

```text
Byte 1: endereço do registrador
Byte 2: dado
```

A função usada é:

```c
static void MAX7219_Write(uint8_t reg, uint8_t data)
```

Essa função baixa o CS, envia os dados e depois levanta o CS.

---

## 9.4 Exibição dos caracteres

As letras e sinais foram definidos como vetores de 8 bytes.

Exemplo:

```c
static const uint8_t CHAR_PLUS[8] =
{
  0b00000000,
  0b00011000,
  0b00011000,
  0b01111110,
  0b01111110,
  0b00011000,
  0b00011000,
  0b00000000
};
```

Cada byte representa uma linha da matriz.

---

## 9.5 Leitura do PCF8591

A função de leitura é:

```c
static HAL_StatusTypeDef PCF8591_ReadChannel(uint8_t channel, uint8_t *value)
```

Ela executa:

1. Seleciona o canal desejado.
2. Envia o byte de controle via I2C.
3. Lê dois bytes.
4. Usa o segundo byte como leitura válida.

O segundo byte é usado porque o primeiro pode representar uma conversão anterior.

---

## 9.6 Escrita no DAC

A função de escrita no DAC é:

```c
static HAL_StatusTypeDef PCF8591_SetDAC(uint8_t value)
```

Ela envia dois bytes:

```text
Byte 1: habilita DAC
Byte 2: valor de saída
```

O valor aceito é de `0` a `255`.

---

## 9.7 Processamento de comandos

Os comandos são tratados pela função:

```c
static void Process_Command(char *cmd)
```

Ela compara o texto recebido e executa a ação correspondente.

Exemplo:

```c
if (strcmp(cmd, "Read_AIN0") == 0)
{
  PCF8591_ReadChannel(0, &value);
}
else if (strcmp(cmd, "Temp") == 0)
{
  PCF8591_ReadChannel(0, &value);
  current_mode = DISPLAY_TEMP;
}
```

---

## 9.8 Atualização da matriz

A alternância dos caracteres é feita pela função:

```c
static void Display_UpdateTask(void)
```

Ela é chamada continuamente dentro do `while(1)`.

O tempo é controlado por:

```c
HAL_GetTick()
```

Quando passam 500 ms, a matriz alterna entre a letra e o sinal.

---

# 10. Decisões Técnicas

- A NUCLEO-L476RG foi usada como placa principal porque era a placa disponível no laboratório.
- A USART2 foi usada no lugar da USART3 porque é a UART conectada ao USB/ST-LINK da NUCLEO-L476RG.
- O PCF8591 foi alimentado em 3V3 para compatibilidade lógica.
- A matriz MAX7219 foi alimentada em 5V, pois é a tensão comum de operação do módulo.
- O SPI foi configurado com prescaler 32 para evitar comunicação rápida demais durante os testes.
- O CS do MAX7219 foi controlado manualmente por GPIO.
- O PCF8591 usa endereço `0x48`, deslocado para a esquerda para uso com a HAL.
- O limite `128` foi usado porque o PCF8591 retorna valores de 8 bits, de `0` a `255`.
- A alternância de caracteres foi feita sem `HAL_Delay()` dentro do loop principal, usando `HAL_GetTick()`, para manter o sistema responsivo.
- O comando `Test` foi criado para validar rapidamente a matriz antes de testar o sistema completo.
- O comando `Clear` foi criado para facilitar a limpeza da matriz durante os testes.

---

# 11. Problemas Comuns e Soluções

| Problema | Possível causa | Solução |
|----------|----------------|---------|
| Nada aparece no terminal | Porta COM errada | Selecionar a COM da ST-LINK |
| Caracteres estranhos no terminal | Baud rate incorreto | Usar 115200 |
| Comando não executa | Terminal não envia Enter corretamente | Configurar CR/LF |
| Erro ao ler PCF8591 | SDA/SCL invertidos | Conferir D14 e D15 |
| PCF8591 não responde | Sem GND comum | Conferir GND entre os módulos |
| Matriz não acende | VCC errado | Ligar VCC da matriz em 5V |
| Matriz não responde | DIN, CLK ou CS errados | Conferir D11, D13 e D10 |
| Matriz mostra padrão invertido | Orientação da matriz | Ajustar os bitmaps no código |
| Valor do DAC não muda | Comando inválido | Usar `Set_DAC_0` até `Set_DAC_255` |
| Valor lido não varia | Sensor/jumper do PCF8591 | Conferir jumpers do módulo |
| LED da placa pisca junto | PA5/D13 também é ligado ao LD2 | Comportamento esperado |

---

# 12. Checklist de Entrega

Antes de apresentar, validar:

- [ ] Projeto compila sem erro.
- [ ] USART2 funciona no terminal em 115200.
- [ ] Menu inicial aparece ao resetar a placa.
- [ ] PCF8591 está alimentado em 3V3.
- [ ] PCF8591 usa D15 para SCL e D14 para SDA.
- [ ] MAX7219 está alimentado em 5V.
- [ ] MAX7219 usa D13 para CLK.
- [ ] MAX7219 usa D11 para DIN.
- [ ] MAX7219 usa D10 para CS.
- [ ] Comando `Test` acende a matriz.
- [ ] Comando `Clear` apaga a matriz.
- [ ] `Read_AIN0` retorna valor entre 0 e 255.
- [ ] `Read_AIN1` retorna valor entre 0 e 255.
- [ ] `Read_AIN3` retorna valor entre 0 e 255.
- [ ] `Set_DAC_128` confirma escrita no DAC.
- [ ] `Temp` alterna `T` com `+` ou `-`.
- [ ] `Volt` alterna `V` com `+` ou `-`.
- [ ] `LDR` alterna `L` com `+` ou `-`.

---

# 13. Conclusão

Este projeto demonstra a integração de diferentes interfaces de comunicação em um sistema embarcado com STM32.

Foram utilizados:

- **I2C**, para comunicação com o PCF8591;
- **SPI**, para controle da matriz de LEDs MAX7219;
- **UART**, para interação com o usuário via terminal serial.

O sistema permite ler canais analógicos, configurar uma saída DAC e exibir informações visuais na matriz de LEDs 8x8. Os casos de teste validam separadamente cada parte do projeto e também o funcionamento integrado entre terminal, sensor e display.

Com isso, o projeto atende ao objetivo de controlar uma matriz de LEDs via SPI com base em leituras obtidas por sensores conectados ao PCF8591.