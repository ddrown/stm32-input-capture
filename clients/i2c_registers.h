#ifndef I2C_REGISTERS_H
#define I2C_REGISTERS_H

#define I2C_ADDR 0x4
#define EXPECTED_FREQ 48000000
#define INPUT_CHANNELS 3

// i2c interface
#define I2C_REGISTER_OFFSET_PAGE 31
#define I2C_REGISTER_PAGE1 0
#define I2C_REGISTER_PAGE2 1
#define I2C_REGISTER_PAGE3 2
#define I2C_REGISTER_PAGE4 3
#define I2C_REGISTER_VERSION 2

#define SAVE_STATUS_NONE 0
#define SAVE_STATUS_OK 1
#define SAVE_STATUS_ERASE_FAIL 2
#define SAVE_STATUS_WRITE_FAIL 3

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

// write from tcxo_a to save
#define I2C_PAGE3_WRITE_LENGTH 20
struct i2c_registers_type_page3 {
  /* tcxo_X variables are floats stored as:
   * byte 1: negative sign (1 bit), exponent bits 7-1
   * byte 2: exponent bit 0, mantissa bits 23-17
   * byte 3: mantissa bits 16-8
   * byte 4: mantissa bits 7-0
   * they describe the expected frequency error in ppm:
   * ppm = tcxo_a + tcxo_b * (F - tcxo_c) + tcxo_d * pow(F - tcxo_c, 2)
   * where F is the temperature from the internal_temp sensor in Fahrenheit
   */
  uint32_t tcxo_a;
  uint32_t tcxo_b;
  uint32_t tcxo_c;
  uint32_t tcxo_d;
  uint8_t max_calibration_temp; // F
  int8_t min_calibration_temp;  // F
  uint8_t rmse_fit;             // ppb
  uint8_t save;                 // 1=save new values to flash
  uint8_t save_status;          // see SAVE_STATUS_X

  uint8_t reserved[10];

  uint8_t page_offset;
};

struct i2c_registers_type_page4 {
  uint16_t tim3;
  uint16_t tim1;
  uint8_t reserved[27];
  uint8_t page_offset;
};

void get_i2c_structs(int fd, struct i2c_registers_type *i2c_registers, struct i2c_registers_type_page2 *i2c_registers_page2);
float last_i2c_time();

#endif
