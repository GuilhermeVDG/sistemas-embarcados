import { Module } from '@nestjs/common';
import { SerialService } from './serial.service';
import { MeasurementsModule } from '../measurements/measurements.module';

@Module({
  imports: [MeasurementsModule],
  providers: [SerialService],
})
export class SerialModule {}
