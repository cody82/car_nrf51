#include "car_esb.h"
#include "sdk_common.h"

#include "nrf_error.h"
#include "nrf.h"
#include "nrf_esb_error_codes.h"
#include "app_error.h"

nrf_esb_payload_t rx_payload;

volatile Packet packet;
volatile int32_t rc_timeout = 0;


void Car_Receive(const Packet *p)
{
	rc_timeout = 0;
	packet = *p;

}

void Car_EsbReceive()
{
            if (nrf_esb_read_rx_payload(&rx_payload) == NRF_SUCCESS)
            {
				if (rx_payload.length > 0)
				{
					if (rx_payload.length == sizeof(Packet))
					{
						Packet *p = (Packet*)rx_payload.data;
						Car_Receive(p);
					}
				}
            }
}

void nrf_esb_event_handler(nrf_esb_evt_t const * p_event)
{
    switch (p_event->evt_id)
    {
        case NRF_ESB_EVENT_TX_SUCCESS:
            break;
        case NRF_ESB_EVENT_TX_FAILED:
            break;
        case NRF_ESB_EVENT_RX_RECEIVED:
        Car_EsbReceive();
            break;
    }
}


uint32_t InitEsb()
{
    uint32_t err_code;
    uint8_t base_addr_0[4] = {0xE7, 0xE7, 0xE7, 0xE7};
    uint8_t base_addr_1[4] = {0xE7, 0xE7, 0xE7, 0xE7};
    uint8_t addr_prefix[8] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7, 0xE7, 0xE7, 0xE7 };

    nrf_esb_config_t nrf_esb_config         = NRF_ESB_DEFAULT_CONFIG;
    nrf_esb_config.protocol                 = NRF_ESB_PROTOCOL_ESB_DPL;
    nrf_esb_config.retransmit_delay         = 600;
    nrf_esb_config.bitrate                  = NRF_ESB_BITRATE_1MBPS;
    nrf_esb_config.event_handler            = nrf_esb_event_handler;
    nrf_esb_config.mode                     = NRF_ESB_MODE_PRX;
    nrf_esb_config.selective_auto_ack       = false;
    nrf_esb_config.crc                      = NRF_ESB_CRC_8BIT;

    err_code = nrf_esb_init(&nrf_esb_config);

    VERIFY_SUCCESS(err_code);

    err_code = nrf_esb_set_base_address_0(base_addr_0);
    VERIFY_SUCCESS(err_code);

    err_code = nrf_esb_set_base_address_1(base_addr_1);
    VERIFY_SUCCESS(err_code);

    err_code = nrf_esb_set_prefixes(addr_prefix, 8);
    VERIFY_SUCCESS(err_code);

    err_code = nrf_esb_set_rf_channel(2);
    VERIFY_SUCCESS(err_code);

    err_code = nrf_esb_start_rx();
    APP_ERROR_CHECK(err_code);
    
	return err_code;
}

uint32_t ShutdownEsb()
{
    return nrf_esb_disable();
}