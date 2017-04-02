#ifndef I2C_H
#define I2C_H

void write_i2c(int fd, void *buffer, ssize_t len);
void read_i2c(int fd, void *buffer, ssize_t len);
int open_i2c(uint16_t i2c_addr);
int unlock_i2c(int fd);
int lock_i2c(int fd);
void write_i2c_register(int fd, uint8_t reg, uint8_t val);
uint8_t read_i2c_register(int fd, uint8_t reg);

#endif
