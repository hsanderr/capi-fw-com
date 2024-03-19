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

#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"
#include "esp_err.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "app_nvs.h"
#include "app_wifi.h"

static const char *TAG = "main"; ///< Tag to be used when logging

void app_error_handling__restart(void)
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
    uint8_t dummy_counter = 0;
    err = app_nvs__init();
    if (err != ESP_OK)
    {
        app_error_handling__restart();
    }
    else
    {
        err = app_nvs__get_data();
        // if (err != ESP_OK)
        if (0)
        {
            app_error_handling__restart();
        }
        else
        {
            err = app_wifi__init();
            if (err != ESP_OK)
            {
                app_error_handling__restart();
            }
            else
            {
                err = app_wifi__start();
                if (err != ESP_OK)
                {
                    app_error_handling__restart();
                }
                else
                {
                    for (;;)
                    {
                        ESP_LOGI(TAG, "I'm alive! Dummy counter = %d", (int)dummy_counter);
                        dummy_counter++;
                        vTaskDelay(60000 / portTICK_PERIOD_MS);
                        err = app_wifi__stop();
                        if (err != ESP_OK)
                        {
                            app_error_handling__restart();
                        }
                        vTaskDelay(10000 / portTICK_PERIOD_MS);
                        err = app_wifi__start();
                        if (err != ESP_OK)
                        {
                            app_error_handling__restart();
                        }
                    }
                }
            }
        }
    }
}
