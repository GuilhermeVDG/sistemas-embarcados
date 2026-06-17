# Backend — Monitoramento de Temperatura e CO2 via UART

Backend NestJS para recepção, armazenamento e consulta das medições enviadas pela placa STM32 NUCLEO-L476RG.

## Arquitetura

```
STM32 NUCLEO-L476RG
        |
        | UART / USB Serial (115200 baud)
        v
Backend NestJS (porta 3000)
        |
        | TypeORM
        v
Banco de dados SQLite (data/measurements.db)
```

## Requisitos

- Node.js 18+
- npm ou yarn

## Instalação

```bash
cd backend
npm install
```

## Configuração do `.env`

Copie o arquivo de exemplo e edite conforme seu ambiente:

```bash
cp .env.example .env
```

Variáveis disponíveis:

```env
SERIAL_PORT=COM3                              # Porta serial da placa
SERIAL_BAUDRATE=115200                        # Baudrate (não alterar)
DATABASE_URL=sqlite:///./data/measurements.db # Caminho do banco SQLite
SERIAL_RECONNECT_SECONDS=5                    # Intervalo de reconexão em segundos
```

## Como descobrir a porta serial

**Windows:** Abra o Gerenciador de Dispositivos → Portas (COM e LPT). Exemplo: `COM3`, `COM4`.

**macOS:**
```bash
ls /dev/cu.usbmodem*
```

**Linux:**
```bash
ls /dev/ttyACM*
ls /dev/ttyUSB*
```

## Como rodar

```bash
# Modo desenvolvimento (com hot-reload)
npm run start:dev

# Modo produção
npm run build
npm run start:prod
```

O servidor sobe em `http://localhost:3000`.

## Exemplos de JSON recebidos pela UART

Medição válida:
```json
{"temperature_c":23.5,"co2_ppm":502,"adc_raw":601,"vpin_mv":484,"vao_mv":5324,"sensor_status":"OK"}
```

Erro no sensor de temperatura:
```json
{"temperature_c":null,"co2_ppm":500,"adc_raw":600,"vpin_mv":483,"vao_mv":5313,"sensor_status":"ERRO_I2C"}
```

## API REST

### Health check
```
GET /health
```
```json
{ "status": "ok" }
```

### Criar medição manualmente
```
POST /measurements
```
```json
{
  "temperature_c": 23.5,
  "co2_ppm": 502,
  "adc_raw": 601,
  "vpin_mv": 484,
  "vao_mv": 5324,
  "sensor_status": "OK",
  "source": "manual"
}
```

### Listar histórico
```
GET /measurements?limit=100&offset=0&start_date=2026-06-01T00:00:00&end_date=2026-06-15T23:59:59
```

### Última medição
```
GET /measurements/latest
```

### Estatísticas
```
GET /measurements/stats?start_date=2026-06-01T00:00:00
```
```json
{
  "count": 120,
  "temperature": { "min": 23.5, "max": 25.0, "avg": 24.4 },
  "co2_ppm": { "min": 450, "max": 906, "avg": 532 }
}
```

## Observação sobre `co2_ppm`

O campo `co2_ppm` representa uma **estimativa didática** obtida a partir da leitura analógica do sensor MQ-135/FC-22. O MQ-135 não é um sensor NDIR específico para CO2 e não foi calibrado laboratorialmente. O valor serve para demonstrar tendência de variação conforme o ambiente ao redor do sensor muda, e **não deve ser tratado como medição certificada**.

## Possíveis problemas

| Problema | Solução |
|---|---|
| `Error: No such file or directory, cannot open /dev/ttyACM0` | Verifique se a placa está conectada e confira o `SERIAL_PORT` no `.env` |
| `Error: Access denied` (Linux/macOS) | Execute `sudo usermod -a -G dialout $USER` e reinicie a sessão |
| Banco não cria automaticamente | Verifique se a pasta `data/` existe. O TypeORM cria as tabelas, mas não a pasta |
| Nenhuma medição aparece na API | Verifique se o firmware está enviando JSON com os campos corretos no baudrate 115200 |
