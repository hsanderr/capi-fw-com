/**
 * @file app_wifi.h
 * @author Henrique Sander Lourenço (henriquesander27@gmail.com)
 * @brief Main header file of the app_wifi component.
 * @version 0.1
 * @date 2024-03-16
 *
 * @copyright Copyright (c) 2024 PetDog
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

esp_err_t app_wifi__init(void);
esp_err_t app_wifi__start(void);
esp_err_t app_wifi__stop(void);
