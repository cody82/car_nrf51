#ifndef __BATTERY_H
#define __BATTERY_H

#include <stdint.h>

extern volatile int32_t BatteryVoltage;
extern volatile int32_t LowVoltage;

void InitBattery();
void BatteryTick();

#endif
