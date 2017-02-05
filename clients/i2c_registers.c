#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "i2c_registers.h"
#include "i2c.h"

static float i2c_time = 0;

float last_i2c_time() {
  return i2c_time;
}

void get_i2c_structs(int fd, struct i2c_registers_type *i2c_registers, struct i2c_registers_type_page2 *i2c_registers_page2) {
  uint8_t set_page[2];
  struct timeval start,end;

  gettimeofday(&start, NULL);
  set_page[0] = I2C_REGISTER_OFFSET_PAGE;
  set_page[1] = I2C_REGISTER_PAGE1;
  write_i2c(fd, set_page, sizeof(set_page));
  read_i2c(fd, i2c_registers, sizeof(struct i2c_registers_type));

  if(i2c_registers->page_offset != I2C_REGISTER_PAGE1) {
    printf("got wrong page offset: %u != %u\n", i2c_registers->page_offset, I2C_REGISTER_PAGE1);
    exit(1);
  }

  if(i2c_registers->version != 1) { // TODO: handle compatability
    printf("got unexpected version: %u != %u\n", i2c_registers->version, 1);
    exit(1);
  }

  set_page[0] = I2C_REGISTER_OFFSET_PAGE;
  set_page[1] = I2C_REGISTER_PAGE2;
  write_i2c(fd, set_page, sizeof(set_page));
  read_i2c(fd, i2c_registers_page2, sizeof(struct i2c_registers_type_page2));

  if(i2c_registers_page2->page_offset != I2C_REGISTER_PAGE2) {
    printf("got wrong page offset: %u != %u\n", i2c_registers_page2->page_offset, I2C_REGISTER_PAGE2);
    exit(1);
  }
  gettimeofday(&end, NULL);

  end.tv_sec -= start.tv_sec;
  if(end.tv_usec < start.tv_usec) {
    end.tv_sec--;
    end.tv_usec = end.tv_usec - start.tv_usec + 1000000;
  } else {
    end.tv_usec = end.tv_usec - start.tv_usec;
  }
  i2c_time = end.tv_sec + end.tv_usec / 1000000.0;
}
