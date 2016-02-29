#include "car_esb.h"
#include "nrf_esb.h"

// Define pipe
#define PIPE_NUMBER 0 ///< We use pipe 0 in this example

// Define payload length
#define TX_PAYLOAD_LENGTH 1 ///< We use 1 byte payload length when transmitting

// Data and acknowledgement payloads
//static uint8_t my_tx_payload[TX_PAYLOAD_LENGTH];                ///< Payload to send to PRX.
static uint8_t my_rx_payload[NRF_ESB_CONST_MAX_PAYLOAD_LENGTH]; ///< Placeholder for received ACK payloads from PRX.

volatile Packet packet;
volatile int32_t rc_timeout = 0;


void InitEsb()
{
	(void)nrf_esb_init(NRF_ESB_MODE_PRX);

	nrf_esb_set_datarate(NRF_ESB_DATARATE_1_MBPS);
	nrf_esb_set_channel(2);

	(void)nrf_esb_enable();
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
