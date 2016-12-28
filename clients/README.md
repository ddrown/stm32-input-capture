# Clients to talk to the stm32 hardware

Tested clients: Raspberry Pi, Odroid C2

 * pi-pwm-setup.c - setup PWM output for the Raspberry Pi (50Hz on GPIO18 / Pin #12)
 * odroid-c2-setup - setup PWM output for the Odroid C2 (50Hz on GPIOX\_6 / Pin #33)
 * input-capture-i2c.c - poll the stm32 every second and write the average frequency over the past 128s to /run/tcxo
 * timespec.c - nanosecond timestamps handling
 * i2c.c - i2c bus code
 * ds3231.c - setup RTC DS3231 (optional)

Example chrony.conf line: `tempcomp /run/tcxo 1 0 0 1 0`

Currently setup to expect 50Hz on channel 1, 1Hz on channel 2, and 1Hz on channel 3.  Channel 1 and channel 3 are relative to channel 2, as long as there's a valid (within 10ppm) pulse on it.
