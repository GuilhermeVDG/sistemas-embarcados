"use strict";
var __decorate = (this && this.__decorate) || function (decorators, target, key, desc) {
    var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
    else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && Object.defineProperty(target, key, r), r;
};
var __metadata = (this && this.__metadata) || function (k, v) {
    if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
};
Object.defineProperty(exports, "__esModule", { value: true });
exports.SerialService = void 0;
const common_1 = require("@nestjs/common");
const config_1 = require("@nestjs/config");
const serialport_1 = require("serialport");
const parser_readline_1 = require("@serialport/parser-readline");
const measurements_service_1 = require("../measurements/measurements.service");
const parser_1 = require("./parser");
let SerialService = class SerialService {
    constructor(config, measurementsService) {
        this.config = config;
        this.measurementsService = measurementsService;
        this.port = null;
        this.reconnectTimer = null;
        this.reconnectScheduled = false;
        this.isShuttingDown = false;
    }
    onApplicationBootstrap() {
        this.connect();
    }
    onApplicationShutdown() {
        this.isShuttingDown = true;
        if (this.reconnectTimer)
            clearTimeout(this.reconnectTimer);
        if (this.port?.isOpen)
            this.port.close();
    }
    connect() {
        const portPath = this.config.get('SERIAL_PORT', 'COM3');
        const baudRate = parseInt(this.config.get('SERIAL_BAUDRATE', '115200'));
        const reconnectSeconds = parseInt(this.config.get('SERIAL_RECONNECT_SECONDS', '5'));
        console.log(`[serial] Tentando conectar em ${portPath} @ ${baudRate}`);
        try {
            const port = new serialport_1.SerialPort({ path: portPath, baudRate });
            port.on('open', () => {
                console.log('[serial] Conectado');
                this.port = port;
                this.reconnectScheduled = false;
                const parser = port.pipe(new parser_readline_1.ReadlineParser({ delimiter: '\n' }));
                parser.on('data', (line) => this.handleLine(line));
            });
            port.on('error', () => {
                console.log(`[serial] Erro ao abrir porta ${portPath}. Tentando novamente em ${reconnectSeconds}s...`);
                this.scheduleReconnect(reconnectSeconds);
            });
            port.on('close', () => {
                if (!this.isShuttingDown) {
                    console.log(`[serial] Conexão perdida. Tentando reconectar em ${reconnectSeconds}s`);
                    this.scheduleReconnect(reconnectSeconds);
                }
            });
        }
        catch {
            console.log(`[serial] Erro ao abrir porta ${portPath}. Tentando novamente em ${reconnectSeconds}s...`);
            this.scheduleReconnect(reconnectSeconds);
        }
    }
    scheduleReconnect(seconds) {
        if (this.isShuttingDown || this.reconnectScheduled)
            return;
        this.reconnectScheduled = true;
        this.reconnectTimer = setTimeout(() => {
            this.reconnectScheduled = false;
            this.connect();
        }, seconds * 1000);
    }
    async handleLine(line) {
        const trimmed = line.trim();
        console.log(`[serial] Linha recebida: ${trimmed}`);
        const parsed = (0, parser_1.parseMeasurementLine)(trimmed);
        if (!parsed)
            return;
        console.log('[parser] Medição válida');
        try {
            await this.measurementsService.createFromSerial(parsed, trimmed);
        }
        catch (err) {
            console.error('[db] Erro ao salvar medição:', err);
        }
    }
};
exports.SerialService = SerialService;
exports.SerialService = SerialService = __decorate([
    (0, common_1.Injectable)(),
    __metadata("design:paramtypes", [config_1.ConfigService,
        measurements_service_1.MeasurementsService])
], SerialService);
//# sourceMappingURL=serial.service.js.map