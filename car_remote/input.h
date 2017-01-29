#ifndef __INPUT_H
#define __INPUT_H

#include <stdint.h>

#define INPUT_PIN_BLINK_LEFT (9)
#define INPUT_PIN_BLINK_RIGHT (10)
#define INPUT_PIN_BLUE_LIGHT (11)
#define INPUT_PIN_FRONT_LIGHT (12)


void InputInit();
void InputTick();

int8_t InputGetSteering();
int8_t InputGetThrottle();

uint8_t InputGetBlinkLeft();
uint8_t InputGetBlinkRight();
uint8_t InputGetBlueLight();
uint8_t InputGetFrontLight();

#endif
