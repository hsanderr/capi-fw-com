/**
 * @file app_gpio.h
 * @author Henrique Sander Lourenço (henriquesander27@gmail.com)
 * @brief Main header file of the app_gpio component.
 * @version 0.1
 * @date 2024-03-20
 *
 * @copyright Copyright (c) 2024 PetDog
 *
 */

#pragma once

#include "esp_err.h"

esp_err_t app_gpio__init(void);
esp_err_t app_gpio__blink_blue_led_slow(uint8_t times);
esp_err_t app_gpio__blink_blue_led_fast(uint8_t times);
esp_err_t app_gpio__blink_red_led_slow(uint8_t times);
esp_err_t app_gpio__blink_red_led_fast(uint8_t times);
