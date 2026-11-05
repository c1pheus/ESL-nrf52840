/**
 * Copyright (c) 2014 - 2021, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/** @file
 *
 * @defgroup blinky_example_main main.c
 * @{
 * @ingroup blinky_example
 * @brief Blinky Example Application main file.
 *
 * This file contains the source code for a sample application to blink LEDs.
 *
 */

#include <stdbool.h>
#include <stdint.h>
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "gpio-led.h"

/**
 * @brief Function for application main entry.
 */
size_t blink(uint32_t led, size_t count, size_t current_blink)
{
    while (current_blink < count)
    {
        if (nrf_gpio_pin_read(BUTTON_PIN) != 0)
            return current_blink;

        nrf_gpio_pin_clear(led);
        nrf_delay_ms(BLINK_DELAY_MS);

        if (nrf_gpio_pin_read(BUTTON_PIN) != 0)
            return current_blink;

        nrf_gpio_pin_set(led);
        nrf_delay_ms(BLINK_DELAY_MS);

        current_blink++;
    }

    nrf_delay_ms(PAUSE_DELAY_MS);

    return 0;
}

int main(void)
{
    nrf_gpio_cfg_output(LED_1);
    nrf_gpio_cfg_output(LED_2);
    nrf_gpio_cfg_output(LED_3);
    nrf_gpio_cfg_output(LED_4);

    nrf_gpio_pin_set(LED_1);
    nrf_gpio_pin_set(LED_2);
    nrf_gpio_pin_set(LED_3);
    nrf_gpio_pin_set(LED_4);

    nrf_gpio_cfg_input(BUTTON_PIN, NRF_GPIO_PIN_PULLUP);

    size_t current_led = 0;
    size_t current_blink = 0;

    while (true)
    {
        while (nrf_gpio_pin_read(BUTTON_PIN) == 0)
        {
            if (current_led == 0)
                current_blink = blink(LED_1, LED_1_BLINK_COUNT, current_blink);

            if (current_led == 1)
                current_blink = blink(LED_2, LED_2_BLINK_COUNT, current_blink);

            if (current_led == 2)
                current_blink = blink(LED_3, LED_3_BLINK_COUNT, current_blink);

            if (current_led == 3)
                current_blink = blink(LED_4, LED_4_BLINK_COUNT, current_blink);

            if (current_blink == 0)
            {
                current_led++;

                if (current_led == 4)
                    current_led = 0;
            }
        }
    }
}
/**
 *@}
 **/
