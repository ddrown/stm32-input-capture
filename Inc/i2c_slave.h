#ifndef I2C_SLAVE
#define I2C_SLAVE

extern I2C_HandleTypeDef hi2c1;

void i2c_slave_start();
void i2c_show_data();
uint8_t i2c_read_active();

#define I2C_REGISTER_PAGE_SIZE 32

#define I2C_REGISTER_OFFSET_HZ_HI 26
#define I2C_REGISTER_OFFSET_HZ_LO 27
#define I2C_REGISTER_OFFSET_PAGE 31

#define I2C_REGISTER_PAGE1 0
#define I2C_REGISTER_PAGE2 1

#define I2C_REGISTER_VERSION 1

extern struct i2c_registers_type {
  // start 0 len 4
  uint32_t milliseconds_now;
  // start 4 len 4
  uint32_t milliseconds_irq_ch1;
  // start 8 len 6
  uint16_t tim3_at_irq[3];
  // start 14 len 6
  uint16_t tim1_at_irq[3];
  // start 20 len 6
  uint16_t tim3_at_cap[3];
  // start 26 len 2
  uint16_t source_HZ_ch1;
  // start 28 len 1
  uint8_t ch2_count;
  // start 29 len 1
  uint8_t ch4_count;
  // start 30 len 1
  uint8_t version;
  // start 31 len 1
  uint8_t page_offset;
} i2c_registers;

extern struct i2c_registers_type_page2 {
  uint32_t last_adc_ms;
  uint16_t internal_temp;
  uint16_t internal_vref;
  uint16_t external_temp;
  uint16_t ts_cal1;     // internal_temp value at 30C+/-5C @3.3V+/-10mV
  uint16_t ts_cal2;     // internal_temp value at 110C+/-5C @3.3V+/-10mV
  uint16_t vrefint_cal; // internal_vref value at 30C+/-5C @3.3V+/-10mV
  uint8_t reserved[15];
  uint8_t page_offset;
} i2c_registers_page2;

#endif
