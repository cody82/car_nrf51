/* Copyright (c) 2012 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */

/**@file
 *
 * @defgroup ble_sdk_srv_car Nordic UART Service
 * @{
 * @ingroup  ble_sdk_srv
 * @brief    Nordic UART Service implementation.
 *
 * @details The Nordic UART Service is a simple GATT-based service with TX and RX characteristics.
 *          Data received from the peer is passed to the application, and the data received
 *          from the application of this service is sent to the peer as Handle Value
 *          Notifications. This module demonstrates how to implement a custom GATT-based
 *          service and characteristics using the S110 SoftDevice. The service
 *          is used by the application to send and receive ASCII text strings to and from the
 *          peer.
 *
 * @note The application must propagate S110 SoftDevice events to the Nordic UART Service module
 *       by calling the ble_car_on_ble_evt() function from the ble_stack_handler callback.
 */

#ifndef BLE_CAR_H__
#define BLE_CAR_H__

#include "ble.h"
#include "ble_srv_common.h"
#include <stdint.h>
#include <stdbool.h>

#define BLE_UUID_CAR_SERVICE 0xA000                      /**< The UUID of the Nordic UART Service. */
#define BLE_CAR_MAX_DATA_LEN (GATT_MTU_SIZE_DEFAULT - 3) /**< Maximum length of data (in bytes) that can be transmitted to the peer by the Nordic UART service module. */

#define CAR_BASE_UUID                  {{0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}} /**< Used vendor specific UUID. */

/* Forward declaration of the ble_car_t type. */
typedef struct ble_car_s ble_car_t;

/**@brief Nordic UART Service event handler type. */
typedef void (*ble_car_data_handler_t) (ble_car_t * p_car, uint8_t * p_data, uint16_t length);

/**@brief Nordic UART Service initialization structure.
 *
 * @details This structure contains the initialization information for the service. The application
 * must fill this structure and pass it to the service using the @ref ble_car_init
 *          function.
 */
typedef struct
{
    ble_car_data_handler_t data_handler; /**< Event handler to be called for handling received data. */
} ble_car_init_t;

typedef struct
{
    int8_t steering;
    int8_t throttle;
    uint8_t front_light;
    bool top_light;
    bool blink_left;
    bool blink_right;
    bool beep;
} Packet;

typedef struct
{
  uint16_t voltage;
  uint8_t front_dist;
  uint8_t back_dist;
} Telemetry;
/**@brief Nordic UART Service structure.
 *
 * @details This structure contains status information related to the service.
 */
struct ble_car_s
{
    uint8_t                  uuid_type;               /**< UUID type for Nordic UART Service Base UUID. */
    uint16_t                 service_handle;          /**< Handle of Nordic UART Service (as provided by the S110 SoftDevice). */
    ble_gatts_char_handles_t control_handles;              /**< Handles related to the RX characteristic (as provided by the S110 SoftDevice). */
    ble_gatts_char_handles_t settings_handles;              /**< Handles related to the RX characteristic (as provided by the S110 SoftDevice). */
    ble_gatts_char_handles_t battery_handles;              /**< Handles related to the RX characteristic (as provided by the S110 SoftDevice). */
    uint16_t                 conn_handle;             /**< Handle of the current connection (as provided by the S110 SoftDevice). BLE_CONN_HANDLE_INVALID if not in a connection. */
    bool                     is_notification_enabled; /**< Variable to indicate if the peer has enabled notification of the RX characteristic.*/
    ble_car_data_handler_t   data_handler;            /**< Event handler to be called for handling received data. */
    Packet packet;
};

uint32_t ble_car_set_telemetry(ble_car_t * p_car, uint16_t mv, uint8_t front_dist, uint8_t back_dist);

/**@brief Function for initializing the Nordic UART Service.
 *
 * @param[out] p_car      Nordic UART Service structure. This structure must be supplied
 *                        by the application. It is initialized by this function and will
 *                        later be used to identify this particular service instance.
 * @param[in] p_car_init  Information needed to initialize the service.
 *
 * @retval NRF_SUCCESS If the service was successfully initialized. Otherwise, an error code is returned.
 * @retval NRF_ERROR_NULL If either of the pointers p_car or p_car_init is NULL.
 */
uint32_t ble_car_init(ble_car_t * p_car, const ble_car_init_t * p_car_init);

/**@brief Function for handling the Nordic UART Service's BLE events.
 *
 * @details The Nordic UART Service expects the application to call this function each time an
 * event is received from the S110 SoftDevice. This function processes the event if it
 * is relevant and calls the Nordic UART Service event handler of the
 * application if necessary.
 *
 * @param[in] p_car       Nordic UART Service structure.
 * @param[in] p_ble_evt   Event received from the S110 SoftDevice.
 */
void ble_car_on_ble_evt(ble_car_t * p_car, ble_evt_t * p_ble_evt);


#endif // BLE_CAR_H__

/** @} */
