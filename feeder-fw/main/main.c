/**
 * @file main.c
 * @author Henrique Sander Lourenço (henriquesander27@gmail.com)
 * @brief Contains the entry point of the program.
 * @version 0.1
 * @date 2024-03-16
 *
 * @copyright Copyright (c) 2024
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

#define LOG_LOCAL_LEVEL ESP_LOG_NONE
#include "esp_log.h"
#include "esp_err.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "app_nvs.h"
#include "app_wifi.h"
#include "app_gpio.h"
#include "app_measure_vcc.h"
#include "app_status.h"
#include "app_pwm.h"
#include "app_beacon.h"

static const char *TAG = "main"; ///< Tag to be used when logging

static void app_error_handling__restart(void)
{
    uint8_t reboot_delay_sec = 3;
    ESP_LOGE(TAG, "Fatal error found, rebooting in %d seconds..", (int)reboot_delay_sec);
    vTaskDelay((1000 * reboot_delay_sec) / portTICK_PERIOD_MS);
    esp_restart();
}

void app_main(void)
{
    ESP_LOGI(TAG, "Hello World!");
    esp_err_t err;
    err = app_nvs__init();
    if (err != ESP_OK)
    {
        app_error_handling__restart();
    }
    err = app_nvs__get_data();
    if (err != ESP_OK && err != ESP_ERR_NOT_FOUND)
    {
        app_error_handling__restart();
    }
    err = app_wifi__init();
    if (err != ESP_OK)
    {
        app_error_handling__restart();
    }
    err = app_gpio__init();
    if (err != ESP_OK)
    {
        app_error_handling__restart();
    }
    app_gpio__blink_blue_led_slow(1);
    err = app_measure_vcc__init();
    if (err != ESP_OK)
    {
        app_error_handling__restart();
    }
    err = app_status__init();
    if (err != ESP_OK)
    {
        app_error_handling__restart();
    }
    err = app_pwm__init();
    if (err != ESP_OK)
    {
        app_error_handling__restart();
    }
    // app_pwm__set_duty_max();
    // app_pwm__set_duty_min();
    err = app_beacon__init();
    if (err != ESP_OK)
    {
        app_error_handling__restart();
    }
}
