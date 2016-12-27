#ifndef TIMER_H
#define TIMER_H

//extern RTC_HandleTypeDef hrtc;
extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim3;

#define DEFAULT_SOURCE_HZ 50

void print_timer_status();
void timer_start();

#endif
