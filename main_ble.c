#include <stdint.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf.h"
#include "app_error.h"
#include "ble.h"
#include "ble_hci.h"
#include "ble_srv_common.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "ble_radio_notification.h"
#include "softdevice_handler.h"
#include "app_timer.h"
#include "app_trace.h"
#include "nrf_drv_clock.h"
#include "nrf_drv_gpiote.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_rtc.h"
#include "nrf_drv_ppi.h"

#include "ble_car.h"

#include "motors.h"
#include "lights.h"
#include "battery.h"
#include "ultrasound.h"

#define IS_SRVC_CHANGED_CHARACT_PRESENT  1                                          /**< Include or not the service_changed characteristic. if not enabled, the server's database cannot be changed for the lifetime of the device*/

#define DEVICE_NAME                      "Car"                               /**< Name of device. Will be included in the advertising data. */
#define MANUFACTURER_NAME                "Cody"                      /**< Manufacturer. Will be passed to Device Information Service. */
#define APP_ADV_INTERVAL                 300                                        /**< The advertising interval (in units of 0.625 ms. This value corresponds to 25 ms). */
#define APP_ADV_TIMEOUT_IN_SECONDS       180                                        /**< The advertising timeout in units of seconds. */

#define APP_TIMER_PRESCALER              0                                          /**< Value of the RTC1 PRESCALER register. */
#define APP_TIMER_OP_QUEUE_SIZE          4                                          /**< Size of timer operation queues. */

#define MIN_CONN_INTERVAL                MSEC_TO_UNITS(20, UNIT_1_25_MS)           /**< Minimum acceptable connection interval (0.1 seconds). */
#define MAX_CONN_INTERVAL                MSEC_TO_UNITS(20, UNIT_1_25_MS)           /**< Maximum acceptable connection interval (0.2 second). */
#define SLAVE_LATENCY                    0                                          /**< Slave latency. */
#define CONN_SUP_TIMEOUT                 MSEC_TO_UNITS(2000, UNIT_10_MS)            /**< Connection supervisory timeout (4 seconds). */

#define FIRST_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(5000, APP_TIMER_PRESCALER) /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY    APP_TIMER_TICKS(30000, APP_TIMER_PRESCALER)/**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT     3                                          /**< Number of attempts before giving up the connection parameter negotiation. */

#define TIMER_INTERVAL         APP_TIMER_TICKS(20, APP_TIMER_PRESCALER) /**< Battery level measurement interval (ticks). This value corresponds to 120 seconds. */

#define SEC_PARAM_BOND                   1                                          /**< Perform bonding. */
#define SEC_PARAM_MITM                   0                                          /**< Man In The Middle protection not required. */
#define SEC_PARAM_IO_CAPABILITIES        BLE_GAP_IO_CAPS_NONE                       /**< No I/O capabilities. */
#define SEC_PARAM_OOB                    0                                          /**< Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE           7                                          /**< Minimum encryption key size. */
#define SEC_PARAM_MAX_KEY_SIZE           16                                         /**< Maximum encryption key size. */

#define DEAD_BEEF                        0xDEADBEEF                                 /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

static uint16_t                          m_conn_handle = BLE_CONN_HANDLE_INVALID;   /**< Handle of the current connection. */

static ble_car_t                        ble_car;                                      /**< Structure to identify the Nordic UART Service. */

APP_TIMER_DEF(m_app_timer_id);

const uint32_t Pin_Servo = 13;
const uint32_t Pin_Beep = 14;
const uint32_t Pin_LED1 = 6;
const uint32_t Pin_LED2 = 10;

void ble_start();
void ble_stop();

typedef enum
{
	BleWait,
	EsbWait,
	Ble,
	Esb
} CarRemoteMode;

CarRemoteMode CurrentMode;

void SetServo(int16_t steering)
{
	steering /= 66;
	nrf_gpio_pin_set(Pin_Servo);
	nrf_delay_us(1500 - steering);
	nrf_gpio_pin_clear(Pin_Servo);
}

