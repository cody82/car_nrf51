#include "timeslot.h"

#include "nrf_esb.h"
#include "nrf.h"
#include "nrf_error.h"
#include "nrf_soc.h"
#include "nrf_sdm.h"
#include "nrf_esb_error_codes.h"
#include "app_error.h"

#include "car_esb.h"
#include "nrf_gpio.h"

#define TIMESLOT_BEGIN_IRQn             LPCOMP_IRQn
#define TIMESLOT_BEGIN_IRQHandler       LPCOMP_IRQHandler
#define TIMESLOT_BEGIN_IRQPriority      1
#define TIMESLOT_END_IRQn               QDEC_IRQn
#define TIMESLOT_END_IRQHandler         QDEC_IRQHandler
#define TIMESLOT_END_IRQPriority        1
#define nrf_esb_RX_HANDLE_IRQn             WDT_IRQn
#define nrf_esb_RX_HANDLE_IRQHandler       WDT_IRQHandler
#define nrf_esb_RX_HANDLE_IRQPriority      3

#define MAX_TX_ATTEMPTS 20
  
#define TS_LEN_US                            (10000UL)
#define TX_LEN_EXTENSION_US                  (TS_LEN_US)
#define TS_SAFETY_MARGIN_US                  (1250UL)    /**< The timeslot activity should be finished with this much to spare. */
#define TS_EXTEND_MARGIN_US                  (2000UL)    /**< The timeslot activity should request an extension this long before end of timeslot. */
#define TS_REQ_AND_END_MARGIN_US             (150UL)     /**< Used to send "request and end" before closing the timeslot. */


void RADIO_IRQHandler(void);

static volatile bool    m_timeslot_session_open;
static uint32_t         m_blocked_cancelled_count;
static uint32_t         m_total_timeslot_length = 0;

static volatile enum
{
    UT_STATE_IDLE, /* Default state */
    UT_STATE_RX,   /* Waiting for packets */
    UT_STATE_TX    /* Trying to transmit packet */
}                       m_ut_state = UT_STATE_IDLE;

/**< This will be used when requesting the first timeslot or any time a timeslot is blocked or cancelled. */
static nrf_radio_request_t m_timeslot_req_earliest = {
        NRF_RADIO_REQ_TYPE_EARLIEST,
        .params.earliest = {
            NRF_RADIO_HFCLK_CFG_XTAL_GUARANTEED,
            NRF_RADIO_PRIORITY_NORMAL,
            TS_LEN_US,
            NRF_RADIO_EARLIEST_TIMEOUT_MAX_US
        }};
/**< This will be used at the end of each timeslot to request the next timeslot. */
static nrf_radio_signal_callback_return_param_t m_rsc_return_sched_next = {
        NRF_RADIO_SIGNAL_CALLBACK_ACTION_REQUEST_AND_END,
        .params.request = {
                (nrf_radio_request_t*) &m_timeslot_req_earliest
        }};
/**< This will be used at the end of each timeslot to request an extension of the timeslot. */
static nrf_radio_signal_callback_return_param_t m_rsc_extend = {
        NRF_RADIO_SIGNAL_CALLBACK_ACTION_EXTEND,
        .params.extend = {TX_LEN_EXTENSION_US}
        };

/**< This will be used at the end of each timeslot to request the next timeslot. */
static nrf_radio_signal_callback_return_param_t m_rsc_return_no_action = {
        NRF_RADIO_SIGNAL_CALLBACK_ACTION_NONE,
        .params.request = {NULL}
        };
        
/**@brief IRQHandler used for execution context management. 
  *        Any available handler can be used as we're not using the associated hardware.
  *        This handler is used to stop and disable UESB
  */
void TIMESLOT_END_IRQHandler(void)
{
    uint32_t err_code;
    
    nrf_gpio_pin_set(10);
    err_code = nrf_esb_stop_rx();
    
    //NRF_TIMER0->TASKS_CAPTURE[3]= 1;

    //if (!nrf_esb_is_idle() && NRF_TIMER0->CC[3] <= (TS_LEN_US - TS_REQ_AND_END_MARGIN_US))
    {
        // Try again in 150 us
        //NRF_TIMER0->CC[0] += 150;
        //return;
    }
    
    err_code = nrf_esb_flush_rx();
    APP_ERROR_CHECK(err_code);
    err_code = nrf_esb_flush_tx();
    APP_ERROR_CHECK(err_code);
    
    err_code = nrf_esb_disable();
    APP_ERROR_CHECK(err_code);
}

/**@brief IRQHandler used for execution context management. 
  *        Any available handler can be used as we're not using the associated hardware.
  *        This handler is used to initiate UESB RX/TX
  */
