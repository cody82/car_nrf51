#ifndef __ULTRASOUND_H
#define __ULTRASOUND_H

#include <stdint.h>
#include "nrf_drv_rtc.h"

extern int8_t UltraSoundFront;
extern int8_t UltraSoundBack;
extern uint32_t UltraSoundFrontTime;
extern uint32_t UltraSoundBackTime;
extern volatile int32_t UltraSoundFrontDist;
extern volatile int32_t UltraSoundBackDist;

void InitUltraSound(const nrf_drv_rtc_t *_rtc);
void UltraSoundTick();

#endif