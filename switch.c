#include "switch.h"

#include "nrf_gpio.h"
#include "nrf_delay.h"

//#define SWITCH_PIN1 (2)
#define SWITCH_PIN2 (30)

SwitchPosition Switch()
{
    SwitchPosition result;

	//nrf_gpio_cfg_output(SWITCH_PIN1);
	//nrf_gpio_pin_clear(SWITCH_PIN1);

    nrf_gpio_cfg_input(SWITCH_PIN2, NRF_GPIO_PIN_PULLUP);

    nrf_delay_us(10);

    if(nrf_gpio_pin_read(SWITCH_PIN2) == 0)
    {
        result = SwitchESB;
    }
    else
    {
        result = SwitchBLE;
    }

    //nrf_gpio_cfg_input(SWITCH_PIN1, NRF_GPIO_PIN_NOPULL);
    nrf_gpio_cfg_input(SWITCH_PIN2, NRF_GPIO_PIN_NOPULL);

    return result;
}
