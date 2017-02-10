#include <stdint.h>
#include <stdio.h>

#include "i2c_registers.h"
#include "adc_calc.h"

#define AVERAGE_SAMPLES 60

static float vrefs[AVERAGE_SAMPLES];
static float temps[AVERAGE_SAMPLES];
static float ext_temps[AVERAGE_SAMPLES];
static int8_t adc_index = -1;
static int32_t delay;

static uint32_t last_adc = 0;

static float avg(float *values, uint8_t index) {
  float sum = 0;
  for(uint8_t i = 0; i <= index; i++) {
    sum += values[i];
  }
  return sum / (index+1.0);
}

float last_vref() {
  return avg(vrefs, adc_index);
}

float last_temp() {
  return avg(temps, adc_index);
}

float last_ext_temp() {
  return avg(ext_temps, adc_index);
}

void adc_header() {
  printf("ext-temp int-temp vref adc.ms i2c.s");
}

void adc_print() {
  printf("%.4f ", last_ext_temp()*9/5.0+32.0);
  printf("%.4f ", last_temp()*9/5.0+32.0);
  printf("%.4f ", last_vref());
  printf("%d ", delay);
  printf("%.6f ", last_i2c_time());
}

void add_adc_data(const struct i2c_registers_type *i2c_registers, const struct i2c_registers_type_page2 *i2c_registers_page2) {
  if(i2c_registers_page2->last_adc_ms == last_adc) {
    return;
  }

  if(adc_index < (AVERAGE_SAMPLES-1)) {
    adc_index++;
  } else {
    for(uint8_t i = 0; i < (AVERAGE_SAMPLES-1); i++) {
      vrefs[i] = vrefs[i+1];
      temps[i] = temps[i+1];
      ext_temps[i] = ext_temps[i+1];
    }
  }

  last_adc = i2c_registers_page2->last_adc_ms;

  float expected = i2c_registers_page2->vrefint_cal/4096.0*3.3;
  float actual = i2c_registers_page2->internal_vref/4096.0*3.3;

  vrefs[adc_index] = expected/actual*3.3;

  float vref = last_vref();

  float temp_voltage = i2c_registers_page2->internal_temp/4096.0*vref;
  float v_30C = i2c_registers_page2->ts_cal1/4096.0*3.3;
  float v_110C = i2c_registers_page2->ts_cal2/4096.0*3.3;

  temps[adc_index] = (temp_voltage - v_30C) * (110 - 30) / (v_110C-v_30C) + 30.0;

  float ext_temp_voltage = i2c_registers_page2->external_temp/4096.0*vref;
  ext_temps[adc_index] = (ext_temp_voltage - 0.750) * 100.0 + 25.0; 

  delay = i2c_registers->milliseconds_now - i2c_registers_page2->last_adc_ms;
}
