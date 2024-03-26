/**
 * @file app_wifi.c
 * @author Henrique Sander Lourenço (henriquesander27@gmail.com)
 * @brief Contains Wi-Fi-related code.
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

#include <string.h>
#include <stdio.h>

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"
#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_mac.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include "app_wifi.h"
#include "app_web_server.h"
#include "app_gpio.h"

#define ESP_WIFI_AP_SSID "PetDog ComeInt"
#define ESP_WIFI_AP_CHANNEL 1
#define ESP_WIFI_AP_PWD "Senha12345"
#define ESP_WIFI_MAX_CONN_TO_AP 1

static const char *TAG = "app_wifi"; ///< Tag to be used when logging

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

/*! @var typedef enum wifi_status
        {
            WIFI_OFF,
            WIFI_ON
        } wifi_status_t;
    @brief Typedef for indicating Wi-Fi status.
 */
typedef enum wifi_status
{
    WIFI_OFF,
    WIFI_ON
} wifi_status_t;

static wifi_status_t wifi_status = WIFI_OFF;                 ///< Wi-Fi status
static uint8_t wifi_timer_reset = 0;                         ///< Wi-Fi timer reset flag
static TaskHandle_t app_wifi__wifi_timer_task_handle = NULL; ///< Wi-Fi timer task handle

static void app_wifi__wifi_timer_task(void *arg)
{
    for (;;)
    {
        for (uint8_t i = 0; i < 180; i++)
        {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            if (wifi_timer_reset)
            {
                i = 0;
                wifi_timer_reset = 0;
            }
            ESP_LOGI(TAG, "Wi-Fi stopping in %d seconds", 60 - i);
        }
        app_wifi__stop();
        vTaskSuspend(NULL);
    }
    vTaskDelete(NULL);
}

/**
 * @brief Initialize Wi-Fi.
 *
 * @return esp_err_t
 * @retval ESP_OK if Wi-Fi i successfully initialized.
 * @retval ESP_FAIL otherwise.
 */
esp_err_t app_wifi__init(void)
{
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = ESP_WIFI_AP_SSID,
            .channel = ESP_WIFI_AP_CHANNEL,
            .password = ESP_WIFI_AP_PWD,
            .max_connection = ESP_WIFI_MAX_CONN_TO_AP,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK},
    };
    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t err = esp_netif_init();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error initializing TCP/IP stack");
        return ESP_FAIL;
    }
    else
    {
        err = esp_event_loop_create_default();
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error %d creating default event loop: %s", err, esp_err_to_name(err));
            return ESP_FAIL;
        }
        else
        {
            esp_netif_create_default_wifi_ap();

            err = esp_event_handler_instance_register(WIFI_EVENT,
                                                      ESP_EVENT_ANY_ID,
                                                      (esp_event_handler_t)&wifi_event_handler,
                                                      NULL, NULL);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "Error %d registering event handler: %s", err, esp_err_to_name(err));
                return ESP_FAIL;
            }
            else
            {
                err = esp_wifi_init(&wifi_init_config);
                if (err != ESP_OK)
                {
                    ESP_LOGE(TAG, "Error %d registering initializing ESP Wi-Fi: %s", err, esp_err_to_name(err));
                    return ESP_FAIL;
                }
                else
                {
                    err = esp_wifi_set_mode(WIFI_MODE_AP);
                    if (err != ESP_OK)
                    {
                        ESP_LOGE(TAG, "Error %d setting Wi-Fi mode to AP: %s", err, esp_err_to_name(err));
                        return ESP_FAIL;
                    }
                    else
                    {

                        if (strlen(ESP_WIFI_AP_PWD) == 0)
                        {
                            wifi_config.ap.authmode = WIFI_AUTH_OPEN;
                        }
                        err = esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
                        if (err != ESP_OK)
                        {
                            ESP_LOGE(TAG, "Error %d setting Wi-Fi configuration: %s", err, esp_err_to_name(err));
                            return ESP_FAIL;
                        }

                        if (xTaskCreate(app_wifi__wifi_timer_task,
                                        "app_wifi__wifi_timer_task", 2048, NULL, 10,
                                        &app_wifi__wifi_timer_task_handle) != pdPASS)
                        {
                            ESP_LOGE(TAG, "Error creating app_wifi__wifi_timer_task");
                            return ESP_FAIL;
                        }
                        vTaskSuspend(app_wifi__wifi_timer_task_handle);
                        ESP_LOGI(TAG, "Created app_wifi__wifi_timer_task");

                        ESP_LOGI(TAG, "Success initializing Wi-Fi!");
                        return ESP_OK;
                    }
                }
            }
        }
    }
}

/**
 * @brief Start Wi-Fi.
 *
 * @return esp_err_t
 * @retval ESP_OK if Wi-Fi is successfully started or it's already started.
 * @retval ESP_FAIL otherwise.
 */
esp_err_t app_wifi__start(void)
{
    esp_err_t err;
    if (wifi_status != WIFI_ON)
    {
        err = esp_wifi_start();
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error %d starting Wi-Fi: %s", err, esp_err_to_name(err));
            return ESP_FAIL;
        }
        else
        {
            ESP_LOGI(TAG, "Wi-Fi started!");
            wifi_status = WIFI_ON;
            app_gpio__blink_blue_led_slow(2);
            vTaskResume(app_wifi__wifi_timer_task_handle);
            return ESP_OK;
        }
    }
    else
    {
        ESP_LOGI(TAG, "Wi-Fi already started");
        wifi_timer_reset = 1;
        return ESP_OK;
    }
}

/**
 * @brief Stop Wi-Fi.
 *
 * @return esp_err_t
 * @retval ESP_OK if Wi-Fi is successfully stopped.
 * @retval ESP_FAIL otherwise.
 */
esp_err_t app_wifi__stop(void)
{
    esp_err_t err;
    if (wifi_status != WIFI_OFF)
    {
        err = esp_wifi_stop();
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error %d stopping Wi-Fi: %s", err, esp_err_to_name(err));
            return ESP_FAIL;
        }
        else
        {
            ESP_LOGI(TAG, "Wi-Fi stopped");
            wifi_status = WIFI_OFF;
            wifi_timer_reset = 1;
            app_gpio__blink_blue_led_fast(2);
            return ESP_OK;
        }
    }
    else
    {
        ESP_LOGI(TAG, "Wi-Fi already stopped");
        return ESP_OK;
    }
}

/**
 * @brief Wi-Fi event handler, triggered on connection/disconnection to AP.
 *
 * @param arg Optional additional arguments passed when some event happens.
 * @param event_base Base ID of the event.
 * @param event_id Event ID.
 * @param event_data Event data.
 */
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " joined, AID: %d",
                 MAC2STR(event->mac), event->aid);
        app_web_server__start();
    }
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " left, AID: %d",
                 MAC2STR(event->mac), event->aid);
        app_web_server__stop();
    }
}