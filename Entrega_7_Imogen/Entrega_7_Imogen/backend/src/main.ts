import { NestFactory } from '@nestjs/core';
import { ValidationPipe } from '@nestjs/common';
import { DocumentBuilder, SwaggerModule } from '@nestjs/swagger';
import { AppModule } from './app.module';

async function bootstrap() {
  const app = await NestFactory.create(AppModule);
  app.useGlobalPipes(new ValidationPipe({ transform: true, whitelist: true }));

  const config = new DocumentBuilder()
    .setTitle('Monitoramento UART')
    .setDescription(
      'API para consulta do histórico de medições de temperatura e CO2 recebidas via UART da placa STM32 NUCLEO-L476RG.\n\n' +
      '> **Atenção:** o campo `co2_ppm` é uma estimativa didática obtida a partir do sensor analógico MQ-135 e não representa uma medição calibrada.',
    )
    .setVersion('1.0')
    .build();

  const document = SwaggerModule.createDocument(app, config);
  SwaggerModule.setup('docs', app, document);

  await app.listen(3000);
  console.log('[app] Backend iniciado na porta 3000');
  console.log('[app] Swagger disponível em http://localhost:3000/docs');
}

bootstrap();
