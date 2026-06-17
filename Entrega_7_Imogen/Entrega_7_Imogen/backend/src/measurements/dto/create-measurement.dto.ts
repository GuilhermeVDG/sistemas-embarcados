import { IsInt, IsNumber, IsOptional, IsString, Max, Min } from 'class-validator';
import { Type } from 'class-transformer';
import { ApiProperty, ApiPropertyOptional } from '@nestjs/swagger';

export class CreateMeasurementDto {
  @ApiPropertyOptional({ example: 23.5, description: 'Temperatura em °C. Null se o sensor WCMCU-75 falhar.' })
  @IsOptional()
  @IsNumber()
  temperature_c: number | null;

  @ApiProperty({ example: 502, description: 'Estimativa didática de CO2 em ppm (sensor MQ-135, não calibrado).' })
  @Type(() => Number)
  @IsInt()
  @Min(0)
  co2_ppm: number;

  @ApiProperty({ example: 601, description: 'Valor bruto do ADC (0–4095).' })
  @Type(() => Number)
  @IsInt()
  @Min(0)
  @Max(4095)
  adc_raw: number;

  @ApiPropertyOptional({ example: 484, description: 'Tensão no pino PA0 em mV.' })
  @IsOptional()
  @Type(() => Number)
  @IsInt()
  @Min(0)
  vpin_mv: number | null;

  @ApiPropertyOptional({ example: 5324, description: 'Tensão estimada na saída analógica do MQ-135 antes do divisor, em mV.' })
  @IsOptional()
  @Type(() => Number)
  @IsInt()
  @Min(0)
  vao_mv: number | null;

  @ApiProperty({ example: 'OK', description: 'Status do sensor de temperatura. Valores: OK, ERRO_I2C.' })
  @IsString()
  sensor_status: string;

  @ApiPropertyOptional({ example: 'manual', default: 'manual', description: 'Origem da medição. Use "manual" para inserções via API.' })
  @IsOptional()
  @IsString()
  source: string = 'manual';
}
