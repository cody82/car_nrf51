#include "car.h"
#include "nrf_drv_gpiote.h"
#include "nrf_drv_ppi.h"
#include "app_error.h"
#include "led.h"
#include "battery.h"
#include "lights.h"
#include "beep.h"
#include "motors.h"
#include "servo.h"


void CarInit()
{
	ret_code_t err_code;

	err_code = nrf_drv_gpiote_init();
	APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_ppi_init();
	APP_ERROR_CHECK(err_code);

	InitBattery();

	LedInit();

	InitLights();

	BeepInit();

	InitMotor();

	InitServo();
}
