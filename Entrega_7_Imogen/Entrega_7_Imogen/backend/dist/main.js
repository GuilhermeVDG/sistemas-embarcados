"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const core_1 = require("@nestjs/core");
const common_1 = require("@nestjs/common");
const swagger_1 = require("@nestjs/swagger");
const app_module_1 = require("./app.module");
async function bootstrap() {
    const app = await core_1.NestFactory.create(app_module_1.AppModule);
    app.useGlobalPipes(new common_1.ValidationPipe({ transform: true, whitelist: true }));
    const config = new swagger_1.DocumentBuilder()
        .setTitle('Monitoramento UART')
        .setDescription('API para consulta do histórico de medições de temperatura e CO2 recebidas via UART da placa STM32 NUCLEO-L476RG.\n\n' +
        '> **Atenção:** o campo `co2_ppm` é uma estimativa didática obtida a partir do sensor analógico MQ-135 e não representa uma medição calibrada.')
        .setVersion('1.0')
        .build();
    const document = swagger_1.SwaggerModule.createDocument(app, config);
    swagger_1.SwaggerModule.setup('docs', app, document);
    await app.listen(3000);
    console.log('[app] Backend iniciado na porta 3000');
    console.log('[app] Swagger disponível em http://localhost:3000/docs');
}
bootstrap();
//# sourceMappingURL=main.js.map