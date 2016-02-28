#include "ws2812b-nrf51.h"

#include "nrf_delay.h"
#include "nrf_gpio.h"

#define NEOPIXEL_SEND_ONE	NRF_GPIO->OUTSET = (1UL << PIN); \
		__ASM ( \
				" NOP\n\t" \
				" NOP\n\t" \
				" NOP\n\t" \
				" NOP\n\t" \
				" NOP\n\t" \
				" NOP\n\t" \
			); \
		NRF_GPIO->OUTCLR = (1UL << PIN);  \
		__ASM ( \
				" NOP\n\t" \
				" NOP\n\t" \
				" NOP\n\t" \
			);

#define NEOPIXEL_SEND_ZERO NRF_GPIO->OUTSET = (1UL << PIN); \
		__ASM (  \
				" NOP\n\t" \
			);  \
		NRF_GPIO->OUTCLR = (1UL << PIN);  \
		__ASM ( \
				" NOP\n\t" \
				" NOP\n\t" \
				" NOP\n\t" \
				" NOP\n\t" \
				" NOP\n\t" \
				" NOP\n\t" \
				" NOP\n\t" \
				" NOP\n\t" \
				" NOP\n\t" \
				" NOP\n\t" \
			);

void ws2812b_init(uint8_t PIN)
{
  nrf_gpio_cfg_output(PIN);
}

void ws2812b_write(uint8_t PIN, const uint8_t *strip, uint32_t size)
{
      const uint8_t *end = strip + size;
    	//NRF_GPIO->OUTCLR = (1UL << PIN);
    	//nrf_delay_us(50);

			while (strip < end)
			{
					if (((*strip) & 128) > 0)	{NEOPIXEL_SEND_ONE}
					else	{NEOPIXEL_SEND_ZERO}

					if (((*strip) & 64) > 0)	{NEOPIXEL_SEND_ONE}
					else	{NEOPIXEL_SEND_ZERO}

					if (((*strip) & 32) > 0)	{NEOPIXEL_SEND_ONE}
					else	{NEOPIXEL_SEND_ZERO}

					if (((*strip) & 16) > 0)	{NEOPIXEL_SEND_ONE}
					else	{NEOPIXEL_SEND_ZERO}

					if (((*strip) & 8) > 0)	{NEOPIXEL_SEND_ONE}
					else	{NEOPIXEL_SEND_ZERO}

					if (((*strip) & 4) > 0)	{NEOPIXEL_SEND_ONE}
					else	{NEOPIXEL_SEND_ZERO}

					if (((*strip) & 2) > 0)	{NEOPIXEL_SEND_ONE}
					else	{NEOPIXEL_SEND_ZERO}

					if (((*strip) & 1) > 0)	{NEOPIXEL_SEND_ONE}
					else	{NEOPIXEL_SEND_ZERO}

          strip++;
			}
}
