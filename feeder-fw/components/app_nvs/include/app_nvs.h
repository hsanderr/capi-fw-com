/**
 * @file app_nvs.h
 * @author Henrique Sander Lourenço (henriquesander27@gmail.com)
 * @brief Main header file of the app_nvs component.
 * @version 0.1
 * @date 2024-03-16
 *
 * @copyright Copyright (c) 2024 PetDog
 *
 */

#pragma once

#include "esp_err.h"

esp_err_t app_nvs__init(void);
esp_err_t app_nvs__get_data(void);
esp_err_t app_nvs__set_authorized_mac(uint8_t authorized_mac[6]);
esp_err_t app_nvs__get_authorized_mac(uint8_t authorized_mac[6]);