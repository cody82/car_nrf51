#include "car_esb.h"
#include "sdk_common.h"
#include "nrf_esb.h"

#include "nrf_error.h"
#include "nrf.h"
#include "nrf_esb_error_codes.h"
#include "app_error.h"

// Define pipe
#define PIPE_NUMBER 0 ///< We use pipe 0 in this example

// Define payload length
//#define TX_PAYLOAD_LENGTH 1 ///< We use 1 byte payload length when transmitting


nrf_esb_payload_t rx_payload;

volatile Packet packet;
volatile int32_t rc_timeout = 0;


void Car_Receive(const Packet *p)
{
	rc_timeout = 0;
	packet = *p;

}
void nrf_esb_event_handler(nrf_esb_evt_t const * p_event)
{
    switch (p_event->evt_id)
    {
        case NRF_ESB_EVENT_TX_SUCCESS:
            //NRF_LOG("TX SUCCESS EVENT\r\n");
            break;
        case NRF_ESB_EVENT_TX_FAILED:
            //NRF_LOG("TX FAILED EVENT\r\n");
            break;
        case NRF_ESB_EVENT_RX_RECEIVED:
            //NRF_LOG("RX RECEIVED EVENT\r\n");
            if (nrf_esb_read_rx_payload(&rx_payload) == NRF_SUCCESS)
            {
                // Set LEDs identical to the ones on the PTX.
                //nrf_gpio_pin_write(LED_1, !(rx_payload.data[1]%8>0 && rx_payload.data[1]%8<=4));
                //nrf_gpio_pin_write(LED_2, !(rx_payload.data[1]%8>1 && rx_payload.data[1]%8<=5));
                //nrf_gpio_pin_write(LED_3, !(rx_payload.data[1]%8>2 && rx_payload.data[1]%8<=6));
                //nrf_gpio_pin_write(LED_4, !(rx_payload.data[1]%8>3));

                //NRF_LOG("Receiving packet: ");
                //NRF_LOG_HEX_CHAR(rx_payload.data[1]);
                //NRF_LOG("\r\n");
				
				if (rx_payload.length > 0)
				{
					if (rx_payload.length == sizeof(Packet))
					{
						Packet *p = (Packet*)rx_payload.data;
						Car_Receive(p);
					}
				}
            }
            break;
    }
}


uint32_t InitEsb()
{
    uint32_t err_code;
    uint8_t base_addr_0[4] = {0xE7, 0xE7, 0xE7, 0xE7};
    uint8_t base_addr_1[4] = {0xC2, 0xC2, 0xC2, 0xC2};
    uint8_t addr_prefix[8] = {0xE7, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8 };

    nrf_esb_config_t nrf_esb_config         = NRF_ESB_DEFAULT_CONFIG;
    nrf_esb_config.protocol                 = NRF_ESB_PROTOCOL_ESB_DPL;
    nrf_esb_config.retransmit_delay         = 600;
    nrf_esb_config.bitrate                  = NRF_ESB_BITRATE_2MBPS;
    nrf_esb_config.event_handler            = nrf_esb_event_handler;
    nrf_esb_config.mode                     = NRF_ESB_MODE_PTX;
    nrf_esb_config.selective_auto_ack       = false;

    err_code = nrf_esb_init(&nrf_esb_config);

    VERIFY_SUCCESS(err_code);

    err_code = nrf_esb_set_base_address_0(base_addr_0);
    VERIFY_SUCCESS(err_code);

    err_code = nrf_esb_set_base_address_1(base_addr_1);
    VERIFY_SUCCESS(err_code);

    err_code = nrf_esb_set_prefixes(addr_prefix, 8);
    VERIFY_SUCCESS(err_code);


    err_code = nrf_esb_start_rx();
    APP_ERROR_CHECK(err_code);
	//(void)nrf_esb_enable();
	return err_code;
}
