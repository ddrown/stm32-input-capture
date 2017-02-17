#include <stdint.h>

#include "i2c_registers.h"
#include "vref_calc.h"
#include "avg.h"

#define AVERAGE_SAMPLES 60
#define AVERAGE_MINUTE_SAMPLES 10

static float vrefs[AVERAGE_SAMPLES];
static float vrefs_minute[AVERAGE_MINUTE_SAMPLES];
static int8_t vref_index = -1;
static int8_t vref_minute_index = -1;
static float last_vref_value = 0.0;

float last_vref() {
  return last_vref_value;
}

static void add_vref_minute() {
  if(vref_minute_index < (AVERAGE_MINUTE_SAMPLES-1)) {
    vref_minute_index++;
  } else {
    for(int8_t i = 0; i < (AVERAGE_MINUTE_SAMPLES-1); i++) {
      vrefs_minute[i] = vrefs_minute[i+1];
    }
  }
  vrefs_minute[vref_minute_index] = avg_f(vrefs, vref_index+1);
  last_vref_value = avg_f(vrefs_minute, vref_minute_index+1);
}

void add_vref_data(const struct i2c_registers_type_page2 *i2c_registers_page2) {
  if(vref_index < (AVERAGE_SAMPLES-1)) {
    vref_index++;
  } else {
    add_vref_minute();
    vref_index = 0;
  }

  float expected = i2c_registers_page2->vrefint_cal/4096.0*3.3;
  float actual = i2c_registers_page2->internal_vref/4096.0*3.3;

  vrefs[vref_index] = expected/actual*3.3;

  if(vref_minute_index < 0)
    last_vref_value = avg_f(vrefs, vref_index+1);
}