void TIMESLOT_BEGIN_IRQHandler(void)
{
    uint32_t       err_code;
    
//    DEBUG_PRINT("G");
    
	nrf_gpio_pin_clear(10);
        err_code = InitEsb();
        APP_ERROR_CHECK(err_code);
    
        err_code = nrf_esb_start_rx();
}

/**@brief IRQHandler used for execution context management. 
  *        Any available handler can be used as we're not using the associated hardware.
  *        This handler is used to notify of received data
  */
void nrf_esb_RX_HANDLE_IRQHandler(void)
{
    //Car_EsbReceive();
}

/**@brief   Function for receiving callbacks from the micro-esb library.
 */
/*
static void nrf_esb_event_handler(void)
{
    uint32_t rf_interrupts;

    nrf_esb_get_clear_interrupts(&rf_interrupts);

    if(rf_interrupts & NRF_ESB_INT_TX_FAILED_MSK)
    {
        // Transmit failed: flush buffer
        nrf_esb_flush_tx();

        m_tx_attempts += 1;
        m_ut_state     = UT_STATE_RX;
    }
    if (rf_interrupts & NRF_ESB_INT_TX_SUCCESS_MSK)
    {
    }
    if(rf_interrupts & NRF_ESB_INT_RX_DR_MSK)
    {
        // Data reception is handled in a lower priority interrupt
        NVIC_SetPendingIRQ(nrf_esb_RX_HANDLE_IRQn);
    }
}*/

/**@brief   Function for handling timeslot events.
 */
static nrf_radio_signal_callback_return_param_t * radio_callback (uint8_t signal_type)
{   
    // NOTE: This callback runs at lower-stack priority (the highest priority possible).
    switch (signal_type)
    {
    case NRF_RADIO_CALLBACK_SIGNAL_TYPE_START:
        // TIMER0 is pre-configured for 1Mhz.
        /*NRF_TIMER0->TASKS_STOP          = 1;
        NRF_TIMER0->TASKS_CLEAR         = 1;
        NRF_TIMER0->MODE                = (TIMER_MODE_MODE_Timer << TIMER_MODE_MODE_Pos);
        NRF_TIMER0->EVENTS_COMPARE[0]   = 0;
        NRF_TIMER0->EVENTS_COMPARE[1]   = 0;
        NRF_TIMER0->EVENTS_COMPARE[2]   = 0;
        NRF_TIMER0->INTENSET            = (TIMER_INTENSET_COMPARE0_Msk | TIMER_INTENSET_COMPARE1_Msk | TIMER_INTENSET_COMPARE2_Msk);
        NRF_TIMER0->CC[0]               = (TS_LEN_US - TS_SAFETY_MARGIN_US);
        NRF_TIMER0->CC[1]               = (TS_LEN_US - TS_EXTEND_MARGIN_US);
        NRF_TIMER0->CC[2]               = (TS_LEN_US - TS_REQ_AND_END_MARGIN_US);
        NRF_TIMER0->BITMODE             = (TIMER_BITMODE_BITMODE_24Bit << TIMER_BITMODE_BITMODE_Pos);
        NRF_TIMER0->TASKS_START         = 1;

        NRF_RADIO->POWER                = (RADIO_POWER_POWER_Enabled << RADIO_POWER_POWER_Pos);

        NVIC_EnableIRQ(TIMER0_IRQn);*/
      NRF_TIMER0->TASKS_STOP          = 1;
        NRF_TIMER0->TASKS_CLEAR         = 1;
        NRF_TIMER0->MODE                = (TIMER_MODE_MODE_Timer << TIMER_MODE_MODE_Pos);

        
        NRF_TIMER0->INTENSET = TIMER_INTENSET_COMPARE0_Msk;
        NRF_TIMER0->CC[0] = TS_LEN_US - 1000;
        
                NRF_TIMER0->BITMODE             = (TIMER_BITMODE_BITMODE_24Bit << TIMER_BITMODE_BITMODE_Pos);
        NRF_TIMER0->TASKS_START         = 1;

        NRF_RADIO->POWER                = (RADIO_POWER_POWER_Enabled << RADIO_POWER_POWER_Pos);

        
        NVIC_EnableIRQ(TIMER0_IRQn);   
        
    
        // UESB packet receiption and transmission are synchronized at the beginning of timeslot extensions. 
        // Ideally we would also transmit at the beginning of the initial timeslot, not only extensions,
        // but this is to simplify a bit. 
        NVIC_SetPendingIRQ(TIMESLOT_BEGIN_IRQn);
        break;
    
    case NRF_RADIO_CALLBACK_SIGNAL_TYPE_TIMER0:
        NVIC_SetPendingIRQ(TIMESLOT_END_IRQn);
        return (nrf_radio_signal_callback_return_param_t*) &m_rsc_return_sched_next;
        
    case NRF_RADIO_CALLBACK_SIGNAL_TYPE_RADIO:
        // Call the uesb IRQHandler
        RADIO_IRQHandler();
        break;
    
    case NRF_RADIO_CALLBACK_SIGNAL_TYPE_EXTEND_FAILED:
        // Don't do anything. Our timer will expire before timeslot ends
        break;
    
    case NRF_RADIO_CALLBACK_SIGNAL_TYPE_EXTEND_SUCCEEDED:
        break;
    
    default:
        //app_error_handler(MAIN_DEBUG, __LINE__, (const uint8_t*)__FILE__);
        break;
    };

    // Fall-through return: return with no action request
    return (nrf_radio_signal_callback_return_param_t*) &m_rsc_return_no_action;
}


