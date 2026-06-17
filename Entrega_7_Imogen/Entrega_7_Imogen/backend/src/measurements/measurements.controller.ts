import { Body, Controller, Get, Post, Query } from '@nestjs/common';
import { ApiOperation, ApiTags } from '@nestjs/swagger';
import { MeasurementsService } from './measurements.service';
import { CreateMeasurementDto } from './dto/create-measurement.dto';
import { QueryMeasurementsDto } from './dto/query-measurements.dto';

@ApiTags('measurements')
@Controller('measurements')
export class MeasurementsController {
  constructor(private readonly service: MeasurementsService) {}

  @ApiOperation({ summary: 'Criar medição manualmente' })
  @Post()
  create(@Body() dto: CreateMeasurementDto) {
    return this.service.create(dto);
  }

  @ApiOperation({ summary: 'Retorna a medição mais recente recebida pela UART' })
  @Get('latest')
  findLatest() {
    return this.service.findLatest();
  }

  @ApiOperation({ summary: 'Estatísticas de temperatura e CO2 (min, max, média)' })
  @Get('stats')
  getStats(@Query() query: { start_date?: string; end_date?: string }) {
    return this.service.getStats(query);
  }

  @ApiOperation({ summary: 'Lista o histórico de medições (mais recentes primeiro)' })
  @Get()
  findAll(@Query() query: QueryMeasurementsDto) {
    return this.service.findAll(query);
  }
}
