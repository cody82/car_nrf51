#include "nrf_dfu.h"
#include "nrf_dfu_transport.h"
#include "nrf_dfu_utils.h"
#include "nrf_bootloader_app_start.h"
#include "nrf_dfu_settings.h"
#include "nrf_gpio.h"
#include "app_scheduler.h"
#include "app_timer_appsh.h"
#include "nrf_log.h"
#include "boards.h"
#include "nrf_bootloader_info.h"
#include "nrf_dfu_req_handler.h"
#include "nrf_delay.h"

//#define SWITCH_PIN1 (2)
//#define SWITCH_PIN2 (30)

#ifdef BOOTLOADER_BUTTON
#undef BOOTLOADER_BUTTON
#endif
#define BOOTLOADER_BUTTON (2)

bool nrf_dfu_enter_check(void)
{
    nrf_gpio_cfg_input(BOOTLOADER_BUTTON, NRF_GPIO_PIN_PULLUP);

    nrf_delay_us(10);

    if (nrf_gpio_pin_read(BOOTLOADER_BUTTON) == 0)
    {
        return true;
    }

    if (s_dfu_settings.enter_buttonless_dfu == 1)
    {
        s_dfu_settings.enter_buttonless_dfu = 0;
        (void)nrf_dfu_settings_write(NULL);
        return true;
    }
    return false;
}

