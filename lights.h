#ifndef __LIGHTS_H
#define __LIGHTS_H

#include <stdint.h>

typedef struct
{
	uint8_t Green;
	uint8_t Red;
	uint8_t Blue;
} Color;

void InitLights();
void LeftBlinkerOn();
void LeftBlinkerOff();
void RightBlinkerOn();
void RightBlinkerOff();
void TopLights(int8_t on);
void SetFrontLights(uint8_t red, uint8_t green, uint8_t blue);
void SetBackLight(int8_t on);
void LightTick();
void BlinkerTick();
void BlinkLeft(int8_t on);
void BlinkRight(int8_t on);
void BlinkWarner();

#endif
