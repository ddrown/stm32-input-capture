#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <time.h>

#include "i2c.h"

#define I2C_ADDR 0x77

// register offsets
#define BME280_ID          0xD0
#define BME280_CTRL_HUMID  0xF2
#define BME280_STATUS      0xF3
#define BME280_CTRL_MEAS   0xF4
#define BME280_CONFIG      0xF5
#define BME280_CALIBRATION_0_25 0x88
#define BME280_CALIBRATION_26_41 0xE1
struct calibration_data {
  uint16_t T1;
  int16_t T2;
  int16_t T3;
  uint16_t P1;
  int16_t P2_9[8];
  uint8_t H1;
  int16_t H2;
  uint8_t H3;
  int16_t H4;
  int16_t H5;
  int8_t H6;
};
struct raw_adc_data {
  uint32_t temp, pressure;
  uint16_t humidity;
};

// register offsets, adc data
#define BME280_PRESSURE    0xF7
#define BME280_TEMPERATURE 0xFA
#define BME280_HUMIDITY    0xFD

// CTRL_HUMID settings
#define CTRL_HUMID_OFF 0b000
#define CTRL_HUMID_1X  0b001
#define CTRL_HUMID_2X  0b010
#define CTRL_HUMID_4X  0b011
#define CTRL_HUMID_8X  0b100
#define CTRL_HUMID_16X 0b101

// STATUS bitflags
#define STATUS_MEASURING 0b1000
#define STATUS_B2         0b100
#define STATUS_REG_UPDATE   0b1

// CTRL_MEAS settings
#define CTRL_MEAS_TEMP_OFF 0b00000000
#define CTRL_MEAS_TEMP_1X  0b00100000
#define CTRL_MEAS_TEMP_2X  0b01000000
#define CTRL_MEAS_TEMP_4X  0b01100000
#define CTRL_MEAS_TEMP_8X  0b10000000
#define CTRL_MEAS_TEMP_16X 0b10100000
#define CTRL_MEAS_PRES_OFF    0b00000
#define CTRL_MEAS_PRES_1X     0b00100
#define CTRL_MEAS_PRES_2X     0b01000
#define CTRL_MEAS_PRES_4X     0b01100
#define CTRL_MEAS_PRES_8X     0b10000
#define CTRL_MEAS_PRES_16X    0b10100
#define CTRL_MEAS_MODE_SLEEP     0b00
#define CTRL_MEAS_MODE_FORCED    0b01
#define CTRL_MEAS_MODE_NORMAL    0b11
const char *oversample_names[] = {"off", "1x", "2x", "4x", "8x", "16x", "16x", "16x"};
const char *mode_names[] = {"sleep","forced","normal","normal"};

// CONFIG settings
#define CONFIG_STANDBY_0_5ms  0b00000000
#define CONFIG_STANDBY_62_5ms 0b00100000
#define CONFIG_STANDBY_125ms  0b01000000
#define CONFIG_STANDBY_250ms  0b01100000
#define CONFIG_STANDBY_500ms  0b10000000
#define CONFIG_STANDBY_1000ms 0b10100000
#define CONFIG_STANDBY_10ms   0b11000000
#define CONFIG_STANDBY_20ms   0b11100000
#define CONFIG_IIR_FILTER_OFF    0b00000
#define CONFIG_IIR_FILTER_2      0b00100
#define CONFIG_IIR_FILTER_4      0b01000
#define CONFIG_IIR_FILTER_8      0b01100
#define CONFIG_IIR_FILTER_16     0b10000
#define CONFIG_SPI3W                 0b1
const char *standby_names[] = {"0.5ms","62.5ms","125ms","250ms","500ms","1s","10ms","20ms"};
const char *iir_filter_names[] = {"off","2","4","8","16","16","16","16"};

void read_calibration(int fd, struct calibration_data *c) {
  uint8_t calibration_0_25[26];
  uint8_t calibration_26_41[7]; // more data available here, but only 7 bytes are used
  uint8_t reg;

  lock_i2c(fd);
  reg = BME280_CALIBRATION_0_25;
  write_i2c(fd, &reg, 1);
  read_i2c(fd, &calibration_0_25, sizeof(calibration_0_25));

  reg = BME280_CALIBRATION_26_41;
  write_i2c(fd, &reg, 1);
  read_i2c(fd, &calibration_26_41, sizeof(calibration_26_41));
  unlock_i2c(fd);

  c->T1 = calibration_0_25[0] | (calibration_0_25[1] << 8);
  c->T2 = calibration_0_25[2] | (calibration_0_25[3] << 8);
  c->T3 = calibration_0_25[4] | (calibration_0_25[5] << 8);

  c->P1 = calibration_0_25[6] | (calibration_0_25[7] << 8);
  for(uint8_t i = 0; i < 8; i++) {
    c->P2_9[i] = calibration_0_25[8+2*i] | (calibration_0_25[9+2*i] << 8);
  }

  c->H1 = calibration_0_25[25];
  c->H2 = calibration_26_41[0] | (calibration_26_41[1] << 8);
  c->H3 = calibration_26_41[2];
  c->H4 = (calibration_26_41[3] << 4) | (calibration_26_41[4] & 0b1111);
  c->H5 = ((calibration_26_41[4] & 0b11110000) >> 4) | (calibration_26_41[5] << 4);
  c->H6 = calibration_26_41[6];
}

