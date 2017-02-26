#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#include "i2c.h"

uint8_t read_i2c_register(int fd, uint8_t reg) {
  write_i2c(fd, &reg, 1);

  read_i2c(fd, &reg, 1);
  return reg;
}

void write_i2c_register(int fd, uint8_t reg, uint8_t val) {
  uint8_t data[2];

  data[0] = reg;
  data[1] = val;

  write_i2c(fd, &data, 2);
}

void write_i2c(int fd, void *buffer, ssize_t len) {
  ssize_t status;

  status = write(fd, buffer, len);
  if(status < 0) {
    perror("write to i2c failed");
    exit(1);
  }
  if(status != len) {
    printf("write not %d bytes: %d\n", len, status);
    exit(1);
  }
}

void read_i2c(int fd, void *buffer, ssize_t len) {
  ssize_t status;

  status = read(fd, buffer, len);
  if(status < 0) {
    perror("read to i2c failed");
    exit(1);
  }
  if(status != len) {
    printf("read not %d bytes: %d\n", len, status);
    exit(1);
  }
}

int open_i2c(uint16_t i2c_addr) {
  int fd;

  fd = open("/dev/i2c-1", O_RDWR);
  if (fd < 0) {
    perror("open /dev/i2c-1 failed");
    exit(1);
  }

  if (ioctl(fd, I2C_SLAVE, i2c_addr) < 0) {
    perror("ioctl i2c slave addr failed");
    exit(1);
  }

  return fd;
}

