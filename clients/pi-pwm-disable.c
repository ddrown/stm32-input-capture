#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main (void) {
  printf ("Raspberry Pi: disable PWM on GPIO18 (physical # 12)\n") ;

  if (wiringPiSetupGpio() == -1)
    exit (1) ;

  pinMode(18,INPUT);
}
