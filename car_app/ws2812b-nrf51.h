#ifndef __WS2812B_NRF51__H
#define __WS2812B_NRF51__H

#include <stdint.h>

void ws2812b_init(uint8_t PIN);
void ws2812b_write(uint8_t PIN, const uint8_t *strip, uint32_t size);

#endif