static volatile uint32_t rc_timeout = 0;
bool RemoteFail()
{
  return (m_conn_handle == BLE_CONN_HANDLE_INVALID) || (LowVoltage > 100) || (rc_timeout > 100);
}

void CalcLights()
{
	if (RemoteFail())
	{
		TopLights(ble_car.packet.top_light);
		SetFrontLights(0, 0, 0);
		SetBackLight(0);
	}
	else
	{
		TopLights(ble_car.packet.top_light);
		SetFrontLights(ble_car.packet.front_light, ble_car.packet.front_light, ble_car.packet.front_light);
		SetBackLight(ble_car.packet.throttle < 0);
	}
}

static uint32_t tick = 0;
static uint32_t mode_tick = 0;

bool blocked_front()
{
	return (UltraSoundFrontDist() >= 0) && (UltraSoundFrontDist() < 15);
}

bool blocked_back()
{
	return (UltraSoundBackDist() >= 0) && (UltraSoundBackDist() < 15);
}

void loop()
{
	mode_tick++;

	BatteryTick();
	ble_car_set_telemetry(&ble_car, BatteryVoltage, UltraSoundFrontDist(), UltraSoundBackDist());

	rc_timeout++;

	if(rc_timeout > 100 && m_conn_handle != BLE_CONN_HANDLE_INVALID)
	{
		sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
	}

	CalcLights();

	if (RemoteFail())
	{
		BlinkWarner();
		SetMotor(0);
	}
	else
	{
		BlinkRight(ble_car.packet.blink_right);
		BlinkLeft(ble_car.packet.blink_left);

		if ((!blocked_front() && ble_car.packet.throttle >= 0) || (!blocked_back() && ble_car.packet.throttle <= 0))
		{
			SetMotor(ble_car.packet.throttle * 256);
		}
		else
		{
			SetMotor(0);
		}
	}

	if (!RemoteFail() && ble_car.packet.beep)
	{
		nrf_gpio_pin_set(Pin_Beep);
	}
	else
	{
		nrf_gpio_pin_clear(Pin_Beep);
	}

	BlinkerTick();

	tick++;
	nrf_gpio_pin_toggle(Pin_LED2);
}

static ble_uuid_t m_adv_uuids[] = {{BLE_UUID_DEVICE_INFORMATION_SERVICE, BLE_UUID_TYPE_BLE}}; /**< Universally unique service identifiers. */


/**@brief Callback function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num   Line number of the failing ASSERT call.
 * @param[in] file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}

static void timer_timeout_handler(void * p_context)
{
    UNUSED_PARAMETER(p_context);
    loop();
}

/**@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module. This creates and starts application timers.
 */
static void timers_init(void)
{

    // Initialize timer module.
    APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_OP_QUEUE_SIZE, false);

    // Create timers.

    /* YOUR_JOB: Create any timers to be used by the application.
                 Below is an example of how to create a timer.
                 For every new timer needed, increase the value of the macro APP_TIMER_MAX_TIMERS by
                 one.*/
    uint32_t err_code;
    err_code = app_timer_create(&m_app_timer_id, APP_TIMER_MODE_REPEATED, timer_timeout_handler);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for the GAP initialization.
 *
 * @details This function sets up all the necessary GAP (Generic Access Profile) parameters of the
 *          device including the device name, appearance, and the preferred connection parameters.
 */
static void gap_params_init(void)
{
    uint32_t                err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *)DEVICE_NAME,
                                          strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

    /* YOUR_JOB: Use an appearance value matching the application's use case.
    err_code = sd_ble_gap_appearance_set(BLE_APPEARANCE_);
    APP_ERROR_CHECK(err_code); */

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}

void car_data_handler(ble_car_t * p_car, uint8_t * p_data, uint16_t length)
{
	rc_timeout = 0;
}

/**@brief Function for initializing services that will be used by the application.
 */
