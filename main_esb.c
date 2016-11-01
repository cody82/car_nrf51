#include "main_esb.h"
#include <stdint.h>
#include "nrf.h"
#include "nrf_drv_config.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_drv_gpiote.h"
//#include "nrf_drv_rtc.h"
#include "app_error.h"
#include "nrf_drv_clock.h"
#include "motors.h"
#include "lights.h"
#include "battery.h"
#include "car_esb.h"
#include "nrf_esb.h"
#include "servo.h"
#include "beep.h"
#include "led.h"

/*
const nrf_drv_rtc_t rtc =
{
 .p_reg = NRF_RTC1,
 .irq = RTC1_IRQn,
 .instance_id = 1
};
*/
static void lfclk_config(void)
{
	ret_code_t err_code = nrf_drv_clock_init();
	APP_ERROR_CHECK(err_code);

	nrf_drv_clock_lfclk_request(NULL);
}

void clocks_start( void )
{
    NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
    NRF_CLOCK->TASKS_HFCLKSTART = 1;

    while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);
}
/*
static void rtc_handler(nrf_drv_rtc_int_type_t int_type)
{
}

static void rtc_config(void)
{
	uint32_t err_code;

	err_code = nrf_drv_rtc_init(&rtc, NULL, rtc_handler);
	APP_ERROR_CHECK(err_code);

	nrf_drv_rtc_enable(&rtc);
}
*/

bool RemoteFail()
{
	return (rc_timeout > 100) || (LowVoltage > 100);
}

void CalcLights()
{
	if (RemoteFail())
	{
		TopLights(false);
		SetFrontLights(0, 0, 0);
		SetBackLight(0);
	}
	else
	{
		TopLights(packet.top_light);
		SetFrontLights(packet.front_light, packet.front_light, packet.front_light);
		SetBackLight(packet.throttle < -1000);
	}
}

static uint32_t tick = 0;

void loop()
{
	BatteryTick();

	rc_timeout++;

	CalcLights();
	
	BreakLightTick(packet.throttle);

	//nrf_esb_disable();
    __disable_irq();
	LightTick();
	__enable_irq();
	//nrf_esb_enable();

	if (!RemoteFail())
	{
		SetMotor(packet.throttle);
	}
	else
	{
		SetMotor(0);
	}

	if (RemoteFail())
	{
		BlinkWarner();
	}
	else
	{
		BlinkRight(packet.blink_right);
		BlinkLeft(packet.blink_left);
	}

	if (!RemoteFail() && packet.beep)
	{
		BeepOn();
	}
	else
	{
		BeepOff();
	}

	if ((tick % 3) == 0)
	{
		SetServo(RemoteFail() ? 0 : packet.steering);
	}

	BlinkerTick();

	nrf_delay_ms(10);
	tick++;
	Led2Toggle();
}

void main_esb()
{
	ret_code_t err_code;

	clocks_start();
	lfclk_config();

//	rtc_config();

	err_code = nrf_drv_gpiote_init();
	APP_ERROR_CHECK(err_code);

	InitBattery();

	LedInit();

	InitLights();

	BeepInit();

	InitMotor();

	InitServo();

	InitEsb();

	while (1)
	{
		loop();
	}
}
