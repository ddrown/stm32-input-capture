#include <stdint.h>
#include <stdio.h>

#include "i2c_registers.h"
#include "adc_calc.h"
#include "vref_calc.h"
#include "avg.h"

#define AVERAGE_SAMPLES 60

static float temps[AVERAGE_SAMPLES];
static float ext_temps[AVERAGE_SAMPLES];
static int8_t adc_index = -1;

static uint32_t last_adc = 0;

float last_temp() {
  return avg_f(temps, adc_index+1);
}

float last_ext_temp() {
  return avg_f(ext_temps, adc_index+1);
}

void adc_header() {
  printf("ext-temp int-temp vref");
}

void adc_print() {
  printf("%.4f ", last_ext_temp()*9/5.0+32.0);
  printf("%.4f ", last_temp()*9/5.0+32.0);
  printf("%.5f ", last_vref());
}

void add_adc_data(const struct i2c_registers_type *i2c_registers, const struct i2c_registers_type_page2 *i2c_registers_page2) {
  if(i2c_registers_page2->last_adc_ms == last_adc) {
    return;
  }

  if(adc_index < (AVERAGE_SAMPLES-1)) {
    adc_index++;
  } else {
    for(uint8_t i = 0; i < (AVERAGE_SAMPLES-1); i++) {
      temps[i] = temps[i+1];
      ext_temps[i] = ext_temps[i+1];
    }
  }

  last_adc = i2c_registers_page2->last_adc_ms;

  add_vref_data(i2c_registers_page2);

  float vref = last_vref();

  float temp_voltage = i2c_registers_page2->internal_temp/4096.0*vref;
  float v_30C = i2c_registers_page2->ts_cal1/4096.0*3.3;
  float v_110C = i2c_registers_page2->ts_cal2/4096.0*3.3;

  temps[adc_index] = (temp_voltage - v_30C) * (110 - 30) / (v_110C-v_30C) + 30.0;

  float ext_temp_voltage = i2c_registers_page2->external_temp/4096.0*vref;
  ext_temps[adc_index] = (ext_temp_voltage - 0.750) * 100.0 + 25.0; 

  //delay = i2c_registers->milliseconds_now - i2c_registers_page2->last_adc_ms;
}
