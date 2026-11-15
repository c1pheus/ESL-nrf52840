#include <stdbool.h>
#include <stdint.h>
#include "sdk_config.h"
#include "app_pwm.h"
#include "app_util_platform.h"
#include "nrf.h"
#include "nrf_drv_gpiote.h"
#include "nrf_drv_timer.h"
#include "nrf_gpio.h"


#define LED_LD1_GREEN_PIN NRF_GPIO_PIN_MAP(0, 6)
#define LED_LD2_RED_PIN   NRF_GPIO_PIN_MAP(0, 8)
#define LED_LD2_GREEN_PIN NRF_GPIO_PIN_MAP(1, 9)
#define LED_LD2_BLUE_PIN  NRF_GPIO_PIN_MAP(0, 12)
#define BUTTON_SW1_PIN NRF_GPIO_PIN_MAP(1, 6)

#define PWM_PERIOD_US          1000 //freqency is 1 kHz
#define FADE_STEP_PERCENT      5 //step of PWM value
#define CONTROL_TICK_MS        10
//using Nordic SDK app_pwm library
APP_PWM_INSTANCE(PWM_A, 2);
APP_PWM_INSTANCE(PWM_B, 3);

static app_pwm_config_t pwm_config_a =
    APP_PWM_DEFAULT_CONFIG_2CH(PWM_PERIOD_US,
                               LED_LD1_GREEN_PIN,
                               LED_LD2_RED_PIN);

static app_pwm_config_t pwm_config_b =
    APP_PWM_DEFAULT_CONFIG_2CH(PWM_PERIOD_US,
                               LED_LD2_GREEN_PIN,
                               LED_LD2_BLUE_PIN);


//TIMER1 is for button handler, TIMER2, TIMER3 are for PWM
static const nrf_drv_timer_t CONTROL_TIMER = NRF_DRV_TIMER_INSTANCE(1);

#define DEBOUNCE_TICKS       (50  / CONTROL_TICK_MS)
#define DOUBLE_CLICK_TICKS   (300 / CONTROL_TICK_MS)

//these parameters are used for GPIOTE and TIMER
static volatile uint32_t g_control_ticks = 0;
static volatile uint32_t g_last_press_tick = 0;
static volatile uint32_t g_first_click_tick = 0;
static volatile bool g_has_last_press = false;
static volatile bool g_click_waiting = false;
static volatile bool g_toggle_request = false;
static volatile bool g_fade_tick_pending = false;

//#6596

static const uint8_t DEVICE_ID_SEQUENCE[4] = {6, 5, 9, 6};

static uint8_t current_led = 0;
static uint8_t pulses_remaining = 6;
static uint8_t duty_percent = 0;
static int8_t fade_direction = 1;
static bool sequence_started = false;
static bool sequence_running = false;

static void fatal_error(void)
{
    __disable_irq();
    while (true)
    {
        __WFE();
    }
}

static void pwm_set_channel(app_pwm_t const * p_instance,
                            uint8_t channel,
                            uint8_t duty)
{
    ret_code_t err_code;

    do
    {
        err_code = app_pwm_channel_duty_set(p_instance, channel, duty);
    }
    while (err_code == NRF_ERROR_BUSY);

    if (err_code != NRF_SUCCESS)
    {
        fatal_error();
    }
}

static void led_duty_set(uint8_t led_index, uint8_t duty)
{
    switch (led_index)
    {
        case 0:
            pwm_set_channel(&PWM_A, 0, duty);
            break;

        case 1:
            pwm_set_channel(&PWM_A, 1, duty);
            break;

        case 2:
            pwm_set_channel(&PWM_B, 0, duty);
            break;

        case 3:
            pwm_set_channel(&PWM_B, 1, duty);
            break;

        default:
            fatal_error();
            break;
    }
}

static void sequence_reset(void)
{
    current_led = 0;
    pulses_remaining = DEVICE_ID_SEQUENCE[0];
    duty_percent = 0;
    fade_direction = 1;
    sequence_started = true;

    led_duty_set(current_led, 0);
}

