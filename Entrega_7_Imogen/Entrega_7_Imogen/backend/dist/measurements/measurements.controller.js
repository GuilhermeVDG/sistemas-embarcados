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
exports.MeasurementsController = void 0;
const common_1 = require("@nestjs/common");
const swagger_1 = require("@nestjs/swagger");
const measurements_service_1 = require("./measurements.service");
const create_measurement_dto_1 = require("./dto/create-measurement.dto");
const query_measurements_dto_1 = require("./dto/query-measurements.dto");
let MeasurementsController = class MeasurementsController {
    constructor(service) {
        this.service = service;
    }
    create(dto) {
        return this.service.create(dto);
    }
    findLatest() {
        return this.service.findLatest();
    }
    getStats(query) {
        return this.service.getStats(query);
    }
    findAll(query) {
        return this.service.findAll(query);
    }
};
exports.MeasurementsController = MeasurementsController;
__decorate([
    (0, swagger_1.ApiOperation)({ summary: 'Criar medição manualmente' }),
    (0, common_1.Post)(),
    __param(0, (0, common_1.Body)()),
    __metadata("design:type", Function),
    __metadata("design:paramtypes", [create_measurement_dto_1.CreateMeasurementDto]),
    __metadata("design:returntype", void 0)
], MeasurementsController.prototype, "create", null);
__decorate([
    (0, swagger_1.ApiOperation)({ summary: 'Retorna a medição mais recente recebida pela UART' }),
    (0, common_1.Get)('latest'),
    __metadata("design:type", Function),
    __metadata("design:paramtypes", []),
    __metadata("design:returntype", void 0)
], MeasurementsController.prototype, "findLatest", null);
__decorate([
    (0, swagger_1.ApiOperation)({ summary: 'Estatísticas de temperatura e CO2 (min, max, média)' }),
    (0, common_1.Get)('stats'),
    __param(0, (0, common_1.Query)()),
    __metadata("design:type", Function),
    __metadata("design:paramtypes", [Object]),
    __metadata("design:returntype", void 0)
], MeasurementsController.prototype, "getStats", null);
__decorate([
    (0, swagger_1.ApiOperation)({ summary: 'Lista o histórico de medições (mais recentes primeiro)' }),
    (0, common_1.Get)(),
    __param(0, (0, common_1.Query)()),
    __metadata("design:type", Function),
    __metadata("design:paramtypes", [query_measurements_dto_1.QueryMeasurementsDto]),
    __metadata("design:returntype", void 0)
], MeasurementsController.prototype, "findAll", null);
exports.MeasurementsController = MeasurementsController = __decorate([
    (0, swagger_1.ApiTags)('measurements'),
    (0, common_1.Controller)('measurements'),
    __metadata("design:paramtypes", [measurements_service_1.MeasurementsService])
], MeasurementsController);
//# sourceMappingURL=measurements.controller.js.map