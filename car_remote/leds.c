#include "leds.h"

#include "nrf_gpio.h"

void LedsInit()
{
    nrf_gpio_cfg_output(LEDS_GREEN_PIN);
    nrf_gpio_pin_clear(LEDS_GREEN_PIN);
    nrf_gpio_cfg_output(LEDS_YELLOW_PIN);
    nrf_gpio_pin_set(LEDS_YELLOW_PIN);
    nrf_gpio_cfg_output(LEDS_RED_PIN);
    nrf_gpio_pin_set(LEDS_RED_PIN);
}

static uint32_t counter;
void LedsTick(uint8_t connected, uint8_t remote_bat_low, uint8_t car_bat_low)
{
    if(!connected)
    {
        if(counter % 5 == 0)
            nrf_gpio_pin_toggle(LEDS_GREEN_PIN);
    }
    else
    {
        nrf_gpio_pin_clear(LEDS_GREEN_PIN);
    }

    if(remote_bat_low)
    {
        if(counter % 5 == 0)
            nrf_gpio_pin_toggle(LEDS_YELLOW_PIN);
    }
    else
    {
        nrf_gpio_pin_set(LEDS_YELLOW_PIN);
    }

    if(car_bat_low)
    {
        if(counter % 5 == 0)
            nrf_gpio_pin_toggle(LEDS_RED_PIN);
    }
    else
    {
        nrf_gpio_pin_set(LEDS_RED_PIN);
    }

    counter++;
}
