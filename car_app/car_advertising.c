#include "car_advertising.h"

#include "ble_gap.h"
#include "ble_srv_common.h"
#include "sdk_common.h"
#include "ble_advdata.h"


static uint32_t car_name_encode(uint8_t             * p_encoded_data,
                            uint16_t            * p_offset,
                            uint16_t              max_size)
{
    uint32_t err_code;
    uint16_t rem_adv_data_len;
    uint16_t actual_length;
    uint8_t  adv_data_format;

    // Check for buffer overflow.
    if (((*p_offset) + ADV_AD_DATA_OFFSET) > max_size)
    {
        return NRF_ERROR_DATA_SIZE;
    }

    rem_adv_data_len = max_size - (*p_offset) - ADV_AD_DATA_OFFSET;
    actual_length    = rem_adv_data_len;

    // Get GAP device name and length
    err_code = sd_ble_gap_device_name_get(&p_encoded_data[(*p_offset) + ADV_AD_DATA_OFFSET],
                                          &actual_length);
    VERIFY_SUCCESS(err_code);

    // Check if device intend to use short name and it can fit available data size.
    if (actual_length <= rem_adv_data_len)
    {
        // Complete device name can fit, setting Complete Name in Adv Data.
        adv_data_format = BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME;
    }
    else
    {
        // Else short name needs to be used. Or application has requested use of short name.
        adv_data_format = BLE_GAP_AD_TYPE_SHORT_LOCAL_NAME;

        actual_length = rem_adv_data_len;
    }

    // There is only 1 byte intended to encode length which is (actual_length + ADV_AD_TYPE_FIELD_SIZE)
    if (actual_length > (0x00FF - ADV_AD_TYPE_FIELD_SIZE))
    {
        return NRF_ERROR_DATA_SIZE;
    }

    // Complete name field in encoded data.
    p_encoded_data[*p_offset]  = (uint8_t)(ADV_AD_TYPE_FIELD_SIZE + actual_length);
    *p_offset                 += ADV_LENGTH_FIELD_SIZE;
    p_encoded_data[*p_offset]  = adv_data_format;
    *p_offset                 += ADV_AD_TYPE_FIELD_SIZE;
    *p_offset                 += actual_length;

    return NRF_SUCCESS;
}

static uint32_t car_flags_encode(int8_t     flags,
                             uint8_t  * p_encoded_data,
                             uint16_t * p_offset,
                             uint16_t   max_size)
{
    // Check for buffer overflow.
    if (((*p_offset) + AD_TYPE_FLAGS_SIZE) > max_size)
    {
        return NRF_ERROR_DATA_SIZE;
    }

    // Encode flags.
    p_encoded_data[*p_offset]  = (uint8_t)(ADV_AD_TYPE_FIELD_SIZE + AD_TYPE_FLAGS_DATA_SIZE);
    *p_offset                 += ADV_LENGTH_FIELD_SIZE;
    p_encoded_data[*p_offset]  = BLE_GAP_AD_TYPE_FLAGS;
    *p_offset                 += ADV_AD_TYPE_FIELD_SIZE;
    p_encoded_data[*p_offset]  = flags;
    *p_offset                 += AD_TYPE_FLAGS_DATA_SIZE;

    return NRF_SUCCESS;
}

static uint32_t car_uuid128_encode(const ble_uuid128_t *service_uuid,
                             uint8_t  * p_encoded_data,
                             uint16_t * p_offset,
                             uint16_t   max_size)
{
    if (((*p_offset) + ADV_AD_DATA_OFFSET + sizeof(ble_uuid128_t)) > max_size)
    {
        return NRF_ERROR_DATA_SIZE;
    }
    
    p_encoded_data[*p_offset] = ADV_AD_TYPE_FIELD_SIZE + sizeof(ble_uuid128_t);
    *p_offset += ADV_LENGTH_FIELD_SIZE;
    p_encoded_data[*p_offset] = BLE_GAP_AD_TYPE_128BIT_SERVICE_UUID_MORE_AVAILABLE;
    *p_offset += ADV_AD_TYPE_FIELD_SIZE;
    memcpy(&p_encoded_data[*p_offset], service_uuid, sizeof(ble_uuid128_t));
    *p_offset += sizeof(ble_uuid128_t);

    return NRF_SUCCESS;
}

uint32_t car_adv_data_encode(const ble_uuid128_t *service_uuid,
                         uint8_t             * const p_encoded_data,
                         uint16_t            * const p_len)
{
    uint32_t err_code = NRF_SUCCESS;
    uint16_t max_size = *p_len;
    *p_len = 0;

    err_code = car_flags_encode(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE, p_encoded_data, p_len, max_size);
    VERIFY_SUCCESS(err_code);

    err_code = car_uuid128_encode(service_uuid, p_encoded_data, p_len, max_size);
    VERIFY_SUCCESS(err_code);

    err_code = car_name_encode(p_encoded_data, p_len, max_size);
    VERIFY_SUCCESS(err_code);

    return err_code;
}

uint8_t car_encoded_advdata[48];
uint16_t car_encoded_advdata_len;
uint32_t car_error;

uint32_t car_adv_data_set(const ble_uuid128_t *service_uuid)
{
    uint32_t err_code = NRF_SUCCESS;

    car_encoded_advdata_len = 31;
    err_code = car_adv_data_encode(service_uuid, car_encoded_advdata, &car_encoded_advdata_len);
    VERIFY_SUCCESS(err_code);

    car_error = sd_ble_gap_adv_data_set(car_encoded_advdata, car_encoded_advdata_len, NULL, 0);
    
    return NRF_SUCCESS;
}
