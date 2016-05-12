#ifndef __CAR_ESB_H
#define __CAR_ESB_H

#include <stdint.h>

#define ESB_ALTERNATIVE_RESOURCES

typedef struct
{
	uint16_t crc16;
	//uint8_t xxx;
	uint8_t type;
	int16_t steering;
	int16_t throttle;
	uint8_t front_light;
	uint8_t top_light;
	uint8_t blink_left;
	uint8_t blink_right;
	uint8_t beep;
} Packet;

extern volatile Packet packet;
extern volatile int32_t rc_timeout;

void InitEsb();


#endif
