/**
 * @file app_status.c
 * @author Henrique Sander Lourenço (henriquesander27@gmail.com)
 * @brief Contains code to manage product statuses.
 * @version 0.1
 * @date 2024-03-23
 *
 * @copyright Copyright (c) 2024 PetDog

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.'
    See the License for the specific language governing permissions and
    limitations under the License.
 */

#define LOG_LOCAL_LEVEL ESP_LOG_NONE
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "app_status.h"
#include "app_gpio.h"

static void app_status__check_status_task(void *arg);

static const char *TAG = "app_status";                           ///< Tag to be used when logging
static TaskHandle_t app_status__check_status_task_handle = NULL; ///< Statuses check task

/*! @var typedef struct
         {
            uint8_t battery_low : 1;
            uint8_t beacon_battery_low : 1;
         } app_status_t;
    @brief Typedef for indicating general product statuses.
 */
typedef struct
{
    uint8_t battery_low : 1;        ///< Indicate that gateway's battery is low
    uint8_t beacon_battery_low : 1; ///< Indicate that beacon's battery is low
} app_status_t;                     ///< status bla bla bla

app_status_t app_status = {
    .battery_low = 0,
    .beacon_battery_low = 0,
}; ///< General product statuses

/**
 * @brief Initialize app_status component by creating a task to check the relevant statuses.
 *
 * @return esp_err_t
 * @retval ESP_OK if app_status is successfully initialized.
 * @retval ESP_FAIL otherwise.
 */
esp_err_t app_status__init(void)
{
    if (xTaskCreate(app_status__check_status_task,
                    "app_status__check_status_task", 2048, NULL, 10,
                    &app_status__check_status_task_handle) != pdPASS)
    {
        ESP_LOGE(TAG, "Error creating app_status__check_status_task");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Created app_status__check_status_task");
    return ESP_OK;
}

/**
 * @brief Task to keep checking the statuses.
 *
 * @param arg Optional arguments (not being used).
 */
static void app_status__check_status_task(void *arg)
{
    for (;;)
    {
        if (app_status.battery_low && !app_status.beacon_battery_low)
        {
            ESP_LOGW(TAG, "Battery low!");
            app_gpio__blink_red_led_fast(2);
        }
        else if (!app_status.battery_low && app_status.beacon_battery_low)
        {
            ESP_LOGW(TAG, "Beacon battery low!");
            app_gpio__blink_red_led_slow(1);
        }
        else if (app_status.battery_low && app_status.beacon_battery_low)
        {
            ESP_LOGI(TAG, "All batteries low!");
            app_gpio__blink_red_led_fast(2);
            vTaskDelay(250 / portTICK_PERIOD_MS);
            app_gpio__blink_red_led_slow(1);
        }
        else
        {
            ESP_LOGI(TAG, "All batteries ok!");
        }
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

/**
 * @brief Set status of low battery.
 *
 * @param battery_low If > 0, set low battery status to 1, otherwise set it to 0.
 */
void app_status__set_battery_low_status(uint8_t battery_low)
{
    if (battery_low > 0)
    {
        app_status.battery_low = 1;
    }
    else
    {
        app_status.battery_low = 0;
    }
}

/**
 * @brief Set status of beacon low battery.
 *
 * @param battery_low If > 0, set beacon low battery status to 1, otherwise set it to 0.
 */
void app_status__set_beacon_battery_low_status(uint8_t beacon_battery_low)
{
    if (beacon_battery_low > 0)
    {
        app_status.beacon_battery_low = 1;
    }
    else
    {
        app_status.beacon_battery_low = 0;
    }
}
