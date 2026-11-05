#pragma once
#include <stdint.h>
#define BLINK_DELAY_MS  500
#define PAUSE_DELAY_MS  1000

// #6596
#define LED_1_BLINK_COUNT  6
#define LED_2_BLINK_COUNT  5
#define LED_3_BLINK_COUNT  9
#define LED_4_BLINK_COUNT  6
#define LED_1 NRF_GPIO_PIN_MAP(0, 6)
#define LED_2 NRF_GPIO_PIN_MAP(0, 8)
#define LED_3 NRF_GPIO_PIN_MAP(1, 9)
#define LED_4 NRF_GPIO_PIN_MAP(0, 12)

#define BUTTON_PIN NRF_GPIO_PIN_MAP(1, 6)

size_t blink(uint32_t led, size_t count, size_t current_blink);



