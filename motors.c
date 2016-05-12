#include "motors.h"
#include "nrf.h"
#include "app_pwm.h"
#include "nrf_gpio.h"
#include "app_error.h"
#include <math.h>

const uint32_t Pin_AIN1 = 9;
const uint32_t Pin_AIN2 = 5;
const uint32_t Pin_PWMA = 7;
const uint32_t Pin_BIN1 = 11;
const uint32_t Pin_BIN2 = 12;
const uint32_t Pin_PWMB = 8;
const uint32_t Pin_Standby = 15;

APP_PWM_INSTANCE(PWM1, 0);
//APP_PWM_INSTANCE(PWM2, 0);

static volatile bool ready_flag;
static void pwm_ready_callback(uint32_t pwm_id)
{
	ready_flag = true;
}

void SetMotor(int16_t throttle)
{
	throttle /= 328;
	uint8_t dir = throttle >= 0;
	if (throttle < 0)
		throttle = -throttle;


	app_pwm_channel_duty_set(&PWM1, 0, 100 - throttle);
	app_pwm_channel_duty_set(&PWM1, 1, 100 - throttle);
	//while (app_pwm_channel_duty_set(&PWM1, 0, 100 - throttle) == NRF_ERROR_BUSY);
	//while (app_pwm_channel_duty_set(&PWM1, 1, 100 - throttle) == NRF_ERROR_BUSY);

	if (throttle == 0)
	{
		nrf_gpio_pin_clear(Pin_Standby);
		nrf_gpio_pin_clear(Pin_AIN1);
		nrf_gpio_pin_clear(Pin_AIN2);
		nrf_gpio_pin_clear(Pin_BIN1);
		nrf_gpio_pin_clear(Pin_BIN2);
	}
	else if (dir)
	{
		nrf_gpio_pin_clear(Pin_AIN1);
		nrf_gpio_pin_set(Pin_AIN2);
		nrf_gpio_pin_clear(Pin_BIN1);
		nrf_gpio_pin_set(Pin_BIN2);
		nrf_gpio_pin_set(Pin_Standby);
	}
	else
	{
		nrf_gpio_pin_clear(Pin_AIN2);
		nrf_gpio_pin_set(Pin_AIN1);
		nrf_gpio_pin_clear(Pin_BIN2);
		nrf_gpio_pin_set(Pin_BIN1);
		nrf_gpio_pin_set(Pin_Standby);
	}
}

void InitMotor()
{
	ret_code_t err_code;
	// init motor controller
	nrf_gpio_cfg_output(Pin_Standby);
	nrf_gpio_cfg_output(Pin_AIN1);
	nrf_gpio_cfg_output(Pin_AIN2);
	nrf_gpio_cfg_output(Pin_BIN1);
	nrf_gpio_cfg_output(Pin_BIN2);
	nrf_gpio_pin_clear(Pin_Standby);
	nrf_gpio_pin_clear(Pin_AIN1);
	nrf_gpio_pin_clear(Pin_AIN2);
	nrf_gpio_pin_clear(Pin_BIN1);
	nrf_gpio_pin_clear(Pin_BIN2);
	app_pwm_config_t pwm1_cfg = APP_PWM_DEFAULT_CONFIG_2CH(100L, Pin_PWMA, Pin_PWMB);
	err_code = app_pwm_init(&PWM1, &pwm1_cfg, pwm_ready_callback);
	APP_ERROR_CHECK(err_code);
	app_pwm_enable(&PWM1);
	app_pwm_channel_duty_set(&PWM1, 0, 0);
	app_pwm_channel_duty_set(&PWM1, 1, 0);
}
