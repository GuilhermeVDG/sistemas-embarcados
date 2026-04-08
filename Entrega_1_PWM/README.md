# 📘 PWM por Software (STM32)

## 🔷 GPIO e PWM por Software

### 🎯 Objetivo
Implementar um PWM por software utilizando GPIO para controlar o LED da placa **NUCLEO-L476RG**.

---

## 🧠 Conceito

PWM (*Pulse Width Modulation*) controla o tempo ligado/desligado de um sinal digital, permitindo:

- Ajustar o brilho de um LED  
- Simular uma saída analógica  

---

## ⚙️ Parâmetros

- **Frequência (Frequency)**  
  Número de ciclos por segundo  

- **Duty Cycle (%)**  
  Percentual do tempo em que o sinal permanece ligado  

---

## 🧮 Fórmulas

```c
period_ms = 1000 / frequency;
on_time   = (period_ms * duty) / 100;
off_time  = period_ms - on_time;
```

---

## 🔌 Hardware

| Item  | Valor            |
|------|------------------|
| Placa | NUCLEO-L476RG   |
| LED   | LD2             |
| Pino  | PA5             |

---

## ⚙️ Configuração

```c
PA5 → GPIO_Output
```

---

## 💻 Implementação

### 🔧 Defines

```c
#define PWM_FREQUENCY_HZ    2
#define DUTY_CYCLE_PERCENT  25
```

---

### ⚡ Função PWM

```c
void software_pwm(uint16_t frequency, uint8_t duty_cycle)
{
  uint32_t period = 1000 / frequency;
  uint32_t on = (period * duty_cycle) / 100;
  uint32_t off = period - on;

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
  HAL_Delay(on);

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
  HAL_Delay(off);
}
```

---

### 🔁 Loop Principal

```c
while (1)
{
  software_pwm(PWM_FREQUENCY_HZ, DUTY_CYCLE_PERCENT);
}
```

---

## 🧪 Casos de Teste

### 🔥 Caso 1 — 25%

| Estado | Tempo   |
|--------|--------|
| ON     | 125 ms |
| OFF    | 375 ms |

👉 LED pisca rápido e fica mais apagado  

---

### 🔥 Caso 2 — 80%

```c
#define DUTY_CYCLE_PERCENT 80
```

👉 LED permanece quase sempre ligado  

---

### 🔥 Caso 3 — 10%

```c
#define DUTY_CYCLE_PERCENT 10
```

👉 LED apresenta flashes rápidos  

---

### ⚡ Caso 4 — Alta Frequência

```c
#define PWM_FREQUENCY_HZ 20
```

👉 LED aparenta brilho contínuo  

---

## ⚠️ Limitações

- Uso de `HAL_Delay()` → bloqueia a CPU  
- Baixa precisão de temporização  
- Não recomendado para aplicações críticas  

---

## 🚀 Melhorias Futuras

- Utilizar PWM por hardware (Timers - TIM)  
- Controle de múltiplos LEDs  
- Integração com RTOS  
- Ajuste dinâmico via botão ou interface  

---

## 🧠 Conclusão

Este projeto demonstra de forma clara:

- Controle de sinais digitais  
- Impacto do duty cycle no comportamento do LED  
- Funcionamento prático do PWM  
