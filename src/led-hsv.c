#include <stdbool.h>
#include <stdint.h>

#include "sdk_config.h"
#include "nrf.h"
#include "nrf_log.h"
#include "led-hsv.h"
#include "pwm.h"
#include "gpiote.h"
#include "log.h"

#define PARAMETER_STEP_TICKS       20
#define HUE_BLINK_HALF_PERIOD      50
#define SAT_BLINK_HALF_PERIOD      10

static input_mode_t g_input_mode = INPUT_MODE_NONE;
static uint8_t g_parameter_hold_ticks = 0;
static uint8_t g_indicator_ticks = 0;
static bool g_indicator_state = false;

static void fatal_error(void)
{
    pwm_stop();

    while (true)
    {
        log_process();
        __WFE();
    }
}

static void next_input_mode(void)
{
    switch (g_input_mode)
    {
        case INPUT_MODE_NONE:
            g_input_mode = INPUT_MODE_HUE;
            NRF_LOG_INFO("Input mode: Hue");
            break;

        case INPUT_MODE_HUE:
            g_input_mode = INPUT_MODE_SATURATION;
            NRF_LOG_INFO("Input mode: Saturation");
            break;

        case INPUT_MODE_SATURATION:
            g_input_mode = INPUT_MODE_BRIGHTNESS;
            NRF_LOG_INFO("Input mode: Value");
            break;

        case INPUT_MODE_BRIGHTNESS:
            g_input_mode = INPUT_MODE_NONE;
            NRF_LOG_INFO("Input mode: None");
            break;

        default:
            g_input_mode = INPUT_MODE_NONE;
            NRF_LOG_WARNING("Invalid input mode! Please select one");
            break;
    }

    g_parameter_hold_ticks = 0;
    g_indicator_ticks = 0;
    g_indicator_state = false;
}

static void update_led1_indicator(void)
{
    uint8_t half_period;

    if (g_input_mode == INPUT_MODE_NONE)
    {
        g_indicator_ticks = 0;
        g_indicator_state = false;
        pwm_set_led1(0);
    }
    else if (g_input_mode == INPUT_MODE_BRIGHTNESS)
    {
        g_indicator_ticks = 0;
        g_indicator_state = true;
        pwm_set_led1(100);
    }
    else
    {
        if (g_input_mode == INPUT_MODE_HUE)
        {
            half_period = HUE_BLINK_HALF_PERIOD;
        }
        else
        {
            half_period = SAT_BLINK_HALF_PERIOD;
        }

        ++g_indicator_ticks;
        if (g_indicator_ticks >= half_period)
        {
            g_indicator_ticks = 0;
            g_indicator_state = !g_indicator_state;
        }

        pwm_set_led1(g_indicator_state ? 100 : 0);
    }
}

static void apply_hsv(void)
{
    hsv_color_t hsv = hsv_get_current();
    rgb_color_t rgb = hsv_to_rgb(hsv);

    pwm_set_rgb(rgb.red, rgb.green, rgb.blue);
}

static void log_hsv(void)
{
    hsv_color_t hsv = hsv_get_current();

    NRF_LOG_INFO("HSV [%u.%u, %u, %u]",
                 (unsigned int)(hsv.hue_tenths / 10),
                 (unsigned int)(hsv.hue_tenths % 10),
                 (unsigned int)hsv.saturation,
                 (unsigned int)hsv.value);
}
static void process_control_tick(void)
{
    bool parameter_changed = false;

    if (gpiote_button_is_held() && (g_input_mode != INPUT_MODE_NONE))
    {
        ++g_parameter_hold_ticks;

        if (g_parameter_hold_ticks >= PARAMETER_STEP_TICKS)
        {
            g_parameter_hold_ticks = 0;
            hsv_adjust(g_input_mode);
            parameter_changed = true;
        }
    }
    else
    {
        g_parameter_hold_ticks = 0;
    }

    update_led1_indicator();
    apply_hsv();

    if (parameter_changed)
    {
        log_hsv();
    }
}

int main(void)
{
    uint32_t err_code;

    err_code = log_init();
    if (err_code != NRF_SUCCESS)
    {
        while (true)
        {
            __WFE();
        }
    }

    NRF_LOG_INFO("HSV picker started!");

    hsv_init();

    err_code = pwm_init();
    if (err_code != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("nrfx_pwm initialization failed: %u", (unsigned int)err_code);
        fatal_error();
    }

    err_code = gpiote_init();
    if (err_code != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("GPIOTE/timer initialization failed: %u", (unsigned int)err_code);
        fatal_error();
    }

    apply_hsv();
    log_hsv();

    while (true)
    {
        if (gpiote_take_double_click())
        {
            next_input_mode();
        }

        if (gpiote_take_control_ticks())
        {
            process_control_tick();
        }

        log_process();
        __WFE();
    }
}
