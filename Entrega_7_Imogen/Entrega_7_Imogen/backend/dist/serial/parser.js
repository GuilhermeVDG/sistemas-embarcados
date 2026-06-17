"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.parseMeasurementLine = parseMeasurementLine;
function parseMeasurementLine(line) {
    const trimmed = line.trim();
    if (!trimmed)
        return null;
    let data;
    try {
        data = JSON.parse(trimmed);
    }
    catch {
        console.log('[parser] Linha ignorada');
        return null;
    }
    const { temperature_c, co2_ppm, adc_raw, vpin_mv, vao_mv, sensor_status } = data;
    const requiredFields = [co2_ppm, adc_raw, vpin_mv, vao_mv, sensor_status];
    if (requiredFields.some((f) => f === undefined)) {
        console.log('[parser] Linha ignorada');
        return null;
    }
    if (temperature_c !== null && temperature_c !== undefined && typeof temperature_c !== 'number') {
        console.log('[parser] Linha ignorada');
        return null;
    }
    if (!Number.isInteger(co2_ppm) || co2_ppm < 0) {
        console.log('[parser] Linha ignorada');
        return null;
    }
    if (!Number.isInteger(adc_raw) || adc_raw < 0 || adc_raw > 4095) {
        console.log('[parser] Linha ignorada');
        return null;
    }
    if (!Number.isInteger(vpin_mv) || vpin_mv < 0) {
        console.log('[parser] Linha ignorada');
        return null;
    }
    if (!Number.isInteger(vao_mv) || vao_mv < 0) {
        console.log('[parser] Linha ignorada');
        return null;
    }
    if (typeof sensor_status !== 'string') {
        console.log('[parser] Linha ignorada');
        return null;
    }
    return {
        temperature_c: temperature_c ?? null,
        co2_ppm,
        adc_raw,
        vpin_mv,
        vao_mv,
        sensor_status,
    };
}
//# sourceMappingURL=parser.js.map