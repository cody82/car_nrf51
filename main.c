#include <stdint.h>
#include "nrf.h"
#include "nrf_drv_config.h"
#include "nrf_esb.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_drv_gpiote.h"
#include "nrf_drv_rtc.h"
#include "nrf_adc.h"
//#include "boards.h"
#include "ws2812b-nrf51.h"
#include "app_pwm.h"
#include "app_error.h"
#include "nrf_drv_clock.h"
#include <math.h>

// Define pipe
#define PIPE_NUMBER 0 ///< We use pipe 0 in this example

// GPIO
#define BUTTONS NRF_GPIO_PORT_SELECT_PORT0 ///< GPIO port for reading from buttons
#define LEDS    NRF_GPIO_PORT_SELECT_PORT1 ///< GPIO port for writing to LEDs

// Define payload length
#define TX_PAYLOAD_LENGTH 1 ///< We use 1 byte payload length when transmitting

// Data and acknowledgement payloads
//static uint8_t my_tx_payload[TX_PAYLOAD_LENGTH];                ///< Payload to send to PRX.
static uint8_t my_rx_payload[NRF_ESB_CONST_MAX_PAYLOAD_LENGTH]; ///< Placeholder for received ACK payloads from PRX.

const nrf_drv_rtc_t rtc = //NRF_DRV_RTC_INSTANCE(0); /**< Declaring an instance of nrf_drv_rtc for RTC0. */
{
 .p_reg        = NRF_RTC1,
 .irq          = RTC1_IRQn,
 .instance_id =  1
};

/** @brief Function starting the internal LFCLK XTAL oscillator.
 */
static void lfclk_config(void)
{
    ret_code_t err_code = nrf_drv_clock_init(NULL);
    APP_ERROR_CHECK(err_code);

    nrf_drv_clock_lfclk_request();
}

/** @brief: Function for handling the RTC0 interrupts.
 * Triggered on TICK and COMPARE0 match.
 */
static void rtc_handler(nrf_drv_rtc_int_type_t int_type)
{
}

/** @brief Function initialization and configuration of RTC driver instance.
 */
static void rtc_config(void)
{
    uint32_t err_code;

    //Initialize RTC instance
    err_code = nrf_drv_rtc_init(&rtc, NULL, rtc_handler);
    APP_ERROR_CHECK(err_code);

    //Enable tick event & interrupt
    //nrf_drv_rtc_tick_enable(&rtc,true);

    //Power on RTC instance
    nrf_drv_rtc_enable(&rtc);
}

volatile int32_t BatteryVoltage = 0;//mV
volatile int32_t adc;
volatile int32_t adc2;
volatile int32_t LowVoltage = 0;
void ADC_IRQHandler(void)
{
    nrf_adc_conversion_event_clean();

    adc = nrf_adc_result_get();
    adc2 = (adc * 3600 / 1023);
    BatteryVoltage = adc2 * (1800 + 6800) / 1800;

    if(BatteryVoltage < 6000)
      LowVoltage++;
    else
      LowVoltage = 0;

    // trigger next ADC conversion
    //nrf_adc_start();
}

#ifndef NRF_APP_PRIORITY_HIGH
#define NRF_APP_PRIORITY_HIGH 1
#endif
static void adc_config(void)
{
    const nrf_adc_config_t nrf_adc_config = NRF_ADC_CONFIG_DEFAULT;

    // Initialize and configure ADC
    nrf_adc_configure( (nrf_adc_config_t *)&nrf_adc_config);
    nrf_adc_input_select(NRF_ADC_CONFIG_INPUT_4);
    nrf_adc_int_enable(ADC_INTENSET_END_Enabled << ADC_INTENSET_END_Pos);
    NVIC_SetPriority(ADC_IRQn, NRF_APP_PRIORITY_HIGH);
    NVIC_EnableIRQ(ADC_IRQn);
}

APP_PWM_INSTANCE(PWM1,1);                   // Create the instance "PWM1" using TIMER1.
APP_PWM_INSTANCE(PWM2,0);                   // Create the instance "PWM1" using TIMER1.

static volatile bool ready_flag;            // A flag indicating PWM status.

void pwm_ready_callback(uint32_t pwm_id)    // PWM callback function
{
    ready_flag = true;
}

typedef struct
{
  uint8_t Green;
  uint8_t Red;
  uint8_t Blue;
} Color;

Color strip[21];

