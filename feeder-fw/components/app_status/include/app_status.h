/**
 * @file app_status.h
 * @author Henrique Sander Lourenço (henriquesander27@gmail.com)
 * @brief Main header file of the app_status component.
 * @version 0.1
 * @date 2024-03-23
 *
 * @copyright Copyright (c) 2024 PetDog
 *
 */

#pragma once

#include "esp_err.h"

esp_err_t app_status__init(void);
void app_status__set_battery_low_status(uint8_t battery_low);
void app_status__set_beacon_battery_low_status(uint8_t beacon_battery_low);