static void sequence_step(void)
{
    if (fade_direction > 0)
    {
        if ((uint16_t)duty_percent + FADE_STEP_PERCENT >= 100U)
        {
            duty_percent = 100;
            fade_direction = -1;
        }
        else
        {
            duty_percent = (uint8_t)(duty_percent + FADE_STEP_PERCENT);
        }
    }
    else
    {
        if (duty_percent <= FADE_STEP_PERCENT)
        {
            duty_percent = 0;

            if (pulses_remaining > 1)
            {
                --pulses_remaining;
            }
            else
            {
               //clear channel
                led_duty_set(current_led, 0);
                current_led = (uint8_t)((current_led + 1) % 4);
                pulses_remaining = DEVICE_ID_SEQUENCE[current_led];
            }

            fade_direction = 1;
        }
        else
        {
            duty_percent = (uint8_t)(duty_percent - FADE_STEP_PERCENT);
        }
    }

    led_duty_set(current_led, duty_percent);
}

static void button_event_handler(nrf_drv_gpiote_pin_t pin,
                                 nrf_gpiote_polarity_t action)
{
    uint32_t now;

    if ((pin != BUTTON_SW1_PIN) || (action != NRF_GPIOTE_POLARITY_HITOLO))
    {
        return;
    }

    now = g_control_ticks;

    if (g_has_last_press &&
        ((now - g_last_press_tick) < DEBOUNCE_TICKS))
    {
        return;
    }

    g_last_press_tick = now;
    g_has_last_press = true;

    if (g_click_waiting &&
        ((now - g_first_click_tick) <= DOUBLE_CLICK_TICKS))
    {
        g_click_waiting = false;
        g_toggle_request = true;
    }
    else
    {
        g_first_click_tick = now;
        g_click_waiting = true;
    }

    __SEV();
}
static void control_timer_handler(nrf_timer_event_t event_type,
                                  void * p_context)
{
    (void)p_context;

    if (event_type != NRF_TIMER_EVENT_COMPARE0)
    {
        return;
    }

    ++g_control_ticks;

    if (g_click_waiting &&
        ((g_control_ticks - g_first_click_tick) > DOUBLE_CLICK_TICKS))
    {
        g_click_waiting = false;
    }
    g_fade_tick_pending = true;
    __SEV();
}

static void button_init(void)
{
    nrf_drv_gpiote_in_config_t button_config =
        GPIOTE_CONFIG_IN_SENSE_HITOLO(true);

    button_config.pull = NRF_GPIO_PIN_PULLUP;

    if (nrf_drv_gpiote_in_init(BUTTON_SW1_PIN,
                               &button_config,
                               button_event_handler) != NRF_SUCCESS)
    {
        fatal_error();
    }

    nrf_drv_gpiote_in_event_enable(BUTTON_SW1_PIN, true);
}

static void control_timer_init(void)
{
    nrf_drv_timer_config_t timer_config = NRF_DRV_TIMER_DEFAULT_CONFIG;
    uint32_t ticks;

    timer_config.frequency = NRF_TIMER_FREQ_1MHz;
    timer_config.bit_width = NRF_TIMER_BIT_WIDTH_16;
    timer_config.interrupt_priority = 7;

    if (nrf_drv_timer_init(&CONTROL_TIMER,
                           &timer_config,
                           control_timer_handler) != NRF_SUCCESS)
    {
        fatal_error();
    }

    ticks = nrf_drv_timer_ms_to_ticks(&CONTROL_TIMER, CONTROL_TICK_MS);

    nrf_drv_timer_extended_compare(&CONTROL_TIMER,
                                   NRF_TIMER_CC_CHANNEL0,
                                   ticks,
                                   NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK,
                                   true);

    nrf_drv_timer_enable(&CONTROL_TIMER);
}

int main(void)
{
    bool toggle_event;
    bool fade_event;

    if (app_pwm_init(&PWM_A, &pwm_config_a, NULL) != NRF_SUCCESS)
    {
        fatal_error();
    }

    if (app_pwm_init(&PWM_B, &pwm_config_b, NULL) != NRF_SUCCESS)
    {
        fatal_error();
    }

    app_pwm_enable(&PWM_A);
    app_pwm_enable(&PWM_B);
    button_init();
    control_timer_init();

    while (true)
    {
        CRITICAL_REGION_ENTER();
        toggle_event = g_toggle_request;
        g_toggle_request = false;
        fade_event = g_fade_tick_pending;
        g_fade_tick_pending = false;
        CRITICAL_REGION_EXIT();

        if (toggle_event)
        {
            if (sequence_running)
            {
                sequence_running = false;
            }
            else
            {
                if (!sequence_started)
                {
                    sequence_reset();
                }

                // Resume from the exact duty where it was paused
                sequence_running = true;
            }
        }

        if (fade_event && sequence_running)
        {
            sequence_step();
        }

        __WFE();
    }
}
