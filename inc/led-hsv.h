#pragma once

#include <stdint.h>

typedef enum
{
    INPUT_MODE_NONE = 0,
    INPUT_MODE_HUE,
    INPUT_MODE_SATURATION,
    INPUT_MODE_BRIGHTNESS
} input_mode_t;

typedef struct
{
    uint16_t hue_tenths;
    uint8_t saturation;
    uint8_t value;
} hsv_color_t;

typedef struct
{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} rgb_color_t;

void hsv_init(void);

void hsv_adjust(input_mode_t mode);

hsv_color_t hsv_get_current(void);

rgb_color_t hsv_to_rgb(hsv_color_t color);

