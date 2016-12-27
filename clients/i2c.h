#ifndef I2C_H
#define I2C_H

void write_i2c(int fd, void *buffer, ssize_t len);
void read_i2c(int fd, void *buffer, ssize_t len);
int open_i2c(uint16_t i2c_addr);

#endif