static void services_init(void)
{
    uint32_t         err_code;
    ble_car_init_t   car_init;

    memset(&car_init, 0, sizeof(car_init));

    car_init.data_handler = car_data_handler;

    err_code = ble_car_init(&ble_car, &car_init);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling the Connection Parameters Module.
 *
 * @details This function will be called for all events in the Connection Parameters Module which
 *          are passed to the application.
 *          @note All this function does is to disconnect. This could have been done by simply
 *                setting the disconnect_on_fail config parameter, but instead we use the event
 *                handler mechanism to demonstrate its use.
 *
 * @param[in] p_evt  Event received from the Connection Parameters Module.
 */
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
    uint32_t err_code;

    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {
        err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(err_code);
    }
}


/**@brief Function for handling a Connection Parameters error.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for initializing the Connection Parameters module.
 */
static void conn_params_init(void)
{
    uint32_t               err_code;
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail             = false;
    cp_init.evt_handler                    = on_conn_params_evt;
    cp_init.error_handler                  = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for starting timers.
*/
static void application_timers_start(void)
{
    /* YOUR_JOB: Start your timers. below is an example of how to start a timer.*/
    uint32_t err_code;
    err_code = app_timer_start(m_app_timer_id, TIMER_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);

}


/**@brief Function for handling advertising events.
 *
 * @details This function will be called for advertising events which are passed to the application.
 *
 * @param[in] ble_adv_evt  Advertising event.
 */
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    //uint32_t err_code;

    switch (ble_adv_evt)
    {
        case BLE_ADV_EVT_FAST:
            //err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);
            //APP_ERROR_CHECK(err_code);
            break;
        case BLE_ADV_EVT_IDLE:
            //sleep_mode_enter();
            break;
        default:
            break;
    }
}


/**@brief Function for handling the Application's BLE Stack events.
 *
 * @param[in] p_ble_evt  Bluetooth stack event.
 */
static void on_ble_evt(ble_evt_t * p_ble_evt)
{
    //uint32_t err_code;

    switch (p_ble_evt->header.evt_id)
            {
        case BLE_GAP_EVT_CONNECTED:
            //err_code = bsp_indication_set(BSP_INDICATE_CONNECTED);
            //APP_ERROR_CHECK(err_code);
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            m_conn_handle = BLE_CONN_HANDLE_INVALID;
            break;

        default:
            // No implementation needed.
            break;
    }
}


/**@brief Function for dispatching a BLE stack event to all modules with a BLE stack event handler.
 *
 * @details This function is called from the BLE Stack event interrupt handler after a BLE stack
 *          event has been received.
 *
 * @param[in] p_ble_evt  Bluetooth stack event.
 */
static void ble_evt_dispatch(ble_evt_t * p_ble_evt)
{
    ble_conn_params_on_ble_evt(p_ble_evt);
    on_ble_evt(p_ble_evt);
    ble_advertising_on_ble_evt(p_ble_evt);
		ble_car_on_ble_evt(&ble_car,p_ble_evt);
}


/**@brief Function for dispatching a system event to interested modules.
 *
 * @details This function is called from the System event interrupt handler after a system
 *          event has been received.
 *
 * @param[in] sys_evt  System stack event.
 */
static void sys_evt_dispatch(uint32_t sys_evt)
{
    ble_advertising_on_sys_evt(sys_evt);
}


/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
    uint32_t err_code;

    // Initialize the SoftDevice handler module.
    SOFTDEVICE_HANDLER_INIT(NRF_CLOCK_LFCLKSRC_XTAL_20_PPM, NULL);

#if defined(S110) || defined(S130) || defined(S132)
    // Enable BLE stack.
    ble_enable_params_t ble_enable_params;
    memset(&ble_enable_params, 0, sizeof(ble_enable_params));
#if (defined(S130) || defined(S132))
    ble_enable_params.gatts_enable_params.attr_tab_size   = BLE_GATTS_ATTR_TAB_SIZE_DEFAULT;
#endif
    ble_enable_params.gatts_enable_params.service_changed = IS_SRVC_CHANGED_CHARACT_PRESENT;
    err_code = sd_ble_enable(&ble_enable_params);
    APP_ERROR_CHECK(err_code);
