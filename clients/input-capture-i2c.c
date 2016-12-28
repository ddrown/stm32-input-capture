#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "i2c.h"
#include "timespec.h"

#define I2C_ADDR 0x4
#define EXPECTED_FREQ 48000000
#define AVERAGING_CYCLES 129
#define PPM_INVALID -1000000.0
#define INPUT_CHANNELS 3
// TODO: allow the average ppm to be detected or configured
#define AVERAGE_PPM 20.465
// TODO: keep an estimate of this offset?
#define AVERAGE_PPB_TCXO 3271
#define AIM_AFTER_MS 5

struct i2c_registers_type {
  uint32_t milliseconds_now;
  uint32_t milliseconds_irq_ch1;
  uint16_t tim3_at_irq[INPUT_CHANNELS];
  uint16_t tim1_at_irq[INPUT_CHANNELS];
  uint16_t tim3_at_cap[INPUT_CHANNELS];
  uint16_t source_HZ_ch1;
};

void print_ppm(float ppm) {
  if(ppm < 500 && ppm > -500) {
    printf("%1.3f ", ppm);
  } else {
    printf("- ");
  }
}

void write_tcxo_ppm(float ppm) {
  FILE *tcxo;

  ppm = ppm - AVERAGE_PPM;

  //ppm = ppm * 0.5; // reduce tempcomp effect by 50%

  if(ppm > 10 || ppm < -10) {
    printf("- ");
    return;
  }

  tcxo = fopen("/run/.tcxo","w");
  if(tcxo == NULL) {
    perror("fopen /run/.tcxo");
    exit(1);
  }
  fprintf(tcxo, "%1.3f\n", ppm);
  fclose(tcxo);
  rename("/run/.tcxo","/run/tcxo");
  printf("%1.3f ", ppm);
}

void add_offset_cycles(double added_offset_ns, struct timespec *cycles, uint16_t *first_cycle, uint16_t *last_cycle) {
  struct timespec *previous_cycle = NULL;
  uint16_t this_cycle_i;

  this_cycle_i = (*last_cycle + 1) % AVERAGING_CYCLES;
  if(*first_cycle != *last_cycle) {
    previous_cycle = &cycles[*last_cycle];
    if(*first_cycle == this_cycle_i) { // we've wrapped around, move the first pointer forward
      *first_cycle = (*first_cycle + 1) % AVERAGING_CYCLES;
    }
  }

  cycles[this_cycle_i].tv_sec = 0;
  cycles[this_cycle_i].tv_nsec = added_offset_ns;
  if(previous_cycle != NULL) {
    add_timespecs(&cycles[this_cycle_i], previous_cycle);
  }

  *last_cycle = this_cycle_i;
}

float calc_ppm(struct timespec *end, struct timespec *start, uint16_t seconds) {
  double diff_s;
  struct timespec diff;
  diff.tv_sec = end->tv_sec;
  diff.tv_nsec = end->tv_nsec;

  sub_timespecs(&diff, start);
  diff_s = diff.tv_sec + diff.tv_nsec / 1000000000.0;

  float ppm = diff_s * 1000000.0 / seconds;
  return ppm;
}

// TODO: dealing with 2s and longer intervals
uint8_t cycles_wrap(uint32_t *this_cycles, uint32_t previous_cycles, int32_t *diff, const struct i2c_registers_type *i2c_registers, uint8_t counter) {
  uint8_t wrap = 0;
  *diff = *this_cycles - previous_cycles - EXPECTED_FREQ;

  if(i2c_registers->tim3_at_cap[counter] > i2c_registers->tim3_at_irq[counter]) {
    if(*diff > 41535) { // allow for +/-500ppm
      wrap = 1;
      *this_cycles -= 65536;
      *diff -= 65536;
    }
  } else if(i2c_registers->tim3_at_cap[counter] > 65300) { // check for wrap if it's close
    if(*diff > 41535) { // allow for +/-500ppm
      wrap = 2;
      *this_cycles -= 65536;
      *diff -= 65536;
    }
  }

  return wrap;
}

uint16_t wrap_add(int16_t a, int16_t b, uint16_t modulus) {
  a = a + b;
  if(a < 0) {
    a += modulus;
  }
  return a;
}

uint16_t wrap_sub(int16_t a, int16_t b, uint16_t modulus) {
  return wrap_add(a, -1 * b, modulus);
}

float show_ppm(int16_t number_points, uint16_t last_cycle_index, uint16_t seconds, struct timespec *cycles) {
  float ppm = PPM_INVALID; // default: invalid PPM

  if(number_points >= seconds) {
    uint16_t start_index = wrap_sub(last_cycle_index, seconds, AVERAGING_CYCLES);
    ppm = calc_ppm(&cycles[last_cycle_index], &cycles[start_index], seconds);
    print_ppm(ppm);
  } else {
    printf("- ");
  }

  return ppm;
}

uint32_t calculate_sleep_ms(uint32_t milliseconds_now, uint32_t milliseconds_irq) {
  uint32_t sleep_ms = 1000 + AIM_AFTER_MS - (milliseconds_now - milliseconds_irq);
  if(sleep_ms > 1000+AIM_AFTER_MS) {
    sleep_ms = 1000+AIM_AFTER_MS;
  } else if(sleep_ms < 1) {
    sleep_ms = 1;
  }
  return sleep_ms;
}

