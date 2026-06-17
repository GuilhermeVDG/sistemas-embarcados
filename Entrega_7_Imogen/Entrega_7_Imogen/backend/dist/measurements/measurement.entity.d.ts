export declare class Measurement {
    id: number;
    timestamp: Date;
    temperature_c: number | null;
    co2_ppm: number;
    adc_raw: number;
    vpin_mv: number | null;
    vao_mv: number | null;
    sensor_status: string;
    source: string;
    raw_line: string | null;
    created_at: Date;
}
