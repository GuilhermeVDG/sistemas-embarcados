import { Module } from '@nestjs/common';
import { ConfigModule, ConfigService } from '@nestjs/config';
import { TypeOrmModule } from '@nestjs/typeorm';
import { AppController } from './app.controller';
import { MeasurementsModule } from './measurements/measurements.module';
import { SerialModule } from './serial/serial.module';
import { Measurement } from './measurements/measurement.entity';

@Module({
  imports: [
    ConfigModule.forRoot({ isGlobal: true }),
    TypeOrmModule.forRootAsync({
      inject: [ConfigService],
      useFactory: (config: ConfigService) => {
        const url = config.get<string>('DATABASE_URL', 'sqlite:///./data/measurements.db');
        const dbPath = url.replace(/^sqlite:\/\/\/\.\//, '').replace(/^sqlite:\/\/\//, '');
        console.log('[db] Banco conectado');
        return {
          type: 'better-sqlite3',
          database: dbPath,
          entities: [Measurement],
          synchronize: true,
        };
      },
    }),
    MeasurementsModule,
    SerialModule,
  ],
  controllers: [AppController],
})
export class AppModule {}
