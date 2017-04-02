#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <wiringPi.h>

#include "i2c.h"
#include "i2c_registers.h"
#include "timespec.h"

#define TIMESTAMP_PIN 13

#define EXPECTED_FREQ 48000000
// EXPECTED_FREQ*0.0005
#define EXPECTED_FREQ_500ppm_CYCLES 24000

// 89s per 32 bit wrap
#define NS_PER_WRAP 89478485333.333

#define STATE_CH2_WRAP    0b001
#define STATE_CH4_WRAP    0b010
#define STATE_MAX_LATENCY 0b100

// 8us
#define MAX_GPIO_LATENCY 0.000008

static struct timespec gpio_start,gpio_end,gpio_down;

struct shmTime {
        int    mode; /* 0 - if valid is set:
                      *       use values,
                      *       clear valid
                      * 1 - if valid is set:
                      *       if count before and after read of values is equal,
                      *         use values
                      *       clear valid
                      */
        volatile int    count;
        time_t          clockTimeStampSec;
        int             clockTimeStampUSec;
        time_t          receiveTimeStampSec;
        int             receiveTimeStampUSec;
        int             leap;
        int             precision;
        int             nsamples;
        volatile int    valid;
        unsigned        clockTimeStampNSec;     /* Unsigned ns timestamps */
        unsigned        receiveTimeStampNSec;   /* Unsigned ns timestamps */
        int             dummy[8];
};

#define NTPD_SHM0 0x4e545030
#define NTPD_SHM1 0x4e545031
#define NTPD_SHM2 0x4e545032
#define NTPD_SHM3 0x4e545033

static void send_offset(float offset, const struct timespec *system_time) {
  struct shmTime *send;
  static void *ntpd_shm = NULL;

  if(ntpd_shm == NULL) {
    int shmid = shmget(NTPD_SHM1, 80, 0600);
    if(shmid < 0) {
      perror("shmget");
      exit(1);
    }

    ntpd_shm = shmat(shmid, NULL, 0);
    if(ntpd_shm == (void *)-1 || ntpd_shm == NULL) {
      perror("shmat");
      exit(1);
    }
  }

  send = ntpd_shm;

  send->mode = 0;
  send->count = 0;
  send->clockTimeStampSec = system_time->tv_sec;
  send->clockTimeStampUSec = 0;
  send->clockTimeStampNSec = 0;

  send->receiveTimeStampSec = system_time->tv_sec;
  if(offset < 0) {
    send->receiveTimeStampSec--;
    send->receiveTimeStampUSec = ((1.0 - offset) * 1000000.0);
    send->receiveTimeStampNSec = ((1.0 - offset) * 1000000000.0);
  } else {
    send->receiveTimeStampUSec = (offset * 1000000.0);
    send->receiveTimeStampNSec = (offset * 1000000000.0);
  }
  send->leap = 0; // unknown leap state
  send->precision = -26;
  send->nsamples = 1;
  send->valid = 1;

  /*
  printf("send: %ld.%06d,%03u -> %ld.%06d,%03u\n", send.receiveTimeStampSec, send.receiveTimeStampUSec, send.receiveTimeStampNSec,
          send.clockTimeStampSec, send.clockTimeStampUSec, send.clockTimeStampNSec);
   */
}

static int calculate_counter(uint16_t lsb, uint16_t msb, uint32_t *last_counter, uint32_t *counter, uint8_t last_events, float expected_s) {
  uint32_t expected_cycles;

  *counter = lsb | ((uint32_t)msb)<<16;
  if(*counter == *last_counter) {
    return -1;
  }

  expected_cycles = EXPECTED_FREQ * expected_s;

  // check to see if the cycle difference is within a second +/- 500ppm window
  if(
     (*counter - *last_counter) > (expected_cycles+65536-EXPECTED_FREQ_500ppm_CYCLES) &&
     (*counter - *last_counter) < (expected_cycles+131072+EXPECTED_FREQ_500ppm_CYCLES) &&
     *last_counter != 0 &&
     last_events != 0
    ) {
    *counter -= 65536; // tim1 wrapped between cap and irq
    return -2;
  }

  return 0;
}

static float calculate_offset(float diff_start, float diff_end, float rtt) {
  return diff_start + rtt/2.0;  
}

static void get_timestamps(int fd, struct i2c_registers_type *page1) {
  uint8_t set_page[2];

  clock_gettime(CLOCK_REALTIME, &gpio_start);
  digitalWrite(TIMESTAMP_PIN, 1);
  clock_gettime(CLOCK_REALTIME, &gpio_end);
  digitalWrite(TIMESTAMP_PIN, 0);
  clock_gettime(CLOCK_REALTIME, &gpio_down);

  set_page[0] = I2C_REGISTER_OFFSET_PAGE;
  set_page[1] = I2C_REGISTER_PAGE1;

  lock_i2c(fd);
  write_i2c(fd, set_page, sizeof(set_page));
  read_i2c(fd, page1, sizeof(*page1));
  unlock_i2c(fd);
}

