# STM32 three input channel timer

Hardware is a stm32f030f4p6 with 12MHz TCXO, communication is via i2c.

Depends on make and arm-none-eabi-gcc being in your path.

arm-none-eabi-gcc should come with newlib (or other embedded c library)

"make" - builds the binary, output is in build/input-capture-i2c.elf + build/input-capture-i2c.bin

"make flash" - build the binary and flash it with a ST-Link (depends on ST-LINK\_CLI being in your path)

Use STM32CubeMX to view the pinout

 * Src/i2c\_slave.c - i2c slave
 * Src/timer.c - hardware timers measuring input capture (tim3 - runs at 48MHz, tim1 - uses tim3 as prescaler, combined they're effectively a 32bit counter) tim3 channels 1, 2, and 4 are used as input capture
 * Src/uart.c - uart print and receive
 * Src/main.c - setup and main loop
 * Src/stm32f0xx\_hal\_msp.c - auto-generated GPIO mapping code
 * Src/stm32f0xx\_it.c - auto-generated interrupt handlers

Clocks are setup for 12MHz HSE (bypass not crystal) and 48MHz PLL

Example i2c client program (for running on a Raspberry Pi or other Linux SBC) is in clients/
