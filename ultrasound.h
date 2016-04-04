#ifndef __ULTRASOUND_H
#define __ULTRASOUND_H

#include <stdint.h>

extern int32_t UltraSoundFrontDist();
extern int32_t UltraSoundBackDist();

void InitUltraSound();
void UltraSoundTick();

#endif
