#include <stdint.h>
#include "avg.h"

float avg_f(float *values, uint8_t length) {
  float sum = 0;
  for(uint8_t i = 0; i < length; i++) {
    sum += values[i];
  }
  return sum / (float)length;
}

