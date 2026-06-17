export interface ParsedMeasurement {
    temperature_c: number | null;
    co2_ppm: number;
    adc_raw: number;
    vpin_mv: number;
    vao_mv: number;
    sensor_status: string;
}
export declare function parseMeasurementLine(line: string): ParsedMeasurement | null;
