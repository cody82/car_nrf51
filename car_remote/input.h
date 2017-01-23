#ifndef __INPUT_H
#define __INPUT_H

#include <stdint.h>

//extern volatile int32_t BatteryVoltage;

void InputInit();
void InputTick();

int8_t InputGetSteering();
int8_t InputGetThrottle();

#endif
