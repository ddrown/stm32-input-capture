input-capture-i2c: input-capture-i2c.c i2c.c timespec.c i2c_registers.c adc_calc.c vref_calc.c avg.c
	gcc -Wall -std=gnu11 -o input-capture-i2c input-capture-i2c.c i2c.c timespec.c i2c_registers.c adc_calc.c vref_calc.c avg.c

pi-pwm-setup: pi-pwm-setup.c
	gcc -Wall -std=gnu11 -o pi-pwm-setup pi-pwm-setup.c -lwiringPi

ds3231: ds3231.c i2c.c
	gcc -Wall -std=gnu11 -o ds3231 ds3231.c i2c.c

pcf2129: pcf2129.c i2c.c
	gcc -Wall -std=gnu11 -o pcf2129 pcf2129.c i2c.c
