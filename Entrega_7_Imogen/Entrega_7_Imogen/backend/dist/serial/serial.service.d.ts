import { OnApplicationBootstrap, OnApplicationShutdown } from '@nestjs/common';
import { ConfigService } from '@nestjs/config';
import { MeasurementsService } from '../measurements/measurements.service';
export declare class SerialService implements OnApplicationBootstrap, OnApplicationShutdown {
    private readonly config;
    private readonly measurementsService;
    private port;
    private reconnectTimer;
    private reconnectScheduled;
    private isShuttingDown;
    constructor(config: ConfigService, measurementsService: MeasurementsService);
    onApplicationBootstrap(): void;
    onApplicationShutdown(): void;
    private connect;
    private scheduleReconnect;
    private handleLine;
}