// modifies this_cycles, wrap, and added_offset_ns
int add_cycles(uint32_t *this_cycles, uint8_t *wrap, double *added_offset_ns, uint8_t has_history, const struct i2c_registers_type *i2c_registers) {
  static uint32_t previous_cycles[INPUT_CHANNELS] = {0,0,0};
  int retval = 0;

  // we can check for wraps if we have history
  if(has_history || (previous_cycles[0] > 0)) {
    int32_t diff[INPUT_CHANNELS];
    for(uint8_t i = 0; i < INPUT_CHANNELS; i++) {
      wrap[i] = cycles_wrap(&this_cycles[i], previous_cycles[i], &diff[i], i2c_registers, i);
      added_offset_ns[i] = diff[i] * 1000000000.0 / EXPECTED_FREQ;
    }
    retval = 1;
  }

  for(uint8_t i = 0; i < INPUT_CHANNELS; i++) {
    previous_cycles[i] = this_cycles[i];
  }

  return retval;
}

// modifies this_cycles
void combine_tim1_tim3(uint32_t *this_cycles, const struct i2c_registers_type *i2c_registers) {
  for(uint8_t i = 0; i < INPUT_CHANNELS; i++) {
    this_cycles[i] = ((uint32_t)i2c_registers->tim1_at_irq[i]) << 16;
    this_cycles[i] += i2c_registers->tim3_at_cap[i];
  }
}

// modifies sleep_ms
// at 500ppm, this estimates the next update within 0.5ms
void adjust_sleep_ms(uint32_t *sleep_ms, const uint32_t *this_cycles) {
  uint32_t aiming_cycles = EXPECTED_FREQ / 1000 * AIM_AFTER_MS;
  uint32_t estimated_cycles_after_sleep = this_cycles[0] + EXPECTED_FREQ + aiming_cycles;

  for(uint8_t i = 0; i < INPUT_CHANNELS; i++) {
    uint32_t estimated_cycles = this_cycles[i] + EXPECTED_FREQ;
    int32_t diff_cycles = estimated_cycles-estimated_cycles_after_sleep;

    if(diff_cycles > EXPECTED_FREQ - aiming_cycles) {
      diff_cycles -= EXPECTED_FREQ;
    }
    if(abs(diff_cycles) < aiming_cycles) {
      // if it's expected within the +/-AIM_AFTER_MS window, move the window forward
      estimated_cycles_after_sleep += aiming_cycles;
      *sleep_ms += AIM_AFTER_MS;
    }
  }
}

int main() {
  int fd;
  struct timespec cycles[AVERAGING_CYCLES];
  uint16_t first_cycle_index = 0, last_cycle_index = 0;
  uint32_t last_timestamp = 0;
  struct i2c_registers_type i2c_registers;

  memset(cycles, '\0', sizeof(cycles));
 
  fd = open_i2c(I2C_ADDR); 

  printf("ts delay wrap sleepms cycles1 cycles2 cycles3 #pts ch1 ch2 ch3 t.offset 16s_ppm 64s_ppm 128s_ppm tempcomp\n");
  while(1) {
    double added_offset_ns[INPUT_CHANNELS];
    uint32_t sleep_ms, this_cycles[INPUT_CHANNELS];
    uint8_t wrap[INPUT_CHANNELS] = {0,0,0};
    int16_t number_points;

    read_i2c(fd, &i2c_registers, sizeof(i2c_registers));

    // was there no new data?
    if(i2c_registers.milliseconds_irq_ch1 == last_timestamp) {
      printf("no new data\n");
      fflush(stdout);
      first_cycle_index = last_cycle_index = 0; // reset because we missed a cycle
      usleep(995000);
      continue;
    }
    last_timestamp = i2c_registers.milliseconds_irq_ch1;

    // aim for 5ms after the event
    sleep_ms = calculate_sleep_ms(i2c_registers.milliseconds_now, i2c_registers.milliseconds_irq_ch1);

    combine_tim1_tim3(this_cycles, &i2c_registers);

    if(!add_cycles(this_cycles, wrap, added_offset_ns, (last_cycle_index != first_cycle_index), &i2c_registers)) {
      printf("first cycle, sleeping %d ms\n", sleep_ms);
      fflush(stdout);
      usleep(sleep_ms * 1000);
      continue;
    }

    // estimate position of ch2/ch3 and modify sleep_ms if they're within 2ms of polling
    adjust_sleep_ms(&sleep_ms, this_cycles);

    // consider channel 2 "absolute" and the other two as relative to it as long as chan2 is within 10ppm
    if(added_offset_ns[1] > -10000 && added_offset_ns[1] < 10000) {
      added_offset_ns[0] -= added_offset_ns[1];
      added_offset_ns[2] -= added_offset_ns[1];
    } else { // fall back to the configured PPB offset
      added_offset_ns[0] -= AVERAGE_PPB_TCXO;
      added_offset_ns[2] -= AVERAGE_PPB_TCXO;
    }
    add_offset_cycles(added_offset_ns[0], cycles, &first_cycle_index, &last_cycle_index);

    number_points = wrap_sub(last_cycle_index, first_cycle_index, AVERAGING_CYCLES);

    printf("%lu %u %u %u %u %u %u %u %0.0f %0.0f %0.0f ",
       time(NULL),
       i2c_registers.milliseconds_now - i2c_registers.milliseconds_irq_ch1,
       (wrap[0] | wrap[1] << 2 | wrap[3] << 4), 
       sleep_ms,
       this_cycles[0], this_cycles[1], this_cycles[2],
       number_points, 
       added_offset_ns[0], added_offset_ns[1], added_offset_ns[2]
       );
    print_timespec(&cycles[last_cycle_index]);
    printf(" ");

    show_ppm(number_points, last_cycle_index, 16, cycles);
    show_ppm(number_points, last_cycle_index, 64, cycles);
    float ppm = show_ppm(number_points, last_cycle_index, AVERAGING_CYCLES-1, cycles);
    write_tcxo_ppm(ppm);

    printf("\n");
    fflush(stdout);

    usleep(sleep_ms * 1000);
  }
}