int main() {
  int fd;
  uint32_t last_ch2 = 0, last_ch4 = 0;
  uint8_t last_ch2_count = 0, last_ch4_count = 0;
  struct timespec gpio_last;

  if (wiringPiSetupGpio() == -1) {
    printf("wiring pi setup gpio error\n");
    exit(1);
  }
  pinMode(TIMESTAMP_PIN,OUTPUT);

  fd = open_i2c(I2C_ADDR); 

  while(1) {
    uint32_t ch2, ch4;
    struct i2c_registers_type page1;
    float system_s, system_s_end, ch2_s, diff_start, diff_end, offset, gpio_rtt_s;
    int32_t sleep_time;
    struct timespec gpio_rtt, gpio_interval, gpio_width;
    int status, state = 0;

    get_timestamps(fd, &page1);

    sub_timespecs3(&gpio_rtt, &gpio_end, &gpio_start);
    sub_timespecs3(&gpio_interval, &gpio_start, &gpio_last);
    sub_timespecs3(&gpio_width, &gpio_down, &gpio_start);

    status = calculate_counter(page1.tim3_at_cap[1], page1.tim1_at_irq[1], &last_ch2, &ch2, last_ch2_count, 1.0);
    if(status == -1) { // TODO: reset last_ch2_count?
      fprintf(stderr,"ch2 unchanged count: %u->%u\n", last_ch2_count, page1.ch2_count);
      sleep(1);
      continue;
    }
    if(status == -2) {
      state |= STATE_CH2_WRAP;
    }

    status = calculate_counter(page1.tim3_at_cap[2], page1.tim1_at_irq[2], &last_ch4, &ch4, last_ch4_count, timespec_to_double(&gpio_interval));
    if(status == -1) {
      fprintf(stderr,"ch4 unchanged count: %u->%u\n", last_ch4_count, page1.ch4_count);
      sleep(1);
      continue;
    }
    if(status == -2) {
      state |= STATE_CH4_WRAP;
    }

    // ch2_s : how long ago the ch2 signal happened in seconds
    ch2_s = (ch4-ch2)/48000000.0;
    // system_s : how many fractional seconds since the top of the second gpio timestamp
    system_s = gpio_start.tv_nsec / 1000000000.0;
    // system_s_end : how many fractional seconds since the top of the second gpio timestamp ended
    system_s_end = gpio_end.tv_nsec / 1000000000.0;

    sleep_time = 1005000-ch2_s*1000000.0;
    if(sleep_time < 100000) {
      sleep_time = 100000;
    } else if(sleep_time > 1000000) {
      sleep_time = 990000;
    }

    diff_start = system_s-ch2_s;
    diff_end = system_s_end-ch2_s;

    // system_s = 0.9, ch2_s=0.001 offset: +0.899=-0.101
    // system_s = 0, ch2_s=0.001 offset:-0.001
    // system_s = 0.9, ch2_s=0.9 offset:0
    // system_s = 0, ch2_s=0.9 offset: -0.9=+0.1
    // system_s = 0, ch2_s=0.5 offset: -0.5
    // system_s = 0.5, ch_s=0.001 offset: 0.499

    if(diff_start > 0.5) { // assumption: system clock is within +/- 499ms
      diff_start = diff_start - 1;
    } else if(diff_start < -0.5) {
      diff_start = diff_start + 1;
    }
    if(diff_end > 0.5) { // assumption: system clock is within +/- 499ms
      diff_end = diff_end - 1;
    } else if(diff_end < -0.5) {
      diff_end = diff_end + 1;
    }

    gpio_rtt_s = timespec_to_double(&gpio_rtt);
    offset = calculate_offset(diff_start, diff_end, gpio_rtt_s);

    if(gpio_rtt_s < MAX_GPIO_LATENCY) {
      send_offset(offset, &gpio_start);
    } else {
      state |= STATE_MAX_LATENCY;
    }

    printf("%lu %u %u %.9f %.9f %.9f %.9f %.9f %d %.9f ", time(NULL), ch4, ch2, ch2_s, system_s, diff_start, diff_end, offset, sleep_time, gpio_rtt_s);
    print_timespec(&gpio_interval);
    printf(" %x\n", state);


    last_ch2 = ch2;
    last_ch2_count = page1.ch2_count;

    last_ch4 = ch4;
    last_ch4_count = page1.ch4_count;
    gpio_last.tv_sec = gpio_start.tv_sec;
    gpio_last.tv_nsec = gpio_start.tv_nsec;

    usleep(sleep_time);
  }
}
