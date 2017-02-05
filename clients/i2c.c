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

void write_i2c(int fd, void *buffer, ssize_t len) {
  ssize_t status;

  status = write(fd, buffer, len);
  if(status < 0) {
    perror("write to i2c failed");
    exit(1);
  }
  if(status != len) {
    printf("write not %zu bytes: %zu\n", len, status);
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
    printf("read not %zu bytes: %zu\n", len, status);
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

