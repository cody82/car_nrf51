#include "input.h"
#include "nrf_adc.h"
#include "app_error.h"


volatile int32_t BatteryVoltage = 0;//mV
static volatile int32_t adc;
static volatile int32_t adc2;

void ADC_IRQHandler(void)
{
	nrf_adc_conversion_event_clean();

	adc = nrf_adc_result_get();
	adc2 = (adc * 3600 / 1023);
	BatteryVoltage = adc2;
}

#ifndef NRF_APP_PRIORITY_HIGH
#define NRF_APP_PRIORITY_HIGH 1
#endif
static void adc_config(void)
{
	const nrf_adc_config_t nrf_adc_config = NRF_ADC_CONFIG_DEFAULT;

	nrf_adc_configure((nrf_adc_config_t *)&nrf_adc_config);
	nrf_adc_input_select(NRF_ADC_CONFIG_INPUT_2);
	nrf_adc_int_enable(ADC_INTENSET_END_Enabled << ADC_INTENSET_END_Pos);
	NVIC_SetPriority(ADC_IRQn, NRF_APP_PRIORITY_HIGH);
	NVIC_EnableIRQ(ADC_IRQn);
}

void InputInit()
{
	adc_config();
}

void InputTick()
{
	nrf_adc_start();
}
