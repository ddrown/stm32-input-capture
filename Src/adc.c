#include "stm32f0xx_hal.h"
#include <stdint.h>
#include "adc.h"
#include "i2c_slave.h"
#include "uart.h"

#define AVERAGE_SAMPLES 10

static uint16_t external_temps[AVERAGE_SAMPLES];
static uint16_t internal_temps[AVERAGE_SAMPLES];
static uint16_t internal_vrefs[AVERAGE_SAMPLES];
static int8_t adc_index = -1;

static uint16_t avg(uint16_t *values, uint8_t index) {
  uint32_t sum = 0;
  for(uint8_t i = 0; i <= index; i++) {
    sum += values[i];
  }
  return sum / (index+1);
}

void adc_poll() {
  if(adc_index < (AVERAGE_SAMPLES-1)) {
    adc_index++;
  } else {
    for(uint8_t i = 0; i < (AVERAGE_SAMPLES-1); i++) {
      external_temps[i] = external_temps[i+1];
      internal_temps[i] = internal_temps[i+1];
      internal_vrefs[i] = internal_vrefs[i+1];
    }
  }

  HAL_ADC_Start(&hadc);
  if(HAL_ADC_PollForConversion(&hadc, 10) == HAL_OK) {
    external_temps[adc_index] = HAL_ADC_GetValue(&hadc);
  }
  if(HAL_ADC_PollForConversion(&hadc, 10) == HAL_OK) {
    internal_temps[adc_index] = HAL_ADC_GetValue(&hadc);
  }
  if(HAL_ADC_PollForConversion(&hadc, 10) == HAL_OK) {
    internal_vrefs[adc_index] = HAL_ADC_GetValue(&hadc);
  }
  HAL_ADC_Stop(&hadc);

  if(!i2c_read_active()) {
    i2c_registers_page2.external_temp = avg(external_temps, adc_index);
    i2c_registers_page2.internal_temp = avg(internal_temps, adc_index);
    i2c_registers_page2.internal_vref = avg(internal_vrefs, adc_index);
    i2c_registers_page2.last_adc_ms = HAL_GetTick();
  }
}
