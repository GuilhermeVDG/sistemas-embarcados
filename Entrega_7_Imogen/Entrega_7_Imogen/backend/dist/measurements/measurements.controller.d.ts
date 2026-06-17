import { MeasurementsService } from './measurements.service';
import { CreateMeasurementDto } from './dto/create-measurement.dto';
import { QueryMeasurementsDto } from './dto/query-measurements.dto';
export declare class MeasurementsController {
    private readonly service;
    constructor(service: MeasurementsService);
    create(dto: CreateMeasurementDto): Promise<import("./measurement.entity").Measurement>;
    findLatest(): Promise<import("./measurement.entity").Measurement>;
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
    findAll(query: QueryMeasurementsDto): Promise<import("./measurement.entity").Measurement[]>;
}