const uint32_t Pin_AIN1  = 9;
const uint32_t Pin_AIN2  = 5;//10
const uint32_t Pin_PWMA  = 7;
const uint32_t Pin_BIN1  = 11;
const uint32_t Pin_BIN2  = 12;
const uint32_t Pin_PWMB  = 8;
const uint32_t Pin_LED   = 21;//6
const uint32_t Pin_Servo = 13;
const uint32_t Pin_Beep  = 14;
const uint32_t Pin_Standby = 15;
const uint32_t Pin_FrontTrig = 16;
const uint32_t Pin_FrontEcho  = 17;
const uint32_t Pin_BackTrig = 18;
const uint32_t Pin_BackEcho  = 19;
const uint32_t Pin_LED1   = 6;
const uint32_t Pin_LED2   = 10;

typedef struct
{
  uint16_t crc16;
    //uint8_t xxx;
    uint8_t type;
    int16_t steering;
    int16_t throttle;
    uint8_t front_light;
    uint8_t top_light;
    uint8_t blink_left;
    uint8_t blink_right;
    uint8_t beep;
} Packet;

volatile Packet packet;

void SetMotor(int16_t throttle)
{
  throttle /= 328;
  uint8_t dir = throttle >= 0;
  if(throttle < 0)
    throttle = -throttle;


    app_pwm_channel_duty_set(&PWM1, 0, 100 - throttle);
    app_pwm_channel_duty_set(&PWM1, 1, 100 - throttle);
    //while (app_pwm_channel_duty_set(&PWM1, 0, 100 - throttle) == NRF_ERROR_BUSY);
    //while (app_pwm_channel_duty_set(&PWM1, 1, 100 - throttle) == NRF_ERROR_BUSY);

    if(throttle == 0)
    {
      nrf_gpio_pin_clear(Pin_Standby);
      nrf_gpio_pin_clear(Pin_AIN1);
      nrf_gpio_pin_clear(Pin_AIN2);
      nrf_gpio_pin_clear(Pin_BIN1);
      nrf_gpio_pin_clear(Pin_BIN2);
    }
    else if(dir)
    {
      nrf_gpio_pin_clear(Pin_AIN1);
      nrf_gpio_pin_set(Pin_AIN2);
      nrf_gpio_pin_clear(Pin_BIN1);
      nrf_gpio_pin_set(Pin_BIN2);
      nrf_gpio_pin_set(Pin_Standby);
    }
    else
    {
      nrf_gpio_pin_clear(Pin_AIN2);
      nrf_gpio_pin_set(Pin_AIN1);
      nrf_gpio_pin_clear(Pin_BIN2);
      nrf_gpio_pin_set(Pin_BIN1);
      nrf_gpio_pin_set(Pin_Standby);
    }
}

void SetServo(int16_t steering)
{
  steering /= 66;
  nrf_gpio_pin_set(Pin_Servo);
  nrf_delay_us(1500 + steering);
  nrf_gpio_pin_clear(Pin_Servo);
}

static const uint8_t front_lights[] = {1,2,4,5};
static const uint8_t right_blinkers[] = {0,14};
static const uint8_t left_blinkers[] = {6,20};
static const uint8_t all_blinkers[] = {0,6,14,20};
static const uint8_t break_lights[] = {15,16,18,19};
static const uint8_t back_lights[] = {17};
static const uint8_t top_lights[] = {7,8,9,10,11,12,13};

#define SET_LIGHTS(LIGHTS_ARRAY,R,G,B) SetLights(LIGHTS_ARRAY, sizeof(LIGHTS_ARRAY), (R), (G), (B))
#define SET_LIGHTS_WHITE(LIGHTS_ARRAY,L) SetLights(LIGHTS_ARRAY, sizeof(LIGHTS_ARRAY), (L), (L), (L))
void SetLights(const uint8_t *lights, size_t count, uint8_t r, uint8_t g, uint8_t b)
{
  size_t i;
  for(i=0;i<count;++i)
  {
    Color *c = &strip[lights[i]];
    c->Red = r;
    c->Green = g;
    c->Blue = b;
  }
}

int32_t blink_left = 0;
int32_t blink_right = 0;

void LeftBlinkerOn()
{
  SET_LIGHTS(left_blinkers,150,150,0);
}

void LeftBlinkerOff()
{
  SET_LIGHTS(left_blinkers,0,0,0);
}
void RightBlinkerOn()
{
  SET_LIGHTS(right_blinkers,150,150,0);
}

