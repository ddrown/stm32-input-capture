#include "stm32f0xx_hal.h"

#include <stdlib.h>
#include <math.h>

#include "timer.h"
#include "uart.h"
#include "i2c_slave.h"

// TIM3 input capture 
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) {
  static uint8_t counts_ch1 = DEFAULT_SOURCE_HZ;
  uint16_t tim3_at_irq, tim1_at_irq;
  uint32_t milliseconds_irq;

  // get the timer values first, to lower the chance of tim3 wrapping
  tim3_at_irq = __HAL_TIM_GET_COUNTER(&htim3);
  tim1_at_irq = __HAL_TIM_GET_COUNTER(&htim1);
  milliseconds_irq = HAL_GetTick();

  // figure out where the input capture came from
  if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) {
    counts_ch1--;
    if(counts_ch1 == 0) {
      i2c_registers.tim3_at_irq[0] = tim3_at_irq;
      i2c_registers.tim1_at_irq[0] = tim1_at_irq;
      i2c_registers.milliseconds_irq_ch1 = milliseconds_irq;
      i2c_registers.tim3_at_cap[0] = HAL_TIM_ReadCapturedValue(&htim3, TIM_CHANNEL_1);

      if(i2c_registers.source_HZ_ch1 > 0) {
	counts_ch1 = i2c_registers.source_HZ_ch1;
      } else {
	counts_ch1 = i2c_registers.source_HZ_ch1 = DEFAULT_SOURCE_HZ;
      }
    }
  } else if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2) {
    i2c_registers.tim3_at_irq[1] = tim3_at_irq;
    i2c_registers.tim1_at_irq[1] = tim1_at_irq;
    i2c_registers.tim3_at_cap[1] = HAL_TIM_ReadCapturedValue(&htim3, TIM_CHANNEL_2);
  } else if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_4) {
    i2c_registers.tim3_at_irq[2] = tim3_at_irq;
    i2c_registers.tim1_at_irq[2] = tim1_at_irq;
    i2c_registers.tim3_at_cap[2] = HAL_TIM_ReadCapturedValue(&htim3, TIM_CHANNEL_4);
  }
}

void timer_start() {
  HAL_TIM_Base_Start(&htim1);
  HAL_TIM_Base_Start(&htim3);
  HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_1);
  HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_2);
  HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_4);
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
