/**
 * @file app_pwm.h
 * @author Henrique Sander Lourenço (henriquesander27@gmail.com)
 * @brief Main header file of the app_pwm component.
 * @version 0.1
 * @date 2024-03-24
 *
 * @copyright Copyright (c) 2024 PetDog
 *
 */

#pragma once

#include "esp_err.h"

esp_err_t app_pwm__init(void);
esp_err_t app_pwm__set_duty_min(void);
esp_err_t app_pwm__set_duty_max(void);