void RightBlinkerOff()
{
  SET_LIGHTS(right_blinkers,0,0,0);
}


int8_t UltraSoundFront = 0;
int8_t UltraSoundBack = 0;
uint32_t UltraSoundFrontTime = 0;
uint32_t UltraSoundBackTime = 0;
volatile int32_t UltraSoundFrontDist = 0;
volatile int32_t UltraSoundBackDist = 0;

void UltraSoundFrontHandler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
  uint32_t now = nrf_drv_rtc_counter_get(&rtc);
  uint32_t delta;

  //if(now >= UltraSoundFrontTime)
  {
    delta = now - UltraSoundFrontTime;
  }
  //else
  {
    //delta = now + 32768 - UltraSoundFrontTime;
  }

  delta = delta * 17150 / 32768;//cm
  if(delta<150)
  {
    UltraSoundFrontDist = (int32_t)delta;
    //nrf_gpio_pin_clear(Pin_LED1);
  }
  else
  {
    UltraSoundFrontDist = -1;
    //nrf_gpio_pin_set(Pin_LED1);
  }

  //nrf_drv_gpiote_out_toggle(PIN_OUT);
  //nrf_gpio_pin_toggle(Pin_LED2);
}

void UltraSoundBackHandler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
  uint32_t now = nrf_drv_rtc_counter_get(&rtc);
  uint32_t delta;

  //if(now >= UltraSoundBackTime)
  {
    delta = now - UltraSoundBackTime;
  }
  //else
  {
    //delta = now + 32768 - UltraSoundBackTime;
  }

delta-=32768/1000/5;
    delta = delta * 17150 / 32768;//cm
    if(delta<150)
    {
      UltraSoundBackDist = (int32_t)delta;
      //nrf_gpio_pin_clear(Pin_LED1);
    }
    else
    {
      UltraSoundBackDist = -1;
      //nrf_gpio_pin_set(Pin_LED1);
    }

  //nrf_drv_gpiote_out_toggle(PIN_OUT);
  //nrf_gpio_pin_toggle(Pin_LED2);
}

int32_t rc_timeout = 0;

bool RemoteFail()
{
  return (rc_timeout > 100) || (LowVoltage > 100);
}

float top_time = 0.0f;
void TopLights()
{
    top_time+=0.2f;
    for(int i=7;i<14;++i)
    {
      Color *c = &strip[i];
        c->Red = c->Green = 0;
        if(packet.top_light)
            c->Blue = (unsigned char)((sinf(top_time + ((float)(i - 7) * 2.0f)) * 0.5f + 0.5f) * 255.0f);
        else
            c->Blue = 0;
    }
}

void CalcLights()
{
  if(RemoteFail())
  {
    SET_LIGHTS_WHITE(top_lights, 0);
    SET_LIGHTS_WHITE(front_lights, 0);
    SET_LIGHTS_WHITE(back_lights, 0);
  }
  else
  {
  TopLights();
    SET_LIGHTS_WHITE(front_lights, packet.front_light);
      SET_LIGHTS_WHITE(back_lights, packet.throttle < 0 ? 200 : 0);

  }
    //SET_LIGHTS(top_lights,0,0,packet.top_light ? 150 : 0);
    //SET_LIGHTS(left_blinkers,packet.blink_left ? 150 : 0,packet.blink_left ? 150 : 0,0);
    //SET_LIGHTS(right_blinkers,packet.blink_right ? 150 : 0,packet.blink_right ? 150 : 0,0);
}

