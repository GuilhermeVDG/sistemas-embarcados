import { Repository } from 'typeorm';
import { Measurement } from './measurement.entity';
import { CreateMeasurementDto } from './dto/create-measurement.dto';
import { QueryMeasurementsDto } from './dto/query-measurements.dto';
import { ParsedMeasurement } from '../serial/parser';
export declare class MeasurementsService {
    private readonly repo;
    constructor(repo: Repository<Measurement>);
    create(dto: CreateMeasurementDto): Promise<Measurement>;
    createFromSerial(parsed: ParsedMeasurement, rawLine: string): Promise<Measurement>;
    findAll(query: QueryMeasurementsDto): Promise<Measurement[]>;
    findLatest(): Promise<Measurement>;
    getStats(query: {
        start_date?: string;
        end_date?: string;
    }): Promise<{
        count: number;
        temperature: {
            min: number;
            max: number;
            avg: number;
        };
        co2_ppm: {
            min: number;
            max: number;
            avg: number;
        };
    }>;
}
