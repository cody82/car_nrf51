#ifndef __CAR_ADVERTISING_H
#define __CAR_ADVERTISING_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "ble.h"
#include "app_util.h"


uint32_t car_adv_data_set(const ble_uuid128_t *service_uuid);

#endif
