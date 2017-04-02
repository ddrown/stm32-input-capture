CFLAGS=-Wall -std=gnu11 -I../
CC=gcc

bme280: bme280.o ../i2c.o
	$(CC) $(CFLAGS) -o $@ $^
