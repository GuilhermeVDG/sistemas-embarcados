import { IsDateString, IsInt, IsOptional, Max, Min } from 'class-validator';
import { Type } from 'class-transformer';
import { ApiPropertyOptional } from '@nestjs/swagger';

export class QueryMeasurementsDto {
  @ApiPropertyOptional({ example: 100, default: 100 })
  @IsOptional()
  @Type(() => Number)
  @IsInt()
  @Min(1)
  @Max(1000)
  limit: number = 100;

  @ApiPropertyOptional({ example: 0, default: 0 })
  @IsOptional()
  @Type(() => Number)
  @IsInt()
  @Min(0)
  offset: number = 0;

  @ApiPropertyOptional({ example: '2026-06-01T00:00:00', description: 'Data inicial (ISO 8601).' })
  @IsOptional()
  @IsDateString()
  start_date?: string;

  @ApiPropertyOptional({ example: '2026-06-16T23:59:59', description: 'Data final (ISO 8601).' })
  @IsOptional()
  @IsDateString()
  end_date?: string;
}