uint32_t timeslot_init()
{
    // Using three avilable interrupt handlers for interrupt level management
    // These can be any available IRQ as we're not using any of the hardware,
    // simply triggering them through software
    NVIC_ClearPendingIRQ(TIMESLOT_END_IRQn);
    NVIC_SetPriority(TIMESLOT_END_IRQn, TIMESLOT_END_IRQPriority);
    NVIC_EnableIRQ(TIMESLOT_END_IRQn);
    
    NVIC_ClearPendingIRQ(TIMESLOT_BEGIN_IRQn);
    NVIC_SetPriority(TIMESLOT_BEGIN_IRQn, TIMESLOT_BEGIN_IRQPriority);
    NVIC_EnableIRQ(TIMESLOT_BEGIN_IRQn);
    
    NVIC_ClearPendingIRQ(nrf_esb_RX_HANDLE_IRQn);
    NVIC_SetPriority(nrf_esb_RX_HANDLE_IRQn, nrf_esb_RX_HANDLE_IRQPriority);
    NVIC_EnableIRQ(nrf_esb_RX_HANDLE_IRQn);
    
    return 0;
}

        
uint32_t timeslot_start()
{
    uint32_t err_code;
    
    if (m_timeslot_session_open)
    {
        return NRF_ERROR_INVALID_STATE;
    }
    
    m_blocked_cancelled_count  = 0;
    
    err_code = sd_radio_session_open(radio_callback);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    err_code = sd_radio_request(&m_timeslot_req_earliest);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }
    
    m_timeslot_session_open = true;
    
    return err_code;
}
   
uint32_t timeslot_stop()
{
    uint32_t err_code;
    err_code = sd_radio_session_close();
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

        return err_code;
}

/**@brief Function for handling the Application's system events.
 *
 * @param[in]   sys_evt   system event.
 */
void timeslot_on_sys_evt(uint32_t sys_evt)
{
    nrf_gpio_pin_toggle(6);
    switch(sys_evt)
    {
        case NRF_EVT_FLASH_OPERATION_SUCCESS:
        case NRF_EVT_FLASH_OPERATION_ERROR:
            break;
        case NRF_EVT_RADIO_BLOCKED:
        case NRF_EVT_RADIO_CANCELED:
        {
            // Blocked events are rescheduled with normal priority. They could also
            // be rescheduled with high priority if necessary.
            uint32_t err_code = sd_radio_request((nrf_radio_request_t*) &m_timeslot_req_earliest);
            APP_ERROR_CHECK(err_code);

            m_blocked_cancelled_count++;
            
            break;
        }
        case NRF_EVT_RADIO_SIGNAL_CALLBACK_INVALID_RETURN:
            //DEBUG_PRINT("NRF_EVT_RADIO_SIGNAL_CALLBACK_INVALID_RETURN\r\n");
            //app_error_handler(MAIN_DEBUG, __LINE__, (const uint8_t*)__FILE__);
            break;
        case NRF_EVT_RADIO_SESSION_CLOSED:
            {
                m_timeslot_session_open = false;
                
                //DEBUG_PRINT("NRF_EVT_RADIO_SESSION_CLOSED\r\n");
            }
        
            break;
        case NRF_EVT_RADIO_SESSION_IDLE:
        {
            //DEBUG_PRINT("NRF_EVT_RADIO_SESSION_IDLE\r\n");
            
            uint32_t err_code = sd_radio_session_close();
            APP_ERROR_CHECK(err_code);
            break;
        }
        default:
            // No implementation needed.
            break;
    }
}
