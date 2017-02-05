#ifndef ADC_CALC_H
#define ADC_CALC_H

void add_adc_data(const struct i2c_registers_type *i2c_registers, const struct i2c_registers_type_page2 *i2c_registers_page2);
float last_temp();
float last_vref();
void adc_print();
void adc_header();

#endif
