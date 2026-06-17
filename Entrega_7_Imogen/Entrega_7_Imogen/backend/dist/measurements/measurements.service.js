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
var __param = (this && this.__param) || function (paramIndex, decorator) {
    return function (target, key) { decorator(target, key, paramIndex); }
};
Object.defineProperty(exports, "__esModule", { value: true });
exports.MeasurementsService = void 0;
const common_1 = require("@nestjs/common");
const typeorm_1 = require("@nestjs/typeorm");
const typeorm_2 = require("typeorm");
const measurement_entity_1 = require("./measurement.entity");
let MeasurementsService = class MeasurementsService {
    constructor(repo) {
        this.repo = repo;
    }
    async create(dto) {
        const measurement = this.repo.create({ ...dto, timestamp: new Date() });
        const saved = await this.repo.save(measurement);
        console.log(`[db] Medição salva id=${saved.id}`);
        return saved;
    }
    async createFromSerial(parsed, rawLine) {
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
    async findAll(query) {
        const { limit = 100, offset = 0, start_date, end_date } = query;
        const where = {};
        if (start_date && end_date) {
            where.timestamp = (0, typeorm_2.Between)(new Date(start_date), new Date(end_date));
        }
        else if (start_date) {
            where.timestamp = (0, typeorm_2.MoreThanOrEqual)(new Date(start_date));
        }
        else if (end_date) {
            where.timestamp = (0, typeorm_2.LessThanOrEqual)(new Date(end_date));
        }
        return this.repo.find({
            where,
            order: { timestamp: 'DESC' },
            take: limit,
            skip: offset,
        });
    }
    async findLatest() {
        const [measurement] = await this.repo.find({
            order: { timestamp: 'DESC' },
            take: 1,
        });
        if (!measurement)
            throw new common_1.NotFoundException('Nenhuma medição encontrada');
        return measurement;
    }
    async getStats(query) {
        const { start_date, end_date } = query;
        const countQb = this.repo.createQueryBuilder('m');
        if (start_date)
            countQb.andWhere('m.timestamp >= :s', { s: new Date(start_date) });
        if (end_date)
            countQb.andWhere('m.timestamp <= :e', { e: new Date(end_date) });
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
        if (start_date)
            tempQb.andWhere('m.timestamp >= :s', { s: new Date(start_date) });
        if (end_date)
            tempQb.andWhere('m.timestamp <= :e', { e: new Date(end_date) });
        const co2Qb = this.repo
            .createQueryBuilder('m')
            .select('MIN(m.co2_ppm)', 'min')
            .addSelect('MAX(m.co2_ppm)', 'max')
            .addSelect('AVG(m.co2_ppm)', 'avg');
        if (start_date)
            co2Qb.andWhere('m.timestamp >= :s', { s: new Date(start_date) });
        if (end_date)
            co2Qb.andWhere('m.timestamp <= :e', { e: new Date(end_date) });
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
};
exports.MeasurementsService = MeasurementsService;
exports.MeasurementsService = MeasurementsService = __decorate([
    (0, common_1.Injectable)(),
    __param(0, (0, typeorm_1.InjectRepository)(measurement_entity_1.Measurement)),
    __metadata("design:paramtypes", [typeorm_2.Repository])
], MeasurementsService);
//# sourceMappingURL=measurements.service.js.map