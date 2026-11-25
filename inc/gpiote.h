#pragma once

#include <stdbool.h>
#include <stdint.h>

uint32_t gpiote_init(void);

bool gpiote_button_is_held(void);

uint32_t gpiote_take_control_ticks(void);

bool gpiote_take_double_click(void);

bool gpiote_button_is_pressed(void);

