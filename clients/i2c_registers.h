#ifndef I2C_REGISTERS_H
#define I2C_REGISTERS_H

#define I2C_ADDR 0x4
#define EXPECTED_FREQ 48000000
#define INPUT_CHANNELS 3

// i2c interface
#define I2C_REGISTER_OFFSET_PAGE 31
#define I2C_REGISTER_PAGE1 0
#define I2C_REGISTER_PAGE2 1

struct i2c_registers_type {
  uint32_t milliseconds_now;
  uint32_t milliseconds_irq_ch1;
  uint16_t tim3_at_irq[INPUT_CHANNELS];
  uint16_t tim1_at_irq[INPUT_CHANNELS];
  uint16_t tim3_at_cap[INPUT_CHANNELS];
  uint16_t source_HZ_ch1;
  uint8_t ch2_count;
  uint8_t ch4_count;
  uint8_t version;
  uint8_t page_offset;
};

struct i2c_registers_type_page2 {
  uint32_t last_adc_ms;
  uint16_t internal_temp;
  uint16_t internal_vref;
  uint16_t external_temp;
  uint16_t ts_cal1;     // internal_temp value at 30C+/-5C @3.3V+/-10mV
  uint16_t ts_cal2;     // internal_temp value at 110C+/-5C @3.3V+/-10mV
  uint16_t vrefint_cal; // internal_vref value at 30C+/-5C @3.3V+/-10mV
  uint8_t reserved[15];
  uint8_t page_offset;
};

void get_i2c_structs(int fd, struct i2c_registers_type *i2c_registers, struct i2c_registers_type_page2 *i2c_registers_page2);
float last_i2c_time();

#endif
