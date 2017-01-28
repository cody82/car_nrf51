#ifndef __LEDS_H
#define __LEDS_H

#include <stdint.h>

#define LEDS_GREEN_PIN (21)
#define LEDS_YELLOW_PIN (23)
#define LEDS_RED_PIN (25)

void LedsInit();
void LedsTick(uint8_t connected, uint8_t remote_bat_low, uint8_t car_bat_low);

#endif
