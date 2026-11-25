#include <stdbool.h>
#include <stdint.h>

#include "sdk_config.h"
#include "nrf.h"
#include "app_util_platform.h"
#include "nrf_drv_gpiote.h"
#include "nrf_drv_timer.h"
#include "gpiote.h"

#define BUTTON_SW1_PIN NRF_GPIO_PIN_MAP(1, 6)

#define CONTROL_TICK_MS 10
#define BUTTON_DEBOUNCE_TICKS 3
#define BUTTON_HOLD_MS 500
#define BUTTON_HOLD_TICKS (BUTTON_HOLD_MS / CONTROL_TICK_MS)
#define DOUBLE_CLICK_TICKS (300 / CONTROL_TICK_MS)

static const nrf_drv_timer_t CONTROL_TIMER = NRF_DRV_TIMER_INSTANCE(1);

static volatile uint32_t g_control_ticks = 0;
static volatile uint32_t g_control_tick_pending = 0;

static volatile uint32_t g_button_last_edge_tick = 0;
static volatile uint32_t g_button_press_tick = 0;

static volatile bool g_button_debounced_pressed = false;
static volatile bool g_button_debounce_pending = false;
static volatile uint8_t g_button_stable_ticks = 0;
static volatile bool g_button_hold_started = false;

static volatile uint32_t g_first_click_release_tick = 0;

static volatile bool g_click_waiting = false;
static volatile bool g_second_click_candidate = false;
static volatile bool g_double_click_pending = false;

static void button_event_handler(nrf_drv_gpiote_pin_t pin,
                                 nrf_gpiote_polarity_t action)
{
    if ((pin != BUTTON_SW1_PIN) ||
        (action != NRF_GPIOTE_POLARITY_TOGGLE))
    {
        return;
    }

    CRITICAL_REGION_ENTER();

    g_button_last_edge_tick = g_control_ticks;
    g_button_stable_ticks = 0;
    g_button_debounce_pending = true;

    CRITICAL_REGION_EXIT();

    __SEV();
}

static void button_debounce_tick(void)
{
    bool pressed;

    if (g_button_debounce_pending)
    {
        if (g_button_stable_ticks < BUTTON_DEBOUNCE_TICKS)
            ++g_button_stable_ticks;

        if (g_button_stable_ticks >= BUTTON_DEBOUNCE_TICKS)
        {
            pressed = !nrf_drv_gpiote_in_is_set(BUTTON_SW1_PIN);

            g_button_debounce_pending = false;

            if (pressed != g_button_debounced_pressed)
            {
                g_button_debounced_pressed = pressed;

                if (pressed)
                {
                    g_button_press_tick = g_button_last_edge_tick;
                    g_button_hold_started = false;
                    g_second_click_candidate = false;

                    if (g_click_waiting)
                    {
                        if ((g_button_last_edge_tick -
                            g_first_click_release_tick) <= DOUBLE_CLICK_TICKS)
                        {
                            g_second_click_candidate = true;
                        }
                        else
                        {
                            g_click_waiting = false;
                        }
                    }
                }
                else
                {
                    if (g_button_hold_started)
                    {
                        g_button_hold_started = false;
                        g_click_waiting = false;
                        g_second_click_candidate = false;
                    }
                    else if (g_second_click_candidate)
                    {
                        g_double_click_pending = true;
                        g_click_waiting = false;
                        g_second_click_candidate = false;
                    }
                    else
                    {
                        g_first_click_release_tick = g_button_last_edge_tick;
                        g_click_waiting = true;
                    }
                }
            }
        }
    }

    if (g_button_debounced_pressed &&
        !g_button_debounce_pending &&
        !g_button_hold_started &&
        ((g_control_ticks - g_button_press_tick) >= BUTTON_HOLD_TICKS))
    {
        g_button_hold_started = true;
        g_click_waiting = false;
        g_second_click_candidate = false;
    }

    if (g_click_waiting &&
        !g_button_debounced_pressed &&
        !g_button_debounce_pending &&
        ((g_control_ticks - g_first_click_release_tick) > DOUBLE_CLICK_TICKS))
    {
        g_click_waiting = false;
    }
}

