"use strict";
var __decorate = (this && this.__decorate) || function (decorators, target, key, desc) {
    var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
    else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && Object.defineProperty(target, key, r), r;
};
Object.defineProperty(exports, "__esModule", { value: true });
exports.AppModule = void 0;
const common_1 = require("@nestjs/common");
const config_1 = require("@nestjs/config");
const typeorm_1 = require("@nestjs/typeorm");
const app_controller_1 = require("./app.controller");
const measurements_module_1 = require("./measurements/measurements.module");
const serial_module_1 = require("./serial/serial.module");
const measurement_entity_1 = require("./measurements/measurement.entity");
let AppModule = class AppModule {
};
exports.AppModule = AppModule;
exports.AppModule = AppModule = __decorate([
    (0, common_1.Module)({
        imports: [
            config_1.ConfigModule.forRoot({ isGlobal: true }),
            typeorm_1.TypeOrmModule.forRootAsync({
                inject: [config_1.ConfigService],
                useFactory: (config) => {
                    const url = config.get('DATABASE_URL', 'sqlite:///./data/measurements.db');
                    const dbPath = url.replace(/^sqlite:\/\/\/\.\//, '').replace(/^sqlite:\/\/\//, '');
                    console.log('[db] Banco conectado');
                    return {
                        type: 'better-sqlite3',
                        database: dbPath,
                        entities: [measurement_entity_1.Measurement],
                        synchronize: true,
                    };
                },
            }),
            measurements_module_1.MeasurementsModule,
            serial_module_1.SerialModule,
        ],
        controllers: [app_controller_1.AppController],
    })
], AppModule);
//# sourceMappingURL=app.module.js.map