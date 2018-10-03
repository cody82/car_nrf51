#include "main_esb.h"
#include <stdint.h>
#include "nrf.h"
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
#include "car.h"

#include "app_timer.h"
#include "app_util_platform.h"
#include "settings.h"

#define APP_TIMER_PRESCALER              15                                          /**< Value of the RTC1 PRESCALER register. */
#define APP_TIMER_OP_QUEUE_SIZE          4                                          /**< Size of timer operation queues. */
#define TIMER_INTERVAL         APP_TIMER_TICKS(20, APP_TIMER_PRESCALER)

APP_TIMER_DEF(m_app_timer_id);

static volatile uint32_t timer_tick = 0;

static void timer_timeout_handler(void * p_context)
{
    UNUSED_PARAMETER(p_context);
    timer_tick++;
}

static void timers_init(void)
{
    APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_OP_QUEUE_SIZE, false);

    uint32_t err_code;
    err_code = app_timer_create(&m_app_timer_id, APP_TIMER_MODE_REPEATED, timer_timeout_handler);
    APP_ERROR_CHECK(err_code);
}

static void application_timers_start(void)
{
    /* YOUR_JOB: Start your timers. below is an example of how to start a timer.*/
    uint32_t err_code;
    err_code = app_timer_start(m_app_timer_id, TIMER_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);
}
/*
const nrf_drv_rtc_t rtc =
{
 .p_reg = NRF_RTC1,
 .irq = RTC1_IRQn,
 .instance_id = 1
};
*/
static void lfclk_start_no_softdevice(void)
{
    nrf_clock_event_clear(NRF_CLOCK_EVENT_LFCLKSTARTED);
    nrf_clock_int_enable(NRF_CLOCK_INT_LF_STARTED_MASK);
    nrf_clock_task_trigger(NRF_CLOCK_TASK_LFCLKSTART);
}


static void lfclk_config(void)
{
	ret_code_t err_code = nrf_drv_clock_init();
	APP_ERROR_CHECK(err_code);

	lfclk_start_no_softdevice();
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
		int32_t t;
		if(packet.throttle > 0)
		{
			t = SettingsData.MaxForwardSpeed;
		}
		else
		{
			t = SettingsData.MaxBackwardSpeed;
		}
		if(t > 100 || t == 0)
			t = 100;
			
		t = t * packet.throttle / 100;

		SetMotor(t);
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

	//if ((tick % 3) == 0)
	{
    	__disable_irq();
		SetServo(RemoteFail() ? 0 : packet.steering);
		__enable_irq();
	}

	BlinkerTick();

	//nrf_delay_ms(10);
	tick++;
	Led2Toggle();
}

static volatile uint32_t loop_tick = 0;
void main_esb()
{
    SettingsInit();

	clocks_start();
	lfclk_config();
	timers_init();

//	rtc_config();

    CarInit();

	InitEsb();

	application_timers_start();

	while (1)
	{
		if(loop_tick != timer_tick)
		{
			loop_tick = timer_tick;
			loop();
		}
	}
}
