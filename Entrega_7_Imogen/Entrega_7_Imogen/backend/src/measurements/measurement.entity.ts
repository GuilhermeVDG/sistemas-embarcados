import { Entity, PrimaryGeneratedColumn, Column, CreateDateColumn } from 'typeorm';

@Entity('measurements')
export class Measurement {
  @PrimaryGeneratedColumn()
  id: number;

  @Column({ type: 'datetime' })
  timestamp: Date;

  @Column({ type: 'float', nullable: true })
  temperature_c: number | null;

  @Column({ type: 'integer' })
  co2_ppm: number;

  @Column({ type: 'integer' })
  adc_raw: number;

  @Column({ type: 'integer', nullable: true })
  vpin_mv: number | null;

  @Column({ type: 'integer', nullable: true })
  vao_mv: number | null;

  @Column({ type: 'text' })
  sensor_status: string;

  @Column({ type: 'text', default: 'uart' })
  source: string;

  @Column({ type: 'text', nullable: true })
  raw_line: string | null;

  @CreateDateColumn()
  created_at: Date;
}
