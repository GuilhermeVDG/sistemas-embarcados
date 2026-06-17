"use strict";
var __decorate = (this && this.__decorate) || function (decorators, target, key, desc) {
    var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
    else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && Object.defineProperty(target, key, r), r;
};
var __metadata = (this && this.__metadata) || function (k, v) {
    if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
};
Object.defineProperty(exports, "__esModule", { value: true });
exports.Measurement = void 0;
const typeorm_1 = require("typeorm");
let Measurement = class Measurement {
};
exports.Measurement = Measurement;
__decorate([
    (0, typeorm_1.PrimaryGeneratedColumn)(),
    __metadata("design:type", Number)
], Measurement.prototype, "id", void 0);
__decorate([
    (0, typeorm_1.Column)({ type: 'datetime' }),
    __metadata("design:type", Date)
], Measurement.prototype, "timestamp", void 0);
__decorate([
    (0, typeorm_1.Column)({ type: 'float', nullable: true }),
    __metadata("design:type", Number)
], Measurement.prototype, "temperature_c", void 0);
__decorate([
    (0, typeorm_1.Column)({ type: 'integer' }),
    __metadata("design:type", Number)
], Measurement.prototype, "co2_ppm", void 0);
__decorate([
    (0, typeorm_1.Column)({ type: 'integer' }),
    __metadata("design:type", Number)
], Measurement.prototype, "adc_raw", void 0);
__decorate([
    (0, typeorm_1.Column)({ type: 'integer', nullable: true }),
    __metadata("design:type", Number)
], Measurement.prototype, "vpin_mv", void 0);
__decorate([
    (0, typeorm_1.Column)({ type: 'integer', nullable: true }),
    __metadata("design:type", Number)
], Measurement.prototype, "vao_mv", void 0);
__decorate([
    (0, typeorm_1.Column)({ type: 'text' }),
    __metadata("design:type", String)
], Measurement.prototype, "sensor_status", void 0);
__decorate([
    (0, typeorm_1.Column)({ type: 'text', default: 'uart' }),
    __metadata("design:type", String)
], Measurement.prototype, "source", void 0);
__decorate([
    (0, typeorm_1.Column)({ type: 'text', nullable: true }),
    __metadata("design:type", String)
], Measurement.prototype, "raw_line", void 0);
__decorate([
    (0, typeorm_1.CreateDateColumn)(),
    __metadata("design:type", Date)
], Measurement.prototype, "created_at", void 0);
exports.Measurement = Measurement = __decorate([
    (0, typeorm_1.Entity)('measurements')
], Measurement);
//# sourceMappingURL=measurement.entity.js.map