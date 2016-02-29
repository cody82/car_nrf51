#include <stdint.h>
#include "nrf.h"
#include "nrf_drv_config.h"
#include "nrf_esb.h"
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

// Define pipe
#define PIPE_NUMBER 0 ///< We use pipe 0 in this example

// Define payload length
#define TX_PAYLOAD_LENGTH 1 ///< We use 1 byte payload length when transmitting

// Data and acknowledgement payloads
//static uint8_t my_tx_payload[TX_PAYLOAD_LENGTH];                ///< Payload to send to PRX.
static uint8_t my_rx_payload[NRF_ESB_CONST_MAX_PAYLOAD_LENGTH]; ///< Placeholder for received ACK payloads from PRX.

const nrf_drv_rtc_t rtc = //NRF_DRV_RTC_INSTANCE(0); /**< Declaring an instance of nrf_drv_rtc for RTC0. */
{
 .p_reg = NRF_RTC1,
 .irq = RTC1_IRQn,
 .instance_id = 1
};

static void lfclk_config(void)
{
	ret_code_t err_code = nrf_drv_clock_init(NULL);
	APP_ERROR_CHECK(err_code);

	nrf_drv_clock_lfclk_request();
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

typedef struct
{
	uint16_t crc16;
	//uint8_t xxx;
	uint8_t type;
	int16_t steering;
	int16_t throttle;
	uint8_t front_light;
	uint8_t top_light;
	uint8_t blink_left;
	uint8_t blink_right;
	uint8_t beep;
} Packet;

volatile Packet packet;

void SetServo(int16_t steering)
{
	steering /= 66;
	nrf_gpio_pin_set(Pin_Servo);
	nrf_delay_us(1500 + steering);
	nrf_gpio_pin_clear(Pin_Servo);
}

int32_t rc_timeout = 0;

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
		SetBackLight(packet.throttle < 0);
	}
}

static uint32_t tick = 0;

void loop()
{
	BatteryTick();

	rc_timeout++;

	bool blocked_front = (UltraSoundFrontDist >= 0) && (UltraSoundFrontDist < 20);
	bool blocked_back = (UltraSoundBackDist >= 0) && (UltraSoundBackDist < 20);

	CalcLights();

	nrf_esb_disable();
	LightTick();

	nrf_esb_enable();

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

	InitUltraSound(&rtc);

	// Initialize ESB
	(void)nrf_esb_init(NRF_ESB_MODE_PRX);

	nrf_esb_set_datarate(NRF_ESB_DATARATE_1_MBPS);
	nrf_esb_set_channel(2);

	(void)nrf_esb_enable();

	while (1)
	{
		loop();
	}
}

void Car_Receive(const Packet *p)
{
	rc_timeout = 0;
	packet = *p;

}

void nrf_esb_tx_success(uint32_t tx_pipe, int32_t rssi)
{
}

void nrf_esb_rx_data_ready(uint32_t rx_pipe, int32_t rssi)
{
	uint32_t my_rx_payload_length;

	(void)nrf_esb_fetch_packet_from_rx_fifo(PIPE_NUMBER, my_rx_payload, &my_rx_payload_length);
	if (my_rx_payload_length > 0)
	{
		if (my_rx_payload_length == sizeof(Packet))
		{
			Packet *p = (Packet*)my_rx_payload;
			Car_Receive(p);
		}
	}
}

void nrf_esb_tx_failed(uint32_t tx_pipe) {}
void nrf_esb_disabled(void) {}