#endif

    // Register with the SoftDevice handler module for BLE events.
    err_code = softdevice_ble_evt_handler_set(ble_evt_dispatch);
    APP_ERROR_CHECK(err_code);

    // Register with the SoftDevice handler module for BLE events.
    err_code = softdevice_sys_evt_handler_set(sys_evt_dispatch);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the Advertising functionality.
 */
static void advertising_init(void)
{
    uint32_t      err_code;
    ble_advdata_t advdata;

    // Build advertising data struct to pass into @ref ble_advertising_init.
    memset(&advdata, 0, sizeof(advdata));

    advdata.name_type               = BLE_ADVDATA_FULL_NAME;
    advdata.include_appearance      = true;
    advdata.flags                   = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
    advdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    advdata.uuids_complete.p_uuids  = m_adv_uuids;

    ble_adv_modes_config_t options = {0};
    options.ble_adv_fast_enabled  = BLE_ADV_FAST_ENABLED;
    options.ble_adv_fast_interval = APP_ADV_INTERVAL;
    options.ble_adv_fast_timeout  = APP_ADV_TIMEOUT_IN_SECONDS;

    err_code = ble_advertising_init(&advdata, NULL, &options, on_adv_evt, NULL);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for the Power manager.
 */
static void power_manage(void)
{
    uint32_t err_code = sd_app_evt_wait();
    APP_ERROR_CHECK(err_code);
}


void car_init()
{
	ret_code_t err_code;

	err_code = nrf_drv_gpiote_init();
	APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_ppi_init();
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

	InitUltraSound();
}

void ble_on_radio_active_evt(bool radio_active)
{
  if(radio_active)
  {
		nrf_gpio_pin_toggle(Pin_LED1);
    LightTick();
		UltraSoundTick();
		if (!RemoteFail())
		{
				SetServo(ble_car.packet.steering * 256);
		}
		else
		{
				SetServo(0);
		}
  }
}

void ble_start()
{
    uint32_t err_code;
	ble_stack_init();
	//device_manager_init(false);
	gap_params_init();
	advertising_init();
	services_init();
	conn_params_init();

	err_code = ble_radio_notification_init(NRF_APP_PRIORITY_HIGH,
																				 NRF_RADIO_NOTIFICATION_DISTANCE_4560US,
																				 ble_on_radio_active_evt);

																				 	 APP_ERROR_CHECK(err_code);

err_code = ble_advertising_start(BLE_ADV_MODE_FAST);
	 APP_ERROR_CHECK(err_code);

}

void ble_stop()
{
    uint32_t err_code;

		if (m_conn_handle != BLE_CONN_HANDLE_INVALID)
    {
        err_code = sd_ble_gap_disconnect(m_conn_handle,  BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
		    APP_ERROR_CHECK(err_code);
    }

		err_code = sd_softdevice_disable();
    APP_ERROR_CHECK(err_code);

		err_code = sd_ble_gap_adv_stop();
    APP_ERROR_CHECK(err_code);

    // Stop any impending connection parameters update.
    err_code = ble_conn_params_stop();
    APP_ERROR_CHECK(err_code);

    err_code = softdevice_handler_sd_disable();
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for application main entry.
 */
int main(void)
{
    timers_init();

		car_init();

ble_start();

CurrentMode = BleWait;

    // Start execution.
    application_timers_start();
    // Enter main loop.
    for (;;)
    {
        power_manage();
/*
								if(CurrentMode == BleWait)
								{
									if(mode_tick > 10 * 50)
									{
										ble_stop();
										CurrentMode = EsbWait;
										mode_tick = 0;
									}
									else
									{

									}
								}
								else if(CurrentMode == EsbWait)
								{
									if(mode_tick > 10 * 50)
									{
										ble_start();
										CurrentMode = BleWait;
										mode_tick = 0;
									}
									else
									{

									}
								}*/

    }
}
