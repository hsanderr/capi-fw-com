/**
 * @file app_beacon.h
 * @author Henrique Sander Lourenço (henriquesander27@gmail.com)
 * @brief Main header file of the app_beacon component.
 * @version 0.1
 * @date 2024-03-24
 *
 * @copyright
 *
    Copyright 2024 PetDog

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
 */

#pragma once

#include "esp_err.h"

esp_err_t app_beacon__init(void);
esp_err_t app_beacon__ble_scan_start(void);
esp_err_t app_beacon__ble_scan_stop(void);
void app_beacon__set_auth_mac(uint8_t mac_addr[6]);
