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

#include "i2c.h"
#include "i2c_registers.h"
#include "timespec.h"

// 8 bits at 400khz = 20us to account for i2c address transmit
#define REQUEST_LATENCY  0.000020
// 32 bits at 400khz = 80us to account for data transmit over i2c
#define RESPONSE_LATENCY 0.000080

static struct timespec i2c_start,i2c_end;

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

static float calculate_offset(float diff_start, float diff_end, float rtt) {
  rtt = rtt - REQUEST_LATENCY - RESPONSE_LATENCY;

  return diff_start + rtt/2.0;  
}

static uint32_t get_i2c_tim(int fd, struct i2c_registers_type *page1) {
  uint8_t set_page[2];
  uint32_t tim;

  set_page[0] = I2C_REGISTER_OFFSET_PAGE;
  set_page[1] = I2C_REGISTER_PAGE4;
  lock_i2c(fd);
  write_i2c(fd, set_page, sizeof(set_page));

  clock_gettime(CLOCK_REALTIME, &i2c_start);
  read_i2c(fd, &tim, sizeof(tim));
  clock_gettime(CLOCK_REALTIME, &i2c_end);

  set_page[1] = I2C_REGISTER_PAGE1;
  write_i2c(fd, set_page, sizeof(set_page));
  read_i2c(fd, page1, sizeof(*page1));

  unlock_i2c(fd);

  return tim;
}

// 89s per 32 bit wrap
#define NS_PER_WRAP 89478485333.333

#define STATE_CH2_WRAP 0b1

int main() {
  int fd;
  uint32_t last_ch2 = 0;
  uint8_t last_ch2_count = 0;

  fd = open_i2c(I2C_ADDR); 

  while(1) {
    uint32_t tim;
    uint32_t ch2;
    struct i2c_registers_type page1;
    float system_s, system_s_end, ch2_s, diff_start, diff_end, offset;
    int32_t sleep_time;
    uint32_t states = 0;
    struct timespec i2c_rtt;

    tim = get_i2c_tim(fd, &page1);
    ch2 = page1.tim3_at_cap[1] | ((uint32_t)page1.tim1_at_irq[1])<<16;
    if(ch2 == last_ch2) {
      fprintf(stderr,"ch2 unchanged count: %u->%u\n", last_ch2_count, page1.ch2_count);
      sleep(1);
      continue;
    }

    if((ch2-last_ch2) > 48065536 && last_ch2 != 0 && last_ch2_count != 0) {
      ch2 -= 65536; // tim1 wrapped between cap and irq
      states |= STATE_CH2_WRAP;
    }

    // ch2_s : how long ago the ch2 signal happened in seconds
    ch2_s = (tim-ch2)/48000000.0;
    // system_s : how many fractional seconds since the top of the second i2c transaction started
    system_s = i2c_start.tv_nsec / 1000000000.0;
    // system_s_end : how many fractional seconds since the top of the second i2c transaction ended
    system_s_end = i2c_end.tv_nsec / 1000000000.0;

    sleep_time = 1005000-ch2_s*1000000.0;
    if(sleep_time < 100000) {
      sleep_time = 100000;
    } else if(sleep_time > 1000000) {
      sleep_time = 990000;
    }

    // remove known latencies
    system_s = system_s + REQUEST_LATENCY;
    system_s_end = system_s_end - RESPONSE_LATENCY;

    diff_start = system_s-ch2_s;
    diff_end = system_s_end-ch2_s;

    i2c_rtt.tv_sec = i2c_end.tv_sec;
    i2c_rtt.tv_nsec = i2c_end.tv_nsec;
    sub_timespecs(&i2c_rtt, &i2c_start);

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

    offset = calculate_offset(diff_start, diff_end, timespec_to_double(&i2c_rtt));

    printf("%u %u %.9f %.9f %.9f %.9f %.9f %d ", tim, ch2, ch2_s, system_s, diff_start, diff_end, offset, sleep_time);
    print_timespec(&i2c_rtt);
    printf(" %x\n", states);

    send_offset(offset, &i2c_start);

    last_ch2 = ch2;
    last_ch2_count = page1.ch2_count;

    usleep(sleep_time);
  }
}
