#include "ultrasound.h"
#include "nrf.h"
#include "nrf_gpio.h"
#include "app_error.h"
#include "nrf_drv_gpiote.h"
#include "nrf_drv_timer.h"

const nrf_drv_timer_t timer = NRF_DRV_TIMER_INSTANCE(2);

//const nrf_drv_timer_co_t timer = NRF_DRV_TIMER_DEFAULT_CONFIG(2);

const uint32_t Pin_FrontTrig = 16;
const uint32_t Pin_FrontEcho = 17;
const uint32_t Pin_BackTrig = 18;
const uint32_t Pin_BackEcho = 19;

uint32_t UltraSoundFrontTime = 0;
uint32_t UltraSoundBackTime = 0;
volatile int32_t UltraSoundFrontDist = 0;
volatile int32_t UltraSoundBackDist = 0;

static uint32_t tick = 0;

static void handler(nrf_timer_event_t event_type, void *p_context)
{
}

static uint16_t get_counter()
{
	NRF_TIMER2->TASKS_CAPTURE[0] = 1;
	return (uint16_t)NRF_TIMER2->CC[0];
}

int32_t UltraSoundCalcDistance(uint32_t now, uint32_t last)
{
	if(now < last)
		now += 0xFFFF;
	uint32_t delta = now - last;

	//delta = delta * 17150 / 32768;//cm
	delta = delta * 17150 / 125000;//cm
	if (delta < 150)
	{
		return (int32_t)delta;
	}
	else
	{
		return -1;
	}
}

void UltraSoundFrontHandler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
	if(nrf_gpio_pin_read(Pin_FrontEcho))
		UltraSoundFrontTime = get_counter();
	else
	{
		UltraSoundFrontDist = UltraSoundCalcDistance(get_counter(), UltraSoundFrontTime);
	}
}

void UltraSoundBackHandler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
	if(nrf_gpio_pin_read(Pin_BackEcho))
		UltraSoundBackTime = get_counter();
	else
	{
		UltraSoundBackDist = UltraSoundCalcDistance(get_counter(), UltraSoundBackTime);
	}
}


void InitUltraSound()
{
	ret_code_t err_code;

	nrf_gpio_cfg_output(Pin_FrontTrig);
	nrf_gpio_pin_clear(Pin_FrontTrig);
	nrf_gpio_cfg_input(Pin_FrontEcho, NRF_GPIO_PIN_NOPULL);
	nrf_gpio_cfg_output(Pin_BackTrig);
	nrf_gpio_pin_clear(Pin_BackTrig);
	nrf_gpio_cfg_input(Pin_BackEcho, NRF_GPIO_PIN_NOPULL);

	nrf_drv_gpiote_in_config_t in_config = GPIOTE_CONFIG_IN_SENSE_TOGGLE(true);
	//in_config.pull = NRF_GPIO_PIN_NOPULL;

	err_code = nrf_drv_gpiote_in_init(Pin_FrontEcho, &in_config, UltraSoundFrontHandler);
	APP_ERROR_CHECK(err_code);

	nrf_drv_gpiote_in_event_enable(Pin_FrontEcho, true);

	err_code = nrf_drv_gpiote_in_init(Pin_BackEcho, &in_config, UltraSoundBackHandler);
	APP_ERROR_CHECK(err_code);

	nrf_drv_gpiote_in_event_enable(Pin_BackEcho, true);

	nrf_drv_timer_init(&timer, NULL, handler);
	nrf_drv_timer_enable(&timer);
}

void UltraSoundTick()
{
	tick++;
	if ((tick % 11) == 0)
	{
		nrf_gpio_pin_toggle(Pin_FrontTrig);
	}
	else if ((tick % 11) == 5)
	{
		nrf_gpio_pin_toggle(Pin_BackTrig);
	}
}
