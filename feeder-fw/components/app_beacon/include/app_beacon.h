/**
 * @file app_beacon.h
 * @author Henrique Sander Lourenço (henriquesander27@gmail.com)
 * @brief Main header file of the app_beacon component.
 * @version 0.1
 * @date 2024-03-24
 *
 * @copyright Copyright (c) 2024 PetDog
 *
 */

#pragma once

#include "esp_err.h"

esp_err_t app_beacon__init(void);
esp_err_t app_beacon__ble_scan_start(void);
esp_err_t app_beacon__ble_scan_stop(void);
void app_beacon__set_auth_mac(uint8_t mac_addr[6]);
