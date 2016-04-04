#include "ultrasound.h"
#include "nrf.h"
#include "nrf_gpio.h"
#include "app_error.h"
#include "nrf_drv_gpiote.h"
#include "nrf_drv_timer.h"
#include "nrf_drv_ppi.h"

static const nrf_drv_timer_t timer = NRF_DRV_TIMER_INSTANCE(2);

static nrf_ppi_channel_t ppi_front_raise, ppi_front_fall;
static nrf_ppi_channel_t ppi_back_raise, ppi_back_fall;

static const uint32_t Pin_FrontTrig = 16;
static const uint32_t Pin_FrontEcho = 17;
static const uint32_t Pin_FrontEcho2 = 23;
static const uint32_t Pin_BackTrig = 18;
static const uint32_t Pin_BackEcho = 19;
static const uint32_t Pin_BackEcho2 = 20;

static uint32_t tick = 0;

//static nrf_drv_gpiote_in_config_t gpiote_front_raise = GPIOTE_CONFIG_IN_SENSE_LOTOHI(true);
static nrf_drv_gpiote_in_config_t gpiote_front_raise = GPIOTE_CONFIG_IN_SENSE_TOGGLE(true);
//static nrf_drv_gpiote_in_config_t gpiote_front_fall = GPIOTE_CONFIG_IN_SENSE_HITOLO(true);
static nrf_drv_gpiote_in_config_t gpiote_back_raise = GPIOTE_CONFIG_IN_SENSE_TOGGLE(true);
//static nrf_drv_gpiote_in_config_t gpiote_back_fall = GPIOTE_CONFIG_IN_SENSE_HITOLO(true);

static int32_t front_dist = -1;
static int32_t back_dist = -1;

static void handler(nrf_timer_event_t event_type, void *p_context)
{
}

int32_t UltraSoundCalcDistance(uint32_t now, uint32_t last)
{
	//if(now < last)
	//	now += 0xFFFF;
	uint32_t delta = now - last;

    if(delta == 0)
        return -1;
        
	//delta = delta * 17150 / 32768;//cm
	delta = delta * 17150 / 125000;//cm
	if (delta < 150 && delta > 0)
	{
		return (int32_t)delta;
	}
	else
	{
		return -1;
	}
}

int32_t UltraSoundFrontDist()
{
    return front_dist;
    //return UltraSoundCalcDistance(nrf_drv_timer_capture_get(&timer, 0), 0);
}

int32_t UltraSoundBackDist()
{
    return back_dist;
}

void InitUltraSound()
{
	ret_code_t err_code;

	nrf_drv_timer_init(&timer, NULL, handler);
    
	nrf_gpio_cfg_output(Pin_FrontTrig);
	nrf_gpio_pin_set(Pin_FrontTrig);
	nrf_gpio_cfg_input(Pin_FrontEcho, NRF_GPIO_PIN_NOPULL);
	nrf_gpio_cfg_input(Pin_FrontEcho2, NRF_GPIO_PIN_NOPULL);
	nrf_gpio_cfg_output(Pin_BackTrig);
	nrf_gpio_pin_set(Pin_BackTrig);
	nrf_gpio_cfg_input(Pin_BackEcho, NRF_GPIO_PIN_NOPULL);
	nrf_gpio_cfg_input(Pin_BackEcho2, NRF_GPIO_PIN_NOPULL);
    
    // back raise
    err_code = nrf_drv_gpiote_in_init((nrf_drv_gpiote_pin_t)Pin_BackEcho, &gpiote_back_raise, NULL);
    APP_ERROR_CHECK(err_code);
    //nrf_drv_gpiote_in_event_enable(Pin_BackEcho, true);
    
    // front raise
    err_code = nrf_drv_gpiote_in_init((nrf_drv_gpiote_pin_t)Pin_FrontEcho, &gpiote_front_raise, NULL);
    APP_ERROR_CHECK(err_code);
    //nrf_drv_gpiote_in_event_enable(Pin_FrontEcho, true);
    
    
    
    err_code = nrf_drv_ppi_channel_alloc(&ppi_front_fall);
    APP_ERROR_CHECK(err_code);
    err_code = nrf_drv_ppi_channel_assign(ppi_front_fall,
                                          nrf_drv_gpiote_in_event_addr_get(Pin_FrontEcho),
                                          nrf_drv_timer_task_address_get(&timer, NRF_TIMER_TASK_CAPTURE0));
    APP_ERROR_CHECK(err_code);
    err_code = nrf_drv_ppi_channel_enable(ppi_front_fall);
    APP_ERROR_CHECK(err_code);
    
    err_code = nrf_drv_ppi_channel_alloc(&ppi_front_raise);
    APP_ERROR_CHECK(err_code);
    err_code = nrf_drv_ppi_channel_assign(ppi_front_raise,
                                          nrf_drv_gpiote_in_event_addr_get(Pin_FrontEcho),
                                          nrf_drv_timer_task_address_get(&timer, NRF_TIMER_TASK_CLEAR));
    APP_ERROR_CHECK(err_code);
    err_code = nrf_drv_ppi_channel_enable(ppi_front_raise);
    APP_ERROR_CHECK(err_code);
    
    
    
    
    err_code = nrf_drv_ppi_channel_alloc(&ppi_back_fall);
    APP_ERROR_CHECK(err_code);
    err_code = nrf_drv_ppi_channel_assign(ppi_back_fall,
                                          nrf_drv_gpiote_in_event_addr_get(Pin_BackEcho),
                                          nrf_drv_timer_task_address_get(&timer, NRF_TIMER_TASK_CAPTURE1));
    APP_ERROR_CHECK(err_code);
    err_code = nrf_drv_ppi_channel_enable(ppi_back_fall);
    APP_ERROR_CHECK(err_code);
    
    err_code = nrf_drv_ppi_channel_alloc(&ppi_back_raise);
    APP_ERROR_CHECK(err_code);
    err_code = nrf_drv_ppi_channel_assign(ppi_back_raise,
                                          nrf_drv_gpiote_in_event_addr_get(Pin_BackEcho),
                                          nrf_drv_timer_task_address_get(&timer, NRF_TIMER_TASK_CLEAR));
    APP_ERROR_CHECK(err_code);
    err_code = nrf_drv_ppi_channel_enable(ppi_back_raise);
    APP_ERROR_CHECK(err_code);
   
    

	nrf_drv_timer_enable(&timer);
}

static bool front = 1;

void UltraSoundTick()
{
	tick++;
	if ((tick % 6) == 0)
	{
        if(front)
        {
            back_dist = UltraSoundCalcDistance(nrf_drv_timer_capture_get(&timer, 1), 0);
            nrf_drv_gpiote_in_event_enable(Pin_FrontEcho, true);
            nrf_drv_gpiote_in_event_enable(Pin_BackEcho, false);
            nrf_gpio_pin_clear(Pin_FrontTrig);
            nrf_gpio_pin_set(Pin_BackTrig);
        }
        else
        {
            front_dist = UltraSoundCalcDistance(nrf_drv_timer_capture_get(&timer, 0), 0);
            nrf_drv_gpiote_in_event_enable(Pin_FrontEcho, false);
            nrf_drv_gpiote_in_event_enable(Pin_BackEcho, true);
            nrf_gpio_pin_set(Pin_FrontTrig);
            nrf_gpio_pin_clear(Pin_BackTrig);
        }
        front = !front;
	}
}
