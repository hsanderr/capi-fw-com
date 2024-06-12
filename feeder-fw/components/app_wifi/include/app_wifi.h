/**
 * @file app_wifi.h
 * @author Henrique Sander Lourenço (henriquesander27@gmail.com)
 * @brief Main header file of the app_wifi component.
 * @version 0.1
 * @date 2024-03-16
 *
 * @copyright Copyright (c) 2024 PetDog
 *
 */

#pragma once

#include "esp_err.h"

esp_err_t app_wifi__init(void);
esp_err_t app_wifi__start(void);
esp_err_t app_wifi__stop(void);
