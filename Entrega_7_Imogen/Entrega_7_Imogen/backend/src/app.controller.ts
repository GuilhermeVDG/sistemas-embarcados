import { Controller, Get } from '@nestjs/common';
import { ApiOperation, ApiTags } from '@nestjs/swagger';

@ApiTags('health')
@Controller()
export class AppController {
  @ApiOperation({ summary: 'Verifica se o backend está no ar' })
  @Get('health')
  health() {
    return { status: 'ok' };
  }
}
