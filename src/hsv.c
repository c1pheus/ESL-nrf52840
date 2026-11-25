#include <stdbool.h>
#include <stdint.h>

#include "led-hsv.h"

#define HSV_TABLE_LENGTH             101
#define HSV_MAX_INDEX                (HSV_TABLE_LENGTH - 1)

#define DEFAULT_SATURATION_PERCENT   100
#define DEFAULT_VALUE_PERCENT        100

static const uint8_t DEVICE_ID_SEQUENCE[4] = {6, 5, 9, 6};

//current HSV indexes
static uint8_t g_hue_index = 0;
static uint8_t g_saturation_index = DEFAULT_SATURATION_PERCENT;
static uint8_t g_value_index = DEFAULT_VALUE_PERCENT;

//routes for change
static bool g_hue_increasing = true;
static bool g_saturation_increasing = false;
static bool g_value_increasing = false;


//if max, then decrease (changing route)
//if min, then increase
static void adjust_index(uint8_t *index, bool *increasing)
{
    if (*increasing)
    {
        if (*index < HSV_MAX_INDEX)
        {
            ++(*index);
        }

        if (*index >= HSV_MAX_INDEX)
        {
            *increasing = false;
        }
    }
    else
    {
        if (*index > 0)
        {
            --(*index);
        }

        if (*index == 0)
        {
            *increasing = true;
        }
    }
}


void hsv_init(void)
{
    g_hue_index = (uint8_t)(
        DEVICE_ID_SEQUENCE[2] * 10 + DEVICE_ID_SEQUENCE[3]);

    if (g_hue_index > HSV_MAX_INDEX)
    {
        g_hue_index = HSV_MAX_INDEX;
    }

    g_saturation_index = DEFAULT_SATURATION_PERCENT;
    g_value_index = DEFAULT_VALUE_PERCENT;

//If parameter max, then it's decreasing
    g_hue_increasing = (g_hue_index < HSV_MAX_INDEX);

    g_saturation_increasing =
    (g_saturation_index < HSV_MAX_INDEX);

    g_value_increasing =
    (g_value_index < HSV_MAX_INDEX);
}


void hsv_adjust(input_mode_t mode)
{
    switch (mode)
    {
        case INPUT_MODE_HUE:
            adjust_index(
                &g_hue_index,
                &g_hue_increasing
            );
            break;

        case INPUT_MODE_SATURATION:
            adjust_index(
                &g_saturation_index,
                &g_saturation_increasing
            );
            break;

        case INPUT_MODE_BRIGHTNESS:
            adjust_index(
                &g_value_index,
                &g_value_increasing
            );
            break;

        case INPUT_MODE_NONE:
        default:
            break;
    }
}


hsv_color_t hsv_get_current(void)
{
    hsv_color_t color;

    color.hue_tenths = (uint16_t)g_hue_index * 36;
    color.saturation = g_saturation_index;
    color.value = g_value_index;

    return color;
}


rgb_color_t hsv_to_rgb(hsv_color_t color)
{
    rgb_color_t rgb;

    uint16_t hue = color.hue_tenths;
    uint8_t saturation = color.saturation;
    uint8_t value = color.value;

    uint8_t region;
    uint8_t fraction;

    uint8_t p;
    uint8_t q;
    uint8_t t;

    if (hue >= 3600)
    {
        hue = 0;
    }

    if (saturation == 0)
    {
        rgb.red = value;
        rgb.green = value;
        rgb.blue = value;

        return rgb;
    }

    region = (uint8_t)(hue / 600);

    fraction = (uint8_t)(
        ((hue - ((uint16_t)region * 600)) * 100) / 600
    );

    p = (uint8_t)(
        ((uint16_t)value * (100 - saturation)) / 100
    );

    q = (uint8_t)(
        ((uint16_t)value *
        (100 - (((uint16_t)saturation * fraction) / 100))) / 100
    );

    t = (uint8_t)(
        ((uint16_t)value *
        (100 - (((uint16_t)saturation * (100- fraction)) / 100))) / 100
    );

    switch (region)
    {
        case 0:
            rgb.red = value;
            rgb.green = t;
            rgb.blue = p;
            break;

        case 1:
            rgb.red = q;
            rgb.green = value;
            rgb.blue = p;
            break;

        case 2:
            rgb.red = p;
            rgb.green = value;
            rgb.blue = t;
            break;

        case 3:
            rgb.red = p;
            rgb.green = q;
            rgb.blue = value;
            break;

        case 4:
            rgb.red = t;
            rgb.green = p;
            rgb.blue = value;
            break;

        default:
            rgb.red = value;
            rgb.green = p;
            rgb.blue = q;
            break;
    }

    return rgb;
}
