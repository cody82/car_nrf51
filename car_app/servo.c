#include "servo.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"

static const uint32_t Pin_Servo = 13;

void InitServo()
{
    nrf_gpio_cfg_output(Pin_Servo);
    nrf_gpio_pin_clear(Pin_Servo);
}

void SetServo(int16_t steering)
{
	steering /= 66;
	nrf_gpio_pin_set(Pin_Servo);
	nrf_delay_us(1500 - steering);
	nrf_gpio_pin_clear(Pin_Servo);
}
