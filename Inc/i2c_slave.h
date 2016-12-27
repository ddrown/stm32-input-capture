#ifndef I2C_SLAVE
#define I2C_SLAVE

extern I2C_HandleTypeDef hi2c1;

void i2c_slave_start();
void i2c_show_data();

// 28 bytes (max smbus transaction: 32 bytes)
extern struct i2c_registers_type {
  uint32_t milliseconds_now;
  uint32_t milliseconds_irq_ch1;
  uint16_t tim3_at_irq[3];
  uint16_t tim1_at_irq[3];
  uint16_t tim3_at_cap[3];
  uint16_t source_HZ_ch1;
} i2c_registers;

#endif
