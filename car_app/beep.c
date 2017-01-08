#include "beep.h"
#include "nrf_gpio.h"

static const uint32_t Pin_Beep = 14;

void BeepInit()
{
    nrf_gpio_cfg_output(Pin_Beep);
    nrf_gpio_pin_clear(Pin_Beep);
}

void BeepOn()
{
    nrf_gpio_pin_set(Pin_Beep);
}

void BeepOff()
{
    nrf_gpio_pin_clear(Pin_Beep);
}
