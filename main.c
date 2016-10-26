#include <stdint.h>
#include "nrf.h"
#include "nrf_drv_config.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_drv_gpiote.h"
#include "nrf_drv_rtc.h"
#include "app_error.h"
#include "nrf_drv_clock.h"
#include "motors.h"
#include "ultrasound.h"
#include "lights.h"
#include "battery.h"
#include "car_esb.h"
#include "nrf_esb.h"

const nrf_drv_rtc_t rtc = //NRF_DRV_RTC_INSTANCE(0); /**< Declaring an instance of nrf_drv_rtc for RTC0. */
{
 .p_reg = NRF_RTC1,
 .irq = RTC1_IRQn,
 .instance_id = 1
};

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

const uint32_t Pin_Servo = 13;
const uint32_t Pin_Beep = 14;
const uint32_t Pin_LED1 = 6;
const uint32_t Pin_LED2 = 10;

void SetServo(int16_t steering)
{
	steering /= 66;
    __disable_irq();
	nrf_gpio_pin_set(Pin_Servo);
	nrf_delay_us(1500 - steering);
	nrf_gpio_pin_clear(Pin_Servo);
	__enable_irq();
}

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

	bool blocked_front = (UltraSoundFrontDist() >= 0) && (UltraSoundFrontDist() < 20);
	bool blocked_back = (UltraSoundBackDist() >= 0) && (UltraSoundBackDist() < 20);

	CalcLights();
	
	BreakLightTick(packet.throttle);

	//nrf_esb_disable();
    __disable_irq();
	LightTick();
	__enable_irq();
	//nrf_esb_enable();

	if (!RemoteFail() && ((!blocked_front && packet.throttle >= 0) || (!blocked_back && packet.throttle <= 0)))
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
		nrf_gpio_pin_set(Pin_Beep);
	}
	else
	{
		nrf_gpio_pin_clear(Pin_Beep);
	}

	UltraSoundTick();

	if ((tick % 3) == 0)
	{
		SetServo(RemoteFail() ? 0 : packet.steering);
	}

	BlinkerTick();

	nrf_delay_ms(10);
	tick++;
	nrf_gpio_pin_toggle(Pin_LED2);
}

int main()
{
	ret_code_t err_code;

	clocks_start();
	lfclk_config();

	rtc_config();

	err_code = nrf_drv_gpiote_init();
	APP_ERROR_CHECK(err_code);

	InitBattery();

	//init LEDs
	nrf_gpio_cfg_output(Pin_LED1);
	nrf_gpio_cfg_output(Pin_LED2);
	nrf_gpio_pin_set(Pin_LED1);
	nrf_gpio_pin_set(Pin_LED2);

	//init lights
	InitLights();

	// init beeper
	nrf_gpio_cfg_output(Pin_Beep);
	nrf_gpio_pin_clear(Pin_Beep);

	InitMotor();

	// init servo
	nrf_gpio_cfg_output(Pin_Servo);
	nrf_gpio_pin_clear(Pin_Servo);

	InitUltraSound(NRF_RTC1);

	InitEsb();

	while (1)
	{
		loop();
	}
}
