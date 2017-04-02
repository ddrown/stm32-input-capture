CFLAGS=-Wall -std=gnu11
CC=gcc

all: input-capture-i2c timestamps-i2c timestamps-gpio set-calibration-data pi-pwm-setup ds3231 pcf2129

input-capture-i2c: input-capture-i2c.o i2c.o timespec.o i2c_registers.o adc_calc.o vref_calc.o avg.o
	$(CC) $(CFLAGS) -o $@ $^ -lm

timestamps-i2c: timestamps-i2c.o i2c.o i2c_registers.o timespec.o
	$(CC) $(CFLAGS) -o $@ $^

timestamps-gpio: timestamps-gpio.o i2c.o i2c_registers.o timespec.o
	$(CC) $(CFLAGS) -o $@ $^ -lwiringPi

set-calibration-data: set-calibration-data.o i2c.o i2c_registers.o float.o
	$(CC) $(CFLAGS) -o $@ $^

pi-pwm-setup: pi-pwm-setup.o
	$(CC) $(CFLAGS) -o $@ $^ -lwiringPi

ds3231: ds3231.o i2c.o
	$(CC) $(CFLAGS) -o $@ $^

pcf2129: pcf2129.o i2c.o
	$(CC) $(CFLAGS) -o $@ $^ -lm

.PHONY: all