void read_raw_adc(int fd, struct raw_adc_data *r) {
  uint8_t d[8];
  uint8_t reg = BME280_PRESSURE;

  lock_i2c(fd);
  write_i2c(fd, &reg, 1);
  read_i2c(fd, &d, sizeof(d));
  unlock_i2c(fd);

  r->pressure = (d[2] >> 4) | (d[1] << 4) | (d[0] << 12);
  r->temp = (d[5] >> 4) | (d[4] << 4) | (d[3] << 12);
  r->humidity = (d[6] << 8) | d[7];
}

int32_t t_fine; // needed in calc_pressure and calc_humidity
// returns C, from BME280 datasheeet
float calc_temp(const struct raw_adc_data *r, const struct calibration_data *c) {
  int32_t var1, var2;
  float C;

  var1  = ((((r->temp>>3) - ((int32_t)c->T1<<1))) * ((int32_t)c->T2)) >> 11;
  var2  = (((((r->temp>>4) - ((int32_t)c->T1)) * ((r->temp>>4) - ((int32_t)c->T1))) >> 12) * ((int32_t)c->T3)) >> 14;
  t_fine = var1 + var2;
  C = (t_fine * 5 + 128) / 25600.0;
  return C;
}

// returns hPa, requires t_fine set (call calc_temp), from BME280 datasheet
float calc_pressure(const struct raw_adc_data *r, const struct calibration_data *c) {
  int64_t var1, var2, p;

  var1 = ((int64_t)t_fine) - 128000;
  var2 = var1 * var1 * (int64_t)c->P2_9[4];
  var2 = var2 + ((var1*(int64_t)c->P2_9[3])<<17);
  var2 = var2 + (((int64_t)c->P2_9[2])<<35);
  var1 = ((var1 * var1 * (int64_t)c->P2_9[1])>>8) + ((var1 * (int64_t)c->P2_9[0])<<12);
  var1 = (((((int64_t)1)<<47)+var1))*((int64_t)c->P1)>>33;
  if (var1 == 0) {
    return 0; // avoid exception caused by division by zero
  }
  p = 1048576 - r->pressure;
  p = (((p<<31)-var2)*3125)/var1;
  var1 = (((int64_t)c->P2_9[7]) * (p>>13) * (p>>13)) >> 25;
  var2 = (((int64_t)c->P2_9[6]) * p) >> 19;
  p = ((p + var1 + var2) >> 8) + (((int64_t)c->P2_9[5])<<4);

  return p / 25600.0;
}

// returns %RH, requires t_fine set (call calc_temp), from BME280 datasheet
float calc_humidity(const struct raw_adc_data *r, const struct calibration_data *c) {
  int32_t v_x1_u32r;

  v_x1_u32r = (t_fine - ((int32_t)76800));
  v_x1_u32r =
    (
      ((
        ((r->humidity << 14) - (((int32_t)c->H4) << 20) - (((int32_t)c->H5) * v_x1_u32r)) + ((int32_t)16384)
      ) >> 15) *
      (((
        ((((v_x1_u32r * ((int32_t)c->H6)) >> 10) * (((v_x1_u32r * ((int32_t)c->H3)) >> 11) + (( int32_t)32768))) >> 10) +
        ((int32_t)2097152)) * ((int32_t)c->H2) + 8192) >> 14
      )
    );
  v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((int32_t)c->H1)) >> 4));
  v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r); 
  v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);
  return v_x1_u32r / 4194304.0;
}