static uint32_t tick = 0;
/*****************************************************************************/
/**
* @brief Main function.
* @return ANSI required int return type.
*/
/*****************************************************************************/
int main()
{
  ret_code_t err_code;

      lfclk_config();

      rtc_config();
  //NRF_RTC1->PRESCALER = 0;
  //NRF_RTC1->TASKS_START = 1;

  err_code = nrf_drv_gpiote_init();
  APP_ERROR_CHECK(err_code);

adc_config();

  //init LEDs
  nrf_gpio_cfg_output(Pin_LED1);
  nrf_gpio_cfg_output(Pin_LED2);
  nrf_gpio_pin_set(Pin_LED1);
  nrf_gpio_pin_set(Pin_LED2);

  //init lights
  ws2812b_init(Pin_LED);

  // init beeper
  nrf_gpio_cfg_output(Pin_Beep);
  nrf_gpio_pin_clear(Pin_Beep);

  // init motor controller
  nrf_gpio_cfg_output(Pin_Standby);
  nrf_gpio_cfg_output(Pin_AIN1);
  nrf_gpio_cfg_output(Pin_AIN2);
  nrf_gpio_cfg_output(Pin_BIN1);
  nrf_gpio_cfg_output(Pin_BIN2);
  nrf_gpio_pin_clear(Pin_Standby);
  nrf_gpio_pin_clear(Pin_AIN1);
  nrf_gpio_pin_clear(Pin_AIN2);
  nrf_gpio_pin_clear(Pin_BIN1);
  nrf_gpio_pin_clear(Pin_BIN2);
  app_pwm_config_t pwm1_cfg = APP_PWM_DEFAULT_CONFIG_2CH(100L, Pin_PWMA, Pin_PWMB);
  err_code = app_pwm_init(&PWM1,&pwm1_cfg, pwm_ready_callback);
  APP_ERROR_CHECK(err_code);
  app_pwm_enable(&PWM1);
  app_pwm_channel_duty_set(&PWM1, 0, 0);
  app_pwm_channel_duty_set(&PWM1, 1, 0);
  //while (app_pwm_channel_duty_set(&PWM1, 0, 0) == NRF_ERROR_BUSY);
  //while (app_pwm_channel_duty_set(&PWM1, 1, 0) == NRF_ERROR_BUSY);
/*
  app_pwm_config_t pwm2_cfg = APP_PWM_DEFAULT_CONFIG_2CH(100000L, Pin_FrontTrig, Pin_BackTrig);
  err_code = app_pwm_init(&PWM2,&pwm2_cfg, pwm_ready_callback);
  APP_ERROR_CHECK(err_code);
  app_pwm_enable(&PWM2);
  while (app_pwm_channel_duty_set(&PWM2, 0, 10) == NRF_ERROR_BUSY);
  while (app_pwm_channel_duty_set(&PWM2, 1, 10) == NRF_ERROR_BUSY);
*/
  // init servo
  nrf_gpio_cfg_output(Pin_Servo);
  nrf_gpio_pin_clear(Pin_Servo);

  // init ultrasound
  nrf_gpio_cfg_output(Pin_FrontTrig);
  nrf_gpio_pin_clear(Pin_FrontTrig);
  nrf_gpio_cfg_input(Pin_FrontEcho, NRF_GPIO_PIN_NOPULL);
  nrf_gpio_cfg_output(Pin_BackTrig);
  nrf_gpio_pin_clear(Pin_BackTrig);
  nrf_gpio_cfg_input(Pin_BackEcho, NRF_GPIO_PIN_NOPULL);

  nrf_drv_gpiote_in_config_t in_config = GPIOTE_CONFIG_IN_SENSE_HITOLO(true);
  in_config.pull = NRF_GPIO_PIN_NOPULL;//NRF_GPIO_PIN_PULLUP;

  err_code = nrf_drv_gpiote_in_init(Pin_FrontEcho, &in_config, UltraSoundFrontHandler);
  APP_ERROR_CHECK(err_code);

  nrf_drv_gpiote_in_event_enable(Pin_FrontEcho, true);

    err_code = nrf_drv_gpiote_in_init(Pin_BackEcho, &in_config, UltraSoundBackHandler);
    APP_ERROR_CHECK(err_code);

    nrf_drv_gpiote_in_event_enable(Pin_BackEcho, true);


    // Initialize ESB
    (void)nrf_esb_init(NRF_ESB_MODE_PRX);

    nrf_esb_set_datarate(NRF_ESB_DATARATE_1_MBPS);
    nrf_esb_set_channel(2);

    // Setup user interface


    // Load data into TX queue
    //(void)nrf_esb_add_packet_to_tx_fifo(PIPE_NUMBER, my_tx_payload, TX_PAYLOAD_LENGTH, NRF_ESB_PACKET_USE_ACK);

    // Enable ESB to start receiving
    (void)nrf_esb_enable();

    while(1)
    {
      nrf_adc_start();
      rc_timeout++;

      bool blocked_front = (UltraSoundFrontDist >= 0) && (UltraSoundFrontDist < 20);
      bool blocked_back = (UltraSoundBackDist >= 0) && (UltraSoundBackDist < 20);

      CalcLights();

      nrf_esb_disable();
      __disable_irq();
      ws2812b_write(Pin_LED, (uint8_t*)strip, sizeof(strip));
      __enable_irq();

      nrf_esb_enable();

if(!RemoteFail() && ((!blocked_front && packet.throttle >= 0) || (!blocked_back && packet.throttle <= 0)))
{
        SetMotor(packet.throttle);
}
else
{
        SetMotor(0);
}

if(RemoteFail())
{

              if(blink_left == 0)
              {
                  if(blink_right > 0)
                      blink_left = blink_right;
                  else
                      blink_left = 1;
              }

              if(blink_right == 0)
              {
                  if(blink_left > 0)
                      blink_right = blink_left;
                  else
                      blink_right = 1;
              }
}
else
{
                    if(packet.blink_right)
                    {
                        if(blink_right == 0)
                        {
                            if(blink_left > 0)
                                blink_right = blink_left;
                            else
                                blink_right = 1;
                        }
                    }
                    else
                        blink_right = 0;


                    if(packet.blink_left)
                    {
                        if(blink_left == 0)
                        {
                            if(blink_right > 0)
                                blink_left = blink_right;
                            else
                                blink_left = 1;
                        }
                    }
                    else
                        blink_left = 0;
}

        if(!RemoteFail() && packet.beep)
        {
          nrf_gpio_pin_set(Pin_Beep);
        }
        else
        {
          nrf_gpio_pin_clear(Pin_Beep);
        }

        if((tick % 4) == 0)
        {
          if(!UltraSoundFront)
          {
            nrf_gpio_pin_set(Pin_FrontTrig);
          }
          else
          {
            nrf_gpio_pin_clear(Pin_FrontTrig);
            //start timer
            UltraSoundFrontTime = nrf_drv_rtc_counter_get(&rtc);
          }
          UltraSoundFront = !UltraSoundFront;
        }
        else if((tick % 4) == 2)
        {
          if(!UltraSoundBack)
          {
            nrf_gpio_pin_set(Pin_BackTrig);
          }
          else
          {
            nrf_gpio_pin_clear(Pin_BackTrig);
            //start timer
            UltraSoundBackTime = nrf_drv_rtc_counter_get(&rtc);
          }
          UltraSoundBack = !UltraSoundBack;
        }

        if((tick % 3) == 0)
        {
          SetServo(RemoteFail() ? 0 : packet.steering);
        }


                if(tick % 50 == 0)
                {
                    if(blink_left > 0)
                    {
                        blink_left++;
                        if(blink_left % 2 == 0)
                            LeftBlinkerOn();
                        else
                            LeftBlinkerOff();
                    }
                    else
                        LeftBlinkerOff();
                    if(blink_right > 0)
                    {
                        blink_right++;
                        if(blink_right % 2 == 0)
                            RightBlinkerOn();
                        else
                            RightBlinkerOff();
                    }
                    else
                        RightBlinkerOff();
                }

      nrf_delay_ms(10);
      tick++;
      nrf_gpio_pin_toggle(Pin_LED2);
    }
}

