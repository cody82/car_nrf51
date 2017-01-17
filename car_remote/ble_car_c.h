/*
 * Copyright (c) 2012 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is confidential property of Nordic Semiconductor. The use,
 * copying, transfer or disclosure of such information is prohibited except by express written
 * agreement with Nordic Semiconductor.
 *
 */

/**@file
 *
 * @defgroup ble_car_c Nordic UART Service Client
 * @{
 * @ingroup  ble_sdk_srv
 * @brief    Nordic UART Service Client module.
 *
 * @details  This module contains the APIs and types exposed by the Nordic UART Service Client
 *           module. These APIs and types can be used by the application to perform discovery of
 *           the Nordic UART Service at the peer and interact with it.
 *
 * @note     The application must propagate BLE stack events to this module by calling
 *           ble_car_c_on_ble_evt().
 *
 */


#ifndef BLE_CAR_C_H__
#define BLE_CAR_C_H__

#include <stdint.h>
#include <stdbool.h>
#include "ble.h"
#include "ble_gatt.h"
#include "ble_db_discovery.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CAR_BASE_UUID                  {{0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}} /**< Used vendor specific UUID. */
#define BLE_UUID_CAR_SERVICE           0xA000
#define BLE_UUID_CAR_CONTROL_CHARACTERISTIC 0xA010
#define BLE_UUID_CAR_SETTINGS_CHARACTERISTIC 0xA005
#define BLE_UUID_CAR_BATTERY_CHARACTERISTIC 0xA001

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

#define BLE_CAR_MAX_CONTROL_CHAR_LEN        (sizeof(Packet)) //7
#define BLE_CAR_MAX_SETTINGS_CHAR_LEN        (sizeof(Settings))
#define BLE_CAR_MAX_BATTERY_CHAR_LEN        (sizeof(Telemetry))

#define BLE_CAR_MAX_DATA_LEN           (GATT_MTU_SIZE_DEFAULT - 3) /**< Maximum length of data (in bytes) that can be transmitted to the peer by the Nordic UART service module. */


/**@brief CAR Client event type. */
typedef enum
{
    BLE_CAR_C_EVT_DISCOVERY_COMPLETE = 1, /**< Event indicating that the CAR service and its characteristics was found. */
    BLE_CAR_C_EVT_CAR_RX_EVT,             /**< Event indicating that the central has received something from a peer. */
    BLE_CAR_C_EVT_DISCONNECTED            /**< Event indicating that the CAR server has disconnected. */
} ble_car_c_evt_type_t;


/**@brief Handles on the connected peer device needed to interact with it.
*/
typedef struct {
    uint16_t                car_control_handle;      /**< Handle of the CAR RX characteristic as provided by a discovery. */
    uint16_t                car_rx_cccd_handle; /**< Handle of the CCCD of the CAR RX characteristic as provided by a discovery. */
    uint16_t                car_tx_handle;      /**< Handle of the CAR TX characteristic as provided by a discovery. */
} ble_car_c_handles_t;


/**@brief Structure containing the CAR event data received from the peer. */
typedef struct {
    ble_car_c_evt_type_t evt_type;
    uint16_t             conn_handle;
    uint8_t            * p_data;
    uint8_t              data_len;
    ble_car_c_handles_t  handles;     /**< Handles on which the Nordic Uart service characteristics was discovered on the peer device. This will be filled if the evt_type is @ref BLE_CAR_C_EVT_DISCOVERY_COMPLETE.*/
} ble_car_c_evt_t;


// Forward declaration of the ble_car_t type.
typedef struct ble_car_c_s ble_car_c_t;

/**@brief   Event handler type.
 *
 * @details This is the type of the event handler that should be provided by the application
 *          of this module to receive events.
 */
typedef void (* ble_car_c_evt_handler_t)(ble_car_c_t * p_ble_car_c, const ble_car_c_evt_t * p_evt);


