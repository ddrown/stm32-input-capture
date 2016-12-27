#include "stm32f0xx_hal.h"

#include <stdlib.h>
#include <math.h>

#include "timer.h"
#include "uart.h"
#include "i2c_slave.h"

// TIM3 input capture 
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) {
  static uint8_t counts = DEFAULT_SOURCE_HZ;
  counts--;
  if(counts == 0) {
    i2c_registers.tim3_at_irq = __HAL_TIM_GET_COUNTER(&htim3);
    i2c_registers.tim1_at_irq = __HAL_TIM_GET_COUNTER(&htim1);
    i2c_registers.milliseconds_irq = HAL_GetTick();
    i2c_registers.tim3_at_cap = HAL_TIM_ReadCapturedValue(&htim3, TIM_CHANNEL_1);

    if(i2c_registers.source_HZ > 0) {
      counts = i2c_registers.source_HZ;
    } else {
      counts = i2c_registers.source_HZ = DEFAULT_SOURCE_HZ;
    }
  }
}

void timer_start() {
  HAL_TIM_Base_Start(&htim1);
  HAL_TIM_Base_Start(&htim3);
  HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_1);
}

void print_timer_status() {
#if 0
  static uint32_t last_irq = 0;

  if(i2c_registers.milliseconds_irq != last_irq) {
    write_uart_s("cap 3=");
    write_uart_u(i2c_registers.tim3_at_cap);
    write_uart_s(" 1=");
    write_uart_u(i2c_registers.tim1_at_irq);
    write_uart_s(" (3@");
    write_uart_u(i2c_registers.tim3_at_irq);
    write_uart_s(") ms=");
    write_uart_u(i2c_registers.milliseconds_irq);
    write_uart_s(" now=");
    write_uart_u(HAL_GetTick());
    write_uart_s("\n");

    last_irq = i2c_registers.milliseconds_irq;
  }
#endif
}