void Car_Receive(const Packet *p)
{
  rc_timeout = 0;
  packet = *p;

}
/*****************************************************************************/
/** @name ESB callback function definitions  */
/*****************************************************************************/


void nrf_esb_tx_success(uint32_t tx_pipe, int32_t rssi)
{
    // Read buttons and load ACK payload into TX queue
    //my_tx_payload[0] = input_get();
    //(void)nrf_esb_add_packet_to_tx_fifo(PIPE_NUMBER, my_tx_payload, TX_PAYLOAD_LENGTH, NRF_ESB_PACKET_USE_ACK);
}

void nrf_esb_rx_data_ready(uint32_t rx_pipe, int32_t rssi)
{
    uint32_t my_rx_payload_length;

//t=!t;output_present(t);
    // Fetch packet and write first byte of the payload to the GPIO port.
    (void)nrf_esb_fetch_packet_from_rx_fifo(PIPE_NUMBER, my_rx_payload, &my_rx_payload_length);
    if (my_rx_payload_length > 0)
    {
      if(my_rx_payload_length == sizeof(Packet))
      {
        Packet *p = (Packet*)my_rx_payload;
        //strip[2].Red = strip[2].Green = strip[2].Blue = p->front_light;
        //nrf_gpio_pin_toggle(LED_2);
        Car_Receive(p);
      }

        //output_present(my_rx_payload[0]);
    }
}

// Callbacks not needed in this example.
void nrf_esb_tx_failed(uint32_t tx_pipe) {}
void nrf_esb_disabled(void) {}
