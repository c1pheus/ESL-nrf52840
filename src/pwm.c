#include <stdbool.h>
#include <stdint.h>

#include "sdk_config.h"
#include "app_util_platform.h"
#include "nrfx_pwm.h"
#include "nrf_gpio.h"
#include "pwm.h"


#define LED_LD1_GREEN_PIN    NRF_GPIO_PIN_MAP(0, 6)

#define LED_LD2_RED_PIN      NRF_GPIO_PIN_MAP(0, 8)
#define LED_LD2_GREEN_PIN    NRF_GPIO_PIN_MAP(1, 9)
#define LED_LD2_BLUE_PIN     NRF_GPIO_PIN_MAP(0, 12)


#define PWM_TOP_VALUE        1000

#define PWM_SEQUENCE_LENGTH \
(sizeof(nrf_pwm_values_individual_t) / sizeof(uint16_t))


static nrfx_pwm_t m_pwm = NRFX_PWM_INSTANCE(0);


typedef struct
{
    uint8_t led1;
    uint8_t red;
    uint8_t green;
    uint8_t blue;

} pwm_levels_t;


static volatile pwm_levels_t m_pwm_requested = {0};

static nrf_pwm_values_individual_t m_pwm_values[2];


static nrf_pwm_sequence_t m_pwm_sequence[2];


static bool m_pwm_initialized = false;



static uint16_t percent_to_pwm(uint8_t percent)
{
    if (percent > 100)
    {
        percent = 100;
    }

    return (uint16_t)(
        ((uint32_t)percent * PWM_TOP_VALUE) / 100
    );
}


static void sequence_values_update(uint8_t sequence)
{
    m_pwm_values[sequence].channel_0 =
    percent_to_pwm(m_pwm_requested.led1);

    m_pwm_values[sequence].channel_1 =
    percent_to_pwm(m_pwm_requested.red);

    m_pwm_values[sequence].channel_2 =
    percent_to_pwm(m_pwm_requested.green);

    m_pwm_values[sequence].channel_3 =
    percent_to_pwm(m_pwm_requested.blue);
}

static void pwm_event_handler(nrfx_pwm_evt_type_t event_type)
{
    switch (event_type)
    {
        case NRFX_PWM_EVT_END_SEQ0:
            sequence_values_update(0);
            break;

        case NRFX_PWM_EVT_END_SEQ1:
            sequence_values_update(1);
            break;

        default:
            break;
    }
}


uint32_t pwm_init(void)
{
    nrfx_pwm_config_t config = NRFX_PWM_DEFAULT_CONFIG;

    uint32_t err_code;

    m_pwm_requested.led1 = 0;
    m_pwm_requested.red = 0;
    m_pwm_requested.green = 0;
    m_pwm_requested.blue = 0;
    config.output_pins[0] =
    LED_LD1_GREEN_PIN | NRFX_PWM_PIN_INVERTED;

    config.output_pins[1] =
    LED_LD2_RED_PIN | NRFX_PWM_PIN_INVERTED;

    config.output_pins[2] =
    LED_LD2_GREEN_PIN | NRFX_PWM_PIN_INVERTED;

    config.output_pins[3] =
    LED_LD2_BLUE_PIN | NRFX_PWM_PIN_INVERTED;


    config.base_clock = NRF_PWM_CLK_1MHz;
    config.count_mode = NRF_PWM_MODE_UP;
    config.top_value = PWM_TOP_VALUE;

    config.load_mode = NRF_PWM_LOAD_INDIVIDUAL;
    config.step_mode = NRF_PWM_STEP_AUTO;


    err_code = nrfx_pwm_init(
        &m_pwm,
        &config,
        pwm_event_handler
    );

    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }


//EasyDMA buffers
    sequence_values_update(0);
    sequence_values_update(1);


 //two sequences!
    for (uint8_t i = 0; i < 2; ++i)
    {
        m_pwm_sequence[i].values.p_individual =
        &m_pwm_values[i];

        m_pwm_sequence[i].length =
        PWM_SEQUENCE_LENGTH;

        m_pwm_sequence[i].repeats = 0;
        m_pwm_sequence[i].end_delay = 0;
    }


    m_pwm_initialized = true;


//callbacks here
    (void)nrfx_pwm_complex_playback(
        &m_pwm,
        &m_pwm_sequence[0],
        &m_pwm_sequence[1],
        1,
        NRFX_PWM_FLAG_LOOP |
        NRFX_PWM_FLAG_SIGNAL_END_SEQ0 |
        NRFX_PWM_FLAG_SIGNAL_END_SEQ1 |
        NRFX_PWM_FLAG_NO_EVT_FINISHED
    );

    return NRF_SUCCESS;
}
void pwm_set_led1(uint8_t duty_percent)
{
    if (!m_pwm_initialized)
    {
        return;
    }

    CRITICAL_REGION_ENTER();

    m_pwm_requested.led1 = duty_percent;

    CRITICAL_REGION_EXIT();
}
void pwm_set_rgb(uint8_t red_percent,
                 uint8_t green_percent,
                 uint8_t blue_percent)
{
    if (!m_pwm_initialized)
    {
        return;
    }

    CRITICAL_REGION_ENTER();

    m_pwm_requested.red = red_percent;
    m_pwm_requested.green = green_percent;
    m_pwm_requested.blue = blue_percent;

    CRITICAL_REGION_EXIT();
}
void pwm_stop(void)
{
    if (m_pwm_initialized)
    {
        (void)nrfx_pwm_stop(&m_pwm, true);

        m_pwm_initialized = false;
    }
}
