import { Injectable, NotFoundException } from '@nestjs/common';
import { InjectRepository } from '@nestjs/typeorm';
import { Between, LessThanOrEqual, MoreThanOrEqual, Repository } from 'typeorm';
import { Measurement } from './measurement.entity';
import { CreateMeasurementDto } from './dto/create-measurement.dto';
import { QueryMeasurementsDto } from './dto/query-measurements.dto';
import { ParsedMeasurement } from '../serial/parser';

@Injectable()
export class MeasurementsService {
  constructor(
    @InjectRepository(Measurement)
    private readonly repo: Repository<Measurement>,
  ) {}

  async create(dto: CreateMeasurementDto): Promise<Measurement> {
    const measurement = this.repo.create({ ...dto, timestamp: new Date() });
    const saved = await this.repo.save(measurement);
    console.log(`[db] Medição salva id=${saved.id}`);
    return saved;
  }

  async createFromSerial(parsed: ParsedMeasurement, rawLine: string): Promise<Measurement> {
    const measurement = this.repo.create({
      ...parsed,
      source: 'uart',
      raw_line: rawLine,
      timestamp: new Date(),
    });
    const saved = await this.repo.save(measurement);
    console.log(`[db] Medição salva id=${saved.id}`);
    return saved;
  }

  async findAll(query: QueryMeasurementsDto): Promise<Measurement[]> {
    const { limit = 100, offset = 0, start_date, end_date } = query;

    const where: any = {};
    if (start_date && end_date) {
      where.timestamp = Between(new Date(start_date), new Date(end_date));
    } else if (start_date) {
      where.timestamp = MoreThanOrEqual(new Date(start_date));
    } else if (end_date) {
      where.timestamp = LessThanOrEqual(new Date(end_date));
    }

    return this.repo.find({
      where,
      order: { timestamp: 'DESC' },
      take: limit,
      skip: offset,
    });
  }

  async findLatest(): Promise<Measurement> {
    const [measurement] = await this.repo.find({
      order: { timestamp: 'DESC' },
      take: 1,
    });
    if (!measurement) throw new NotFoundException('Nenhuma medição encontrada');
    return measurement;
  }

  async getStats(query: { start_date?: string; end_date?: string }) {
    const { start_date, end_date } = query;

    const countQb = this.repo.createQueryBuilder('m');
    if (start_date) countQb.andWhere('m.timestamp >= :s', { s: new Date(start_date) });
    if (end_date) countQb.andWhere('m.timestamp <= :e', { e: new Date(end_date) });
    const count = await countQb.getCount();

    if (count === 0) {
      return {
        count: 0,
        temperature: { min: null, max: null, avg: null },
        co2_ppm: { min: null, max: null, avg: null },
      };
    }

    const tempQb = this.repo
      .createQueryBuilder('m')
      .select('MIN(m.temperature_c)', 'min')
      .addSelect('MAX(m.temperature_c)', 'max')
      .addSelect('AVG(m.temperature_c)', 'avg')
      .where('m.temperature_c IS NOT NULL');
    if (start_date) tempQb.andWhere('m.timestamp >= :s', { s: new Date(start_date) });
    if (end_date) tempQb.andWhere('m.timestamp <= :e', { e: new Date(end_date) });

    const co2Qb = this.repo
      .createQueryBuilder('m')
      .select('MIN(m.co2_ppm)', 'min')
      .addSelect('MAX(m.co2_ppm)', 'max')
      .addSelect('AVG(m.co2_ppm)', 'avg');
    if (start_date) co2Qb.andWhere('m.timestamp >= :s', { s: new Date(start_date) });
    if (end_date) co2Qb.andWhere('m.timestamp <= :e', { e: new Date(end_date) });

    const [tempRaw, co2Raw] = await Promise.all([tempQb.getRawOne(), co2Qb.getRawOne()]);

    return {
      count,
      temperature: {
        min: tempRaw.min != null ? parseFloat(tempRaw.min) : null,
        max: tempRaw.max != null ? parseFloat(tempRaw.max) : null,
        avg: tempRaw.avg != null ? parseFloat(parseFloat(tempRaw.avg).toFixed(2)) : null,
      },
      co2_ppm: {
        min: co2Raw.min != null ? parseInt(co2Raw.min) : null,
        max: co2Raw.max != null ? parseInt(co2Raw.max) : null,
        avg: co2Raw.avg != null ? parseFloat(parseFloat(co2Raw.avg).toFixed(2)) : null,
      },
    };
  }
}
