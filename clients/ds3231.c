#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "i2c.h"

#define I2C_ADDR 0x68

// register offsets
#define DS3231_CONTROL        0x0E
#define DS3231_STATUSREG      0x0F
#define DS3231_AGING          0x10
#define DS3231_TEMPERATURE_HI 0x11
#define DS3231_TEMPERATURE_LO 0x12

// DS3231_CONTROL bits
#define DS3231_CTRL_ALRM1INT 0x01
#define DS3231_CTRL_ALRM2INT 0x02
#define DS3231_CTRL_INTCN    0x04
#define DS3231_CTRL_RS1      0x08
#define DS3231_CTRL_RS2      0x10
#define DS3231_CTRL_CONVTEMP 0x20
#define DS3231_CTRL_BBSQW    0x40
#define DS3231_CTRL_EOSC     0x80

// DS3231_STATUSREG bits
#define DS3231_STAT_A1F   0x01
#define DS3231_STAT_A2F   0x02
#define DS3231_STAT_BSY   0x04
#define DS3231_STAT_EN32K 0x08
#define DS3231_STAT_OSF   0x80

// DS3231_CTRL_RS1 / DS3231_CTRL_RS2 settings
#define DS3231_OUT_1HZ  0x00
#define DS3231_OUT_1KHZ DS3231_CTRL_RS1
#define DS3231_OUT_4KHZ DS3231_CTRL_RS2
#define DS3231_OUT_8KHZ (DS3231_CTRL_RS1|DS3231_CTRL_RS2)

int main(int argc, char **argv) {
  int fd;
  uint8_t ctrl;

  fd = open_i2c(I2C_ADDR); 

  if(argc == 1) {
    printf("commands: setsqw, gettemp, getadj, setadj\n");
    exit(1);
  }

  if(strcmp(argv[1], "setsqw") == 0) {
    printf("set square wave to 1HZ\n");
    ctrl = read_i2c_register(fd, DS3231_CONTROL);

    // turn off INTCON
    ctrl &= ~DS3231_CTRL_INTCN;
    // set RS1/RS2 to 0
    ctrl &= ~(DS3231_CTRL_RS1|DS3231_CTRL_RS2);
    ctrl |= DS3231_OUT_1HZ;
    write_i2c_register(fd, DS3231_CONTROL, ctrl);
    exit(0);
  }

  if(strcmp(argv[1], "gettemp") == 0) {
    uint8_t hi, lo;
    int16_t temp;

    hi = read_i2c_register(fd, DS3231_TEMPERATURE_HI);
    lo = read_i2c_register(fd, DS3231_TEMPERATURE_LO);

    temp = hi << 8 | lo;
    temp = temp / 64;
    printf("%.2f degC\n", temp/4.0); 
    exit(0);
  }

  if(strcmp(argv[1], "getadj") == 0) {
    int8_t adj;

    adj = read_i2c_register(fd, DS3231_AGING);

    printf("%d (~%.1f ppm)\n", adj, adj/10.0); 
    exit(0);
  }

  if(strcmp(argv[1], "setadj") == 0) {
    int8_t newadj;
    if(argc < 3) { 
      printf("setadj requires an argument\n");
      exit(1);
    }
    newadj = atoi(argv[2]);
    write_i2c_register(fd, DS3231_AGING, newadj);
    printf("set to %d\n", newadj);
    exit(0);
  }

  printf("invalid command\n");
  exit(1);
}
