#include "led.h"
#include "nrf_gpio.h"

static const uint32_t Pin_LED1 = 6;
static const uint32_t Pin_LED2 = 10;

void LedInit()
{
    nrf_gpio_cfg_output(Pin_LED1);
    nrf_gpio_cfg_output(Pin_LED2);
    nrf_gpio_pin_set(Pin_LED1);
    nrf_gpio_pin_set(Pin_LED2);
}

void Led1Toggle()
{
    nrf_gpio_pin_toggle(Pin_LED1);
}

void Led2Toggle()
{
    nrf_gpio_pin_toggle(Pin_LED2);
}