/**@brief CAR Client structure.
 */
struct ble_car_c_s
{
    uint8_t                 uuid_type;          /**< UUID type. */
    uint16_t                conn_handle;        /**< Handle of the current connection. Set with @ref ble_car_c_handles_assign when connected. */
    ble_car_c_handles_t     handles;            /**< Handles on the connected peer device needed to interact with it. */
    ble_car_c_evt_handler_t evt_handler;        /**< Application event handler to be called when there is an event related to the CAR. */
};

/**@brief CAR Client initialization structure.
 */
typedef struct {
    ble_car_c_evt_handler_t evt_handler;
} ble_car_c_init_t;


/**@brief     Function for initializing the Nordic UART client module.
 *
 * @details   This function registers with the Database Discovery module
 *            for the CAR. Doing so will make the Database Discovery
 *            module look for the presence of a CAR instance at the peer when a
 *            discovery is started.
 *
 * @param[in] p_ble_car_c      Pointer to the CAR client structure.
 * @param[in] p_ble_car_c_init Pointer to the CAR initialization structure containing the
 *                             initialization information.
 *
 * @retval    NRF_SUCCESS If the module was initialized successfully. Otherwise, an error
 *                        code is returned. This function
 *                        propagates the error code returned by the Database Discovery module API
 *                        @ref ble_db_discovery_evt_register.
 */
uint32_t ble_car_c_init(ble_car_c_t * p_ble_car_c, ble_car_c_init_t * p_ble_car_c_init);


/**@brief Function for handling events from the database discovery module.
 *
 * @details This function will handle an event from the database discovery module, and determine
 *          if it relates to the discovery of CAR at the peer. If so, it will
 *          call the application's event handler indicating that CAR has been
 *          discovered at the peer. It also populates the event with the service related
 *          information before providing it to the application.
 *
 * @param[in] p_ble_car_c Pointer to the CAR client structure.
 * @param[in] p_evt       Pointer to the event received from the database discovery module.
 */
 void ble_car_c_on_db_disc_evt(ble_car_c_t * p_ble_car_c, ble_db_discovery_evt_t * p_evt);


/**@brief     Function for handling BLE events from the SoftDevice.
 *
 * @details   This function handles the BLE events received from the SoftDevice. If a BLE
 *            event is relevant to the CAR module, it is used to update
 *            internal variables and, if necessary, send events to the application.
 *
 * @param[in] p_ble_car_c Pointer to the CAR client structure.
 * @param[in] p_ble_evt   Pointer to the BLE event.
 */
void ble_car_c_on_ble_evt(ble_car_c_t * p_ble_car_c, const ble_evt_t * p_ble_evt);

uint32_t ble_car_c_control_send(ble_car_c_t * p_ble_car_c, Packet* p_string);


/**@brief Function for assigning handles to a this instance of car_c.
 *
 * @details Call this function when a link has been established with a peer to
 *          associate this link to this instance of the module. This makes it
 *          possible to handle several link and associate each link to a particular
 *          instance of this module. The connection handle and attribute handles will be
 *          provided from the discovery event @ref BLE_CAR_C_EVT_DISCOVERY_COMPLETE.
 *
 * @param[in] p_ble_car_c    Pointer to the CAR client structure instance to associate with these
 *                           handles.
 * @param[in] conn_handle    Connection handle to associated with the given CAR Instance.
 * @param[in] p_peer_handles Attribute handles on the CAR server that you want this CAR client to
 *                           interact with.
 *
 * @retval    NRF_SUCCESS    If the operation was successful.
 * @retval    NRF_ERROR_NULL If a p_car was a NULL pointer.
 */
uint32_t ble_car_c_handles_assign(ble_car_c_t * p_ble_car_c, const uint16_t conn_handle, const ble_car_c_handles_t * p_peer_handles);



#ifdef __cplusplus
}
#endif

#endif // BLE_CAR_C_H__

/** @} */
