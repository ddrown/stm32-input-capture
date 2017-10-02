#include "stm32f0xx_hal.h"
#include <string.h>

#include "i2c_slave.h"
#include "uart.h"
#include "timer.h"
#include "flash.h"

struct i2c_registers_type i2c_registers;
struct i2c_registers_type_page2 i2c_registers_page2;
struct i2c_registers_type_page3 i2c_registers_page3;
struct i2c_registers_type_page4 i2c_registers_page4;

static void *current_page = &i2c_registers;
static uint8_t current_page_data[I2C_REGISTER_PAGE_SIZE];

static void i2c_data_xmt(I2C_HandleTypeDef *hi2c);

static uint8_t i2c_transfer_position;
static enum {STATE_WAITING, STATE_GET_ADDR, STATE_GET_DATA, STATE_SEND_DATA, STATE_DROP_DATA} i2c_transfer_state;
static uint8_t i2c_data;

// addresses from the STM32F030 datasheet
uint16_t *ts_cal1 = (uint16_t *)0x1ffff7b8;
uint16_t *ts_cal2 = (uint16_t *)0x1ffff7c2;
uint16_t *vrefint_cal = (uint16_t *)0x1ffff7ba;

void i2c_slave_start() {
  memset(&i2c_registers, '\0', sizeof(i2c_registers));

  memset(&i2c_registers_page2, '\0', sizeof(i2c_registers_page2));

  memset(&i2c_registers_page3, '\0', sizeof(i2c_registers_page3));

  memset(&i2c_registers_page4, '\0', sizeof(i2c_registers_page4));

  i2c_registers.page_offset = I2C_REGISTER_PAGE1;
  i2c_registers.source_HZ_ch1 = DEFAULT_SOURCE_HZ;
  i2c_registers.version = I2C_REGISTER_VERSION;
  memcpy(current_page_data, current_page, I2C_REGISTER_PAGE_SIZE);

  i2c_registers_page2.page_offset = I2C_REGISTER_PAGE2;
  i2c_registers_page2.ts_cal1 = *ts_cal1;
  i2c_registers_page2.ts_cal2 = *ts_cal2;
  i2c_registers_page2.vrefint_cal = *vrefint_cal;

  i2c_registers_page3.page_offset = I2C_REGISTER_PAGE3;
  i2c_registers_page3.tcxo_a = tcxo_calibration[0];
  i2c_registers_page3.tcxo_b = tcxo_calibration[1];
  i2c_registers_page3.tcxo_c = tcxo_calibration[2];
  i2c_registers_page3.tcxo_d = tcxo_calibration[3];
  i2c_registers_page3.max_calibration_temp = tcxo_calibration[4] & 0xff;
  i2c_registers_page3.min_calibration_temp = (tcxo_calibration[4] >> 8) & 0xff;
  i2c_registers_page3.rmse_fit = (tcxo_calibration[4] >> 16) & 0xff;

  i2c_registers_page4.page_offset = I2C_REGISTER_PAGE4;

  HAL_I2C_EnableListen_IT(&hi2c1);
}

uint8_t i2c_read_active() {
  return (i2c_transfer_state == STATE_SEND_DATA) || (i2c_transfer_state == STATE_GET_ADDR);
}

void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c, uint8_t TransferDirection, uint16_t AddrMatchCode) {
  if(TransferDirection == I2C_DIRECTION_TRANSMIT) { // master transmit
    i2c_transfer_state = STATE_GET_ADDR;
    HAL_I2C_Slave_Sequential_Receive_IT(hi2c, &i2c_data, 1, I2C_FIRST_FRAME);
  } else {
    i2c_transfer_state = STATE_SEND_DATA;
    i2c_data_xmt(hi2c);
  }
}

static void change_page(uint8_t data) {
  switch(data) {
    case I2C_REGISTER_PAGE2:
      current_page = &i2c_registers_page2;
      break;
    case I2C_REGISTER_PAGE3:
      current_page = &i2c_registers_page3;
      break;
    case I2C_REGISTER_PAGE4:
      i2c_registers_page4.tim3 = __HAL_TIM_GET_COUNTER(&htim3);
      i2c_registers_page4.tim1 = __HAL_TIM_GET_COUNTER(&htim1);
      current_page = &i2c_registers_page4;
      break;
    default:
    case I2C_REGISTER_PAGE1:
      i2c_registers.milliseconds_now = HAL_GetTick();
      current_page = &i2c_registers;
      break;
  }

  __disable_irq(); // copy with interrupts off to prevent the page's data from changing during read
  memcpy(current_page_data, current_page, I2C_REGISTER_PAGE_SIZE);
  __enable_irq();
}

static void i2c_data_rcv(uint8_t position, uint8_t data) {
  if(position == I2C_REGISTER_OFFSET_PAGE) {
    change_page(data);
    return;
  }
  if(position >= I2C_REGISTER_PAGE_SIZE) { // 0-based index
    return;
  }

  if(current_page == &i2c_registers) {
    uint8_t *p = (uint8_t *)&i2c_registers;

    switch(position) {
      case I2C_REGISTER_OFFSET_HZ_HI:
      case I2C_REGISTER_OFFSET_HZ_LO:
        p[position] = data;
        break;
    }

    return;
  } 

  if(current_page == &i2c_registers_page3) {
    uint8_t *p = (uint8_t *)&i2c_registers_page3;

    if(position < 19) {
      p[position] = data;
    } else if(position == 19 && data) {
      write_flash_data();
    }
  }
}

static void i2c_data_xmt(I2C_HandleTypeDef *hi2c) {
  HAL_I2C_Slave_Sequential_Transmit_IT(hi2c, current_page_data, I2C_REGISTER_PAGE_SIZE, I2C_FIRST_FRAME);
}

void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *hi2c) {
  if(i2c_transfer_state == STATE_DROP_DATA) { // encountered a stop/error condition
    return;
  }

  i2c_data = 0;
  HAL_I2C_Slave_Sequential_Transmit_IT(hi2c, &i2c_data, 1, I2C_NEXT_FRAME);
}

void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c) {
  switch(i2c_transfer_state) {
    case STATE_GET_ADDR:
      i2c_transfer_position = i2c_data;
      i2c_transfer_state = STATE_GET_DATA;
      HAL_I2C_Slave_Sequential_Receive_IT(hi2c, &i2c_data, 1, I2C_LAST_FRAME);
      break;
    case STATE_GET_DATA:
      i2c_data_rcv(i2c_transfer_position, i2c_data);
      i2c_transfer_position++;
      HAL_I2C_Slave_Sequential_Receive_IT(hi2c, &i2c_data, 1, I2C_LAST_FRAME);
      break;
    default:
      HAL_I2C_Slave_Sequential_Receive_IT(hi2c, &i2c_data, 1, I2C_LAST_FRAME);
      break;
  }

  if(i2c_transfer_position > sizeof(struct i2c_registers_type)) {
    i2c_transfer_state = STATE_DROP_DATA;
  }
}

void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef *hi2c) {
  HAL_I2C_EnableListen_IT(hi2c);
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c) {
  i2c_transfer_state = STATE_DROP_DATA;

  if(hi2c->ErrorCode != HAL_I2C_ERROR_AF) {
    HAL_I2C_EnableListen_IT(hi2c);
  }
}

void HAL_I2C_AbortCpltCallback(I2C_HandleTypeDef *hi2c) {
  i2c_transfer_state = STATE_DROP_DATA;
}

void i2c_show_data() {
}
