#include "ultrasound.h"
#include "nrf.h"
#include "nrf_gpio.h"
#include "app_error.h"
#include "nrf_drv_gpiote.h"

const uint32_t Pin_FrontTrig = 16;
const uint32_t Pin_FrontEcho = 17;
const uint32_t Pin_BackTrig = 18;
const uint32_t Pin_BackEcho = 19;

uint32_t UltraSoundFrontTime = 0;
uint32_t UltraSoundBackTime = 0;
volatile int32_t UltraSoundFrontDist = 0;
volatile int32_t UltraSoundBackDist = 0;

static uint32_t tick = 0;

int32_t UltraSoundCalcDistance(uint32_t now, uint32_t last)
{
	//uint32_t delta = now - last - (450 * 32768 / 1000000);
	if(now < last)
		now += 0xFFFFFF;
	uint32_t delta = now - last;
	/*if(delta > 14)
		delta-=14;
	else
		delta = 0;
*/
	delta = delta * 17150 / 32768;//cm
	if (delta < 150)
	{
		return (int32_t)delta;
	}
	else
	{
		return -1;
	}
}

static NRF_RTC_Type *rtc;

void UltraSoundFrontHandler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
	if(nrf_gpio_pin_read(Pin_FrontEcho))
		UltraSoundFrontTime = nrf_rtc_counter_get(rtc);
	else
	{
		UltraSoundFrontDist = UltraSoundCalcDistance(nrf_rtc_counter_get(rtc), UltraSoundFrontTime);
	}
}

void UltraSoundBackHandler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
	if(nrf_gpio_pin_read(Pin_BackEcho))
		UltraSoundBackTime = nrf_rtc_counter_get(rtc);
	else
	{
		UltraSoundBackDist = UltraSoundCalcDistance(nrf_rtc_counter_get(rtc), UltraSoundBackTime);
	}
}


void InitUltraSound(const NRF_RTC_Type *p_rtc)
{
	ret_code_t err_code;
	rtc = (NRF_RTC_Type*)p_rtc;

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
