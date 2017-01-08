#include "battery.h"
#include "nrf_drv_adc.h"
#include "app_error.h"


volatile int32_t BatteryVoltage = 0;//mV
static volatile int32_t adc;
static volatile int32_t adc2;
volatile int32_t LowVoltage = 0;

/*void ADC_IRQHandler(void)
{
	nrf_adc_conversion_event_clean();

	adc = nrf_adc_result_get();
	adc2 = (adc * 3600 / 1023);
	BatteryVoltage = adc2 * (1800 + 6800) / 1800;

	if (BatteryVoltage < 6000)
		LowVoltage++;
	else
		LowVoltage = 0;
}

static void adc_config(void)
{
	const nrf_adc_config_t nrf_adc_config = NRF_ADC_CONFIG_DEFAULT;

	nrf_adc_configure((nrf_adc_config_t *)&nrf_adc_config);
	nrf_adc_input_select(NRF_ADC_CONFIG_INPUT_4);
	nrf_adc_int_enable(ADC_INTENSET_END_Enabled << ADC_INTENSET_END_Pos);
	NVIC_SetPriority(ADC_IRQn, NRF_APP_PRIORITY_HIGH);
	NVIC_EnableIRQ(ADC_IRQn);
}*/

//#define ADC_BUFFER_SIZE 1                                /**< Size of buffer for ADC samples.  */
//static nrf_adc_value_t       adc_buffer[ADC_BUFFER_SIZE]; /**< ADC buffer. */
static nrf_drv_adc_channel_t m_channel_config = NRF_DRV_ADC_DEFAULT_CHANNEL(NRF_ADC_CONFIG_INPUT_4); /**< Channel instance. Default configuration used. */

static void adc_event_handler(nrf_drv_adc_evt_t const * p_event)
{
    if (p_event->type == NRF_DRV_ADC_EVT_SAMPLE)
    {
		adc = p_event->data.sample.sample;
		adc2 = (adc * 3600 / 1023);
		BatteryVoltage = adc2 * (1800 + 6800) / 1800;

		if (BatteryVoltage < 6000)
			LowVoltage++;
		else
			LowVoltage = 0;
    }
}

static void adc_config(void)
{
    ret_code_t ret_code;
    nrf_drv_adc_config_t config = NRF_DRV_ADC_DEFAULT_CONFIG;

    ret_code = nrf_drv_adc_init(&config, adc_event_handler);
    APP_ERROR_CHECK(ret_code);

    nrf_drv_adc_channel_enable(&m_channel_config);
}

void InitBattery()
{
	adc_config();
}

void BatteryTick()
{
	nrf_drv_adc_sample();
}