static void control_timer_handler(nrf_timer_event_t event_type,
                                  void *p_context)
{
    (void)p_context;

    if (event_type != NRF_TIMER_EVENT_COMPARE0)
        return;

    CRITICAL_REGION_ENTER();

    ++g_control_ticks;

    button_debounce_tick();

    if (g_control_tick_pending < UINT32_MAX)
        ++g_control_tick_pending;

    CRITICAL_REGION_EXIT();

    __SEV();
}

uint32_t gpiote_init(void)
{
    nrf_drv_timer_config_t timer_config = NRF_DRV_TIMER_DEFAULT_CONFIG;

    nrf_drv_gpiote_in_config_t button_config =
    GPIOTE_CONFIG_IN_SENSE_TOGGLE(true);

    uint32_t err_code;
    uint32_t ticks;

    button_config.pull = NRF_GPIO_PIN_PULLUP;

    err_code = nrf_drv_gpiote_init();

    if ((err_code != NRF_SUCCESS) &&
        (err_code != NRF_ERROR_INVALID_STATE))
    {
        return err_code;
    }

    g_control_ticks = 0;
    g_control_tick_pending = 0;

    g_button_last_edge_tick = 0;
    g_button_press_tick = 0;

    g_button_debounce_pending = false;
    g_button_stable_ticks = 0;
    g_button_hold_started = false;

    g_click_waiting = false;
    g_second_click_candidate = false;
    g_double_click_pending = false;
    g_first_click_release_tick = 0;

    err_code = nrf_drv_gpiote_in_init(
        BUTTON_SW1_PIN,
        &button_config,
        button_event_handler
    );

    if (err_code != NRF_SUCCESS)
        return err_code;

    g_button_debounced_pressed =
    !nrf_drv_gpiote_in_is_set(BUTTON_SW1_PIN);

    if (g_button_debounced_pressed)
        g_button_press_tick = g_control_ticks;

    timer_config.frequency = NRF_TIMER_FREQ_1MHz;
    timer_config.bit_width = NRF_TIMER_BIT_WIDTH_16;
    timer_config.interrupt_priority = 7;

    err_code = nrf_drv_timer_init(
        &CONTROL_TIMER,
        &timer_config,
        control_timer_handler
    );

    if (err_code != NRF_SUCCESS)
        return err_code;

    ticks = nrf_drv_timer_ms_to_ticks(
        &CONTROL_TIMER,
        CONTROL_TICK_MS
    );

    nrf_drv_timer_extended_compare(
        &CONTROL_TIMER,
        NRF_TIMER_CC_CHANNEL0,
        ticks,
        NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK,
        true
    );

    nrf_drv_gpiote_in_event_enable(BUTTON_SW1_PIN, true);
    nrf_drv_timer_enable(&CONTROL_TIMER);

    return NRF_SUCCESS;
}

uint32_t gpiote_take_control_ticks(void)
{
    uint32_t ticks;

    CRITICAL_REGION_ENTER();

    ticks = g_control_tick_pending;
    g_control_tick_pending = 0;

    CRITICAL_REGION_EXIT();

    return ticks;
}

bool gpiote_take_double_click(void)
{
    bool pending;

    CRITICAL_REGION_ENTER();

    pending = g_double_click_pending;
    g_double_click_pending = false;

    CRITICAL_REGION_EXIT();

    return pending;
}

bool gpiote_button_is_pressed(void)
{
    bool pressed;

    CRITICAL_REGION_ENTER();

    pressed = g_button_debounced_pressed;

    CRITICAL_REGION_EXIT();

    return pressed;
}

bool gpiote_button_is_held(void)
{
    bool held;

    CRITICAL_REGION_ENTER();

    held = g_button_hold_started &&
    g_button_debounced_pressed &&
    !g_button_debounce_pending;

    CRITICAL_REGION_EXIT();

    return held;
}
