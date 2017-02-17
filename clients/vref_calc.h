#ifndef VREF_CALC_H
#define VREF_CALC_H

void add_vref_data(const struct i2c_registers_type_page2 *i2c_registers_page2);
float last_vref();

#endif
