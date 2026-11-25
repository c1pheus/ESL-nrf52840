#pragma once

#include <stdint.h>

uint32_t pwm_init(void);

void pwm_set_led1(uint8_t duty_percent);

void pwm_set_rgb(uint8_t red_percent,
                 uint8_t green_percent,
                 uint8_t blue_percent);

void pwm_stop(void);

