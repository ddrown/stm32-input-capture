#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "i2c.h"

#define I2C_ADDR 0x51

// register offsets
#define PCF2129_PWRSTAT 0x2
#define PCF2129_CLKOUT  0xF
#define PCF2129_AGING   0x19

// power status flags
#define PCF2129_PWR_BTSE 0b10000
#define PCF2129_PWR_BF    0b1000
#define PCF2129_PWR_BLF    0b100
#define PCF2129_PWR_BIE     0b10
#define PCF2129_PWR_BLIE     0b1

// temperature measurement interval
#define PCF2129_CLK_TCR_4min 0b00
#define PCF2129_CLK_TCR_2min 0b01
#define PCF2129_CLK_TCR_1min 0b10
#define PCF2129_CLK_TCR_30s  0b11

// clkout frequencies
#define PCF2129_CLK_32768   0b000
#define PCF2129_CLK_16384   0b001
#define PCF2129_CLK_8192    0b010
#define PCF2129_CLK_4096    0b011
#define PCF2129_CLK_2048    0b100
#define PCF2129_CLK_1024    0b101
#define PCF2129_CLK_1       0b110
#define PCF2129_CLK_DISABLE 0b111

char *freqs[] = {"32k", "16k", "8k", "4k", "2k", "1k", "1", "disable"};

static uint8_t read_i2c_register(int fd, uint8_t reg) {
  write_i2c(fd, &reg, 1);

  read_i2c(fd, &reg, 1);
  return reg;
}

static void write_i2c_register(int fd, uint8_t reg, uint8_t val) {
  uint8_t data[2];

  data[0] = reg;
  data[1] = val;

  write_i2c(fd, &data, 2);
}

int main(int argc, char **argv) {
  int fd;

  fd = open_i2c(I2C_ADDR); 

  if(argc == 1) {
    printf("commands: getadj, setadj, gettcr, settcr, getclk, setclk, getpwr\n");
    exit(1);
  }

  if(strcmp(argv[1], "getpwr") == 0) {
    int8_t pwr;

    pwr = read_i2c_register(fd, PCF2129_PWRSTAT);

    printf("%x pwr stats: ",pwr);
    if(pwr & PCF2129_PWR_BTSE) {
      printf("btse ");
    }
    if(pwr & PCF2129_PWR_BF) {
      printf("bf ");
    }
    if(pwr & PCF2129_PWR_BLF) {
      printf("blf ");
    }
    if(pwr & PCF2129_PWR_BIE) {
      printf("bie ");
    }
    if(pwr & PCF2129_PWR_BLIE) {
      printf("blie ");
    }
    printf("\n");
    exit(0);
  }

  if(strcmp(argv[1], "gettcr") == 0) {
    int8_t tcr;
    double interval;

    tcr = read_i2c_register(fd, PCF2129_CLKOUT);

    tcr = (tcr >> 6) & 0b11;
    interval = 4.0 / pow(2, tcr);

    printf("%d - tempcomp every %0.1f minutes\n", tcr, interval);
    exit(0);
  }

  if(strcmp(argv[1], "settcr") == 0) {
    uint8_t tcr, newtcr;

    tcr = read_i2c_register(fd, PCF2129_CLKOUT);
    if(argc < 3) { 
      printf("settcr requires an argument\n");
      exit(1);
    }
    newtcr = atoi(argv[2]);
    switch (newtcr) {
      case 4:
        newtcr = PCF2129_CLK_TCR_4min;
        break;
      case 2:
        newtcr = PCF2129_CLK_TCR_2min;
        break;
      case 1:
        newtcr = PCF2129_CLK_TCR_1min;
        break;
      case 0:
        newtcr = PCF2129_CLK_TCR_30s;
        break;
      default:
        printf("invalid tcr given: %s, valid values: 4,2,1,0\n", argv[2]);
        exit(1);
	break;
    }
    write_i2c_register(fd, PCF2129_CLKOUT, (newtcr << 6) | (tcr & 0b00111111));
    printf("set to %d\n", newtcr);
    exit(0);
  }

  if(strcmp(argv[1], "getclk") == 0) {
    int8_t tcr;

    tcr = read_i2c_register(fd, PCF2129_CLKOUT);
    tcr = tcr & 0b111;

    printf("%d - clockout %sHz\n", tcr, freqs[tcr]);
    exit(0);
  }

  if(strcmp(argv[1], "setclk") == 0) {
    uint8_t tcr;
    int newtcr;

    tcr = read_i2c_register(fd, PCF2129_CLKOUT);
    if(argc < 3) { 
      printf("settcr requires an argument\n");
      exit(1);
    }
    newtcr = atoi(argv[2]);
    switch (newtcr) {
      case 32768:
        newtcr = PCF2129_CLK_32768;
        break;
      case 16384:
        newtcr = PCF2129_CLK_16384;
        break;
      case 8192:
        newtcr = PCF2129_CLK_8192;
        break;
      case 4096:
        newtcr = PCF2129_CLK_4096;
        break;
      case 2048:
        newtcr = PCF2129_CLK_2048;
        break;
      case 1024:
        newtcr = PCF2129_CLK_1024;
        break;
      case 1:
        newtcr = PCF2129_CLK_1;
        break;
      case 0:
        newtcr = PCF2129_CLK_DISABLE;
        break;
      default:
        printf("invalid clk given: %s, valid values: 32768,16384,8192,4096,2048,1024,1,0\n", argv[2]);
        exit(1);
	break;
    }
    write_i2c_register(fd, PCF2129_CLKOUT, newtcr | (tcr & 0b11111000));
    printf("set to %d (%sHz)\n", newtcr, freqs[newtcr]);
    exit(0);
  }

  if(strcmp(argv[1], "getadj") == 0) {
    int8_t adj;

    adj = read_i2c_register(fd, PCF2129_AGING);

    adj = adj & 0b1111;

    printf("%d (~%d ppm)\n", adj, 0b1000 - adj); 
    exit(0);
  }

  if(strcmp(argv[1], "setadj") == 0) {
    int8_t newadj;
    if(argc < 3) { 
      printf("setadj requires an argument\n");
      exit(1);
    }
    newadj = atoi(argv[2]);
    newadj = 8 - newadj;
    if(newadj < 0 || newadj > 15) {
      printf("error: max +8 min -7\n");
      exit(1);
    }
    write_i2c_register(fd, PCF2129_AGING, newadj);
    printf("set to %d\n", newadj);
    exit(0);
  }

  printf("invalid command\n");
  exit(1);
}
