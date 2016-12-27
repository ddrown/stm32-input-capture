#ifndef I2C_SLAVE
#define I2C_SLAVE

extern I2C_HandleTypeDef hi2c1;

void i2c_slave_start();
void i2c_show_data();

extern struct i2c_registers_type {
  uint32_t milliseconds_now;
  uint32_t milliseconds_irq;
  uint16_t tim3_at_irq;
  uint16_t tim1_at_irq;
  uint16_t tim3_at_cap;
  uint16_t source_HZ;
} i2c_registers;

#endif
