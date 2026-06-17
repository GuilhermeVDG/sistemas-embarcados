import {
  Injectable,
  OnApplicationBootstrap,
  OnApplicationShutdown,
} from '@nestjs/common';
import { ConfigService } from '@nestjs/config';
import { SerialPort } from 'serialport';
import { ReadlineParser } from '@serialport/parser-readline';
import { MeasurementsService } from '../measurements/measurements.service';
import { parseMeasurementLine } from './parser';

@Injectable()
export class SerialService implements OnApplicationBootstrap, OnApplicationShutdown {
  private port: SerialPort | null = null;
  private reconnectTimer: NodeJS.Timeout | null = null;
  private reconnectScheduled = false;
  private isShuttingDown = false;

  constructor(
    private readonly config: ConfigService,
    private readonly measurementsService: MeasurementsService,
  ) {}

  onApplicationBootstrap() {
    this.connect();
  }

  onApplicationShutdown() {
    this.isShuttingDown = true;
    if (this.reconnectTimer) clearTimeout(this.reconnectTimer);
    if (this.port?.isOpen) this.port.close();
  }

  private connect(): void {
    const portPath = this.config.get<string>('SERIAL_PORT', 'COM3');
    const baudRate = parseInt(this.config.get<string>('SERIAL_BAUDRATE', '115200'));
    const reconnectSeconds = parseInt(this.config.get<string>('SERIAL_RECONNECT_SECONDS', '5'));

    console.log(`[serial] Tentando conectar em ${portPath} @ ${baudRate}`);

    try {
      const port = new SerialPort({ path: portPath, baudRate });

      port.on('open', () => {
        console.log('[serial] Conectado');
        this.port = port;
        this.reconnectScheduled = false;

        const parser = port.pipe(new ReadlineParser({ delimiter: '\n' }));
        parser.on('data', (line: string) => this.handleLine(line));
      });

      port.on('error', () => {
        console.log(
          `[serial] Erro ao abrir porta ${portPath}. Tentando novamente em ${reconnectSeconds}s...`,
        );
        this.scheduleReconnect(reconnectSeconds);
      });

      port.on('close', () => {
        if (!this.isShuttingDown) {
          console.log(
            `[serial] Conexão perdida. Tentando reconectar em ${reconnectSeconds}s`,
          );
          this.scheduleReconnect(reconnectSeconds);
        }
      });
    } catch {
      console.log(
        `[serial] Erro ao abrir porta ${portPath}. Tentando novamente em ${reconnectSeconds}s...`,
      );
      this.scheduleReconnect(reconnectSeconds);
    }
  }

  private scheduleReconnect(seconds: number): void {
    if (this.isShuttingDown || this.reconnectScheduled) return;
    this.reconnectScheduled = true;
    this.reconnectTimer = setTimeout(() => {
      this.reconnectScheduled = false;
      this.connect();
    }, seconds * 1000);
  }

  private async handleLine(line: string): Promise<void> {
    const trimmed = line.trim();
    console.log(`[serial] Linha recebida: ${trimmed}`);

    const parsed = parseMeasurementLine(trimmed);
    if (!parsed) return;

    console.log('[parser] Medição válida');
    try {
      await this.measurementsService.createFromSerial(parsed, trimmed);
    } catch (err) {
      console.error('[db] Erro ao salvar medição:', err);
    }
  }
}
