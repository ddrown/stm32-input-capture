#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main (void) {
  printf ("Raspberry Pi: output 50Hz on pin GPIO18 (physical # 12)\n") ;

  if (wiringPiSetupGpio() == -1)
    exit (1) ;

  pinMode(18,PWM_OUTPUT);
  pwmSetMode(PWM_MODE_MS);
  pwmSetClock(1920);
  pwmSetRange(200);
  pwmWrite(18, 10);
}