int main(int argc, char **argv) {
  int fd;

  fd = open_i2c(I2C_ADDR); 

  if(argc == 1) {
    printf("commands: show, force, getcalib, data, raw, id, stream\n");
    exit(1);
  }

  if(strcmp(argv[1], "id") == 0) {
    uint8_t id = read_i2c_register(fd, BME280_ID);
    printf("id = %x\n", id);
  }

  if(strcmp(argv[1], "show") == 0) {
    uint8_t d[4];
    uint8_t reg = BME280_CTRL_HUMID;
    lock_i2c(fd);
    write_i2c(fd, &reg, 1);
    read_i2c(fd, &d, sizeof(d));
    unlock_i2c(fd);

    printf("ctrl_humid = %x (h=%s)\n", d[0], oversample_names[d[0] & 0b111]);
    printf("status = %x%s%s%s\n", d[1],
      (d[1] & STATUS_MEASURING) ? " measuring" : "",
      (d[1] & STATUS_B2) ? " +b2" : "",
      (d[1] & STATUS_REG_UPDATE) ? " regupdate" : ""
    );
    printf("ctrl_meas = %x (t=%s, p=%s, m=%s)\n", d[2],
      oversample_names[(d[2] & 0b11100000) >> 5],
      oversample_names[(d[2] &    0b11100) >> 2],
      mode_names[       d[2] &       0b11]
    );
    printf("config = %x (standby=%s, iir=%s, spi=%s)\n", d[3],
      standby_names[(d[3] & 0b11100000) >> 5],
      iir_filter_names[(d[3] & 0b11100) >> 2],
      (d[3] & CONFIG_SPI3W) ? "3w" : "n"
    );

    exit(0);
  } 

  if(strcmp(argv[1], "raw") == 0) {
    struct raw_adc_data r;
    read_raw_adc(fd, &r);
    printf("P = %u T = %u H = %u\n", r.pressure, r.temp, r.humidity);

    exit(0);
  } 

  if(strcmp(argv[1], "force") == 0) {
    lock_i2c(fd);
    write_i2c_register(fd, BME280_CTRL_HUMID, CTRL_HUMID_1X);
    write_i2c_register(fd, BME280_CTRL_MEAS, CTRL_MEAS_TEMP_1X | CTRL_MEAS_PRES_1X | CTRL_MEAS_MODE_FORCED);
    unlock_i2c(fd);
    exit(0);
  } 

  if(strcmp(argv[1], "getcalib") == 0) {
    struct calibration_data c;
    read_calibration(fd, &c);

    printf("T1 = %u T2 = %d T3 = %d\n", c.T1, c.T2, c.T3);
    printf("P1 = %u P2 = %d P3 = %d\n", c.P1, c.P2_9[0], c.P2_9[1]);
    printf("P4 = %d P5 = %d P6 = %d\n", c.P2_9[2], c.P2_9[3], c.P2_9[4]);
    printf("P7 = %d P8 = %d P9 = %d\n", c.P2_9[5], c.P2_9[6], c.P2_9[7]);
    printf("H1 = %u H2 = %d H3 = %u\n", c.H1, c.H2, c.H3);
    printf("H4 = %d H5 = %d H6 = %d\n", c.H4, c.H5, c.H6);

    exit(0);
  }

  if(strcmp(argv[1], "data") == 0) {
    struct raw_adc_data r;
    struct calibration_data c;
    float C, F, pressure, RH;
    uint8_t status;

    lock_i2c(fd);
    write_i2c_register(fd, BME280_CTRL_HUMID, CTRL_HUMID_1X);
    write_i2c_register(fd, BME280_CTRL_MEAS, CTRL_MEAS_TEMP_1X | CTRL_MEAS_PRES_1X | CTRL_MEAS_MODE_FORCED);
    unlock_i2c(fd);

    usleep(20000);
    lock_i2c(fd);
    status = read_i2c_register(fd, BME280_STATUS);
    unlock_i2c(fd);

    printf("status = %x\n", status);

    read_raw_adc(fd, &r);
    read_calibration(fd, &c);

    C = calc_temp(&r, &c);
    F = C * 9 / 5 + 32;
    printf("T = %.2fC %.3fF (raw=%u)\n", C, F, r.temp);

    pressure = calc_pressure(&r, &c);
    printf("P = %.3fhPa (raw=%u)\n", pressure, r.pressure);

    RH = calc_humidity(&r, &c);
    printf("H = %.3f%% RH (raw=%u)\n", RH, r.humidity);
    exit(0);
  }

  if(strcmp(argv[1], "stream") == 0) {
    struct raw_adc_data r;
    struct calibration_data c;
    float C, F, pressure, RH;
    uint8_t status;

    read_calibration(fd, &c);
    while(1) {
      lock_i2c(fd);
      write_i2c_register(fd, BME280_CTRL_HUMID, CTRL_HUMID_1X);
      write_i2c_register(fd, BME280_CTRL_MEAS, CTRL_MEAS_TEMP_1X | CTRL_MEAS_PRES_1X | CTRL_MEAS_MODE_FORCED);
      unlock_i2c(fd);

      usleep(20000);
      lock_i2c(fd);
      status = read_i2c_register(fd, BME280_STATUS);
      unlock_i2c(fd);

      read_raw_adc(fd, &r);

      C = calc_temp(&r, &c);
      F = C * 9 / 5 + 32;

      pressure = calc_pressure(&r, &c);

      RH = calc_humidity(&r, &c);

      printf("%ld %x %.2f %.3f %.3f %u %.3f\n", time(NULL), status, C, F, pressure, r.pressure, RH);
      usleep(980000);
    }
  }

  printf("invalid command\n");
  exit(1);
}
