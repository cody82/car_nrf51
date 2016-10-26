#include "lights.h"
#include "ws2812b-nrf51.h"
#include <stddef.h>
#include <math.h>
#include "nrf.h"

static Color strip[21];
const uint32_t Pin_LED = 21;

static const uint8_t front_lights[] = { 1,2,4,5 };
static const uint8_t right_blinkers[] = { 6,20 };
static const uint8_t left_blinkers[] = { 0,14 };
static const uint8_t all_blinkers[] = { 0,6,14,20 };
static const uint8_t break_lights[] = { 15,16,18,19 };
static const uint8_t back_lights[] = { 17 };
static const uint8_t top_lights[] = { 7,8,9,10,11,12,13 };

#define SET_LIGHTS(LIGHTS_ARRAY,R,G,B) SetLights(LIGHTS_ARRAY, sizeof(LIGHTS_ARRAY), (R), (G), (B))
#define SET_LIGHTS_WHITE(LIGHTS_ARRAY,L) SetLights(LIGHTS_ARRAY, sizeof(LIGHTS_ARRAY), (L), (L), (L))
void SetLights(const uint8_t *lights, size_t count, uint8_t r, uint8_t g, uint8_t b)
{
	size_t i;
	for (i = 0; i < count; ++i)
	{
		Color *c = &strip[lights[i]];
		c->Red = r;
		c->Green = g;
		c->Blue = b;
	}
}

int32_t blink_left = 0;
int32_t blink_right = 0;

void LeftBlinkerOn()
{
	SET_LIGHTS(left_blinkers, 150, 150, 0);
}

void LeftBlinkerOff()
{
	SET_LIGHTS(left_blinkers, 0, 0, 0);
}
void RightBlinkerOn()
{
	SET_LIGHTS(right_blinkers, 150, 150, 0);
}

void RightBlinkerOff()
{
	SET_LIGHTS(right_blinkers, 0, 0, 0);
}

void SetFrontLights(uint8_t red, uint8_t green, uint8_t blue)
{
	SET_LIGHTS(front_lights, red, green, blue);
}

void SetBackLight(int8_t on)
{
	SET_LIGHTS_WHITE(back_lights, on ? 150 : 0);
}

void InitLights()
{
	ws2812b_init(Pin_LED);
}

int32_t break_throttle = 0;
void BreakLightTick(int16_t throttle)
{
	uint8_t power;
	
	if(break_throttle < throttle)
	{
		break_throttle += 200;
		if(break_throttle > throttle)
			break_throttle = throttle;
	}
	else if(break_throttle > throttle)
	{
		break_throttle -= 200;
		if(break_throttle < throttle)
			break_throttle = throttle;
	}

	if(break_throttle > throttle && break_throttle > 1000)
	{
		power = ((int32_t)break_throttle - (int32_t)throttle) * 200 / 65536;
		SET_LIGHTS(break_lights, 10 + power, 0, 0);
	}
	else if(break_throttle < throttle && break_throttle < -1000)
	{
		power = ((int32_t)throttle - (int32_t)break_throttle) * 200 / 65536;
		SET_LIGHTS(break_lights, 10 + power, 0, 0);
	}
	else
	{
		SET_LIGHTS(break_lights, 0, 0, 0);
	}
}

static float top_time = 0.0f;
static int8_t last_top = 0;
void TopLights(int8_t on)
{
	if(on && !last_top)
	{
		top_time = 0.0f;
	}
	last_top = on;
	top_time += 0.2f;
	for (int i = 7; i < 14; ++i)
	{
		Color *c = &strip[i];
		c->Red = c->Green = 0;
		if (on)
			c->Blue = (unsigned char)((sinf(top_time + ((float)(i - 7) * 2.0f)) * 0.5f + 0.5f) * 255.0f * (top_time < 3.0f ? (pow(top_time / 3.0f, 10)) : 1.0f));
		else
			c->Blue = 0;
	}
}

void LightTick()
{
	//__disable_irq();
	ws2812b_write(Pin_LED, (uint8_t*)strip, sizeof(strip));
	//__enable_irq();
}

static uint32_t tick;

void BlinkerTick()
{
	tick++;

	if (tick % 25 == 0)
	{
		if (blink_left > 0)
		{
			blink_left++;
			if (blink_left % 2 == 0)
				LeftBlinkerOn();
			else
				LeftBlinkerOff();
		}
		else
			LeftBlinkerOff();
		if (blink_right > 0)
		{
			blink_right++;
			if (blink_right % 2 == 0)
				RightBlinkerOn();
			else
				RightBlinkerOff();
		}
		else
			RightBlinkerOff();
	}
}

void BlinkLeft(int8_t on)
{
	if (on)
	{
		if (blink_left == 0)
		{
			if (blink_right > 0)
				blink_left = blink_right;
			else
				blink_left = 1;
		}
	}
	else
		blink_left = 0;
}

void BlinkRight(int8_t on)
{
	if (on)
	{
		if (blink_right == 0)
		{
			if (blink_left > 0)
				blink_right = blink_left;
			else
				blink_right = 1;
		}
	}
	else
		blink_right = 0;
}

void BlinkWarner()
{
	if (blink_left == 0)
	{
		if (blink_right > 0)
			blink_left = blink_right;
		else
			blink_left = 1;
	}

	if (blink_right == 0)
	{
		if (blink_left > 0)
			blink_right = blink_left;
		else
			blink_right = 1;
	}
}
