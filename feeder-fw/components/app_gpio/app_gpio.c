/**
 * @file app_web_server.c
 * @author Henrique Sander Lourenço (henriquesander27@gmail.com)
 * @brief Contains web GPIO-related code (buttons and LEDs).
 * @version 0.1
 * @date 2024-03-20
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

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "app_gpio.h"
#include "app_wifi.h"

#define GPIO_GREEN_LED (2)                                                                        ///< Green LED GPIO
#define GPIO_RED_LED (3)                                                                          ///< Red LED GPIO
#define GPIO_BUTTON (4)                                                                           ///< Button GPIO
#define GPIO_OUTPUT_PIN_SEL ((((uint64_t)1) << GPIO_GREEN_LED) | (((uint64_t)1) << GPIO_RED_LED)) ///< LEDs pin mask
#define GPIO_INPUT_PIN_SEL (((uint64_t)1) << GPIO_BUTTON)                                         ///< Button pin mask
#define BUTTON_HOLD_TIME_SECS (3)

static const char *TAG = "app_gpio";                           ///< Tag to be used when logging
static TaskHandle_t app_gpio__check_button_task_handle = NULL; ///< Button state check task handle
static uint8_t button_pressed = 0;                             ///< Button state, 1 if pressed, 0 otherwise

static void IRAM_ATTR app_gpio__isr_handler(void *arg);
static void app_gpio__check_button_task(void *arg);

static void app_error_handling__restart(void)
{
    uint8_t reboot_delay_sec = 3;
    ESP_LOGE(TAG, "Fatal error found, rebooting in %d seconds..", (int)reboot_delay_sec);
    vTaskDelay((1000 * reboot_delay_sec) / portTICK_PERIOD_MS);
    esp_restart();
}

/**
 * @brief Initialize GPIOs.
 *
 * @return esp_err_t
 * @retval ESP_OK if GPIOs are successfully initialized.
 * @retval ESP_FAIL otherwise.
 */
esp_err_t app_gpio__init(void)
{
    gpio_config_t config = {
        .pin_bit_mask = GPIO_INPUT_PIN_SEL,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};
    esp_err_t err = gpio_config(&config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error %d configuring input GPIOs: %s", err, esp_err_to_name(err));
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Success configuring input GPIOs!");

    config.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    config.mode = GPIO_MODE_OUTPUT;
    config.pull_up_en = GPIO_PULLUP_DISABLE;
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    config.intr_type = GPIO_INTR_POSEDGE;
    err = gpio_config(&config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error %d configuring output GPIOs: %s", err, esp_err_to_name(err));
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Success configuring output GPIOs!");

    gpio_set_level(GPIO_RED_LED, 0);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error %d setting red LED GPIO to low: %s", err, esp_err_to_name(err));
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Set red LED GPIO to low");

    gpio_set_level(GPIO_GREEN_LED, 0);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error %d setting green LED GPIO to low: %s", err, esp_err_to_name(err));
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Set green LED GPIO to low");

    if (xTaskCreate(app_gpio__check_button_task,
                    "app_gpio__check_button_task", 2048, NULL, 10,
                    &app_gpio__check_button_task_handle) != pdPASS)
    {
        ESP_LOGE(TAG, "Error creating app_gpio__check_button_task");
        return ESP_FAIL;
    }
    vTaskSuspend(app_gpio__check_button_task_handle);

    return ESP_OK;
}

/**
 * @brief Slowly blink green LED for the specified number of times.
 *
 * @param times Number of times to blink the LED.
 * @return esp_err_t
 * @retval ESP_OK if LED is successfully blinked.
 * @retval ESP_FAIL otherwise.
 */
esp_err_t app_gpio__blink_green_led_slow(uint8_t times)
{
    for (uint8_t i = 0; i < times; i++)
    {
        esp_err_t err = gpio_set_level(GPIO_GREEN_LED, 1);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error %d setting green LED GPIO to high: %s", err, esp_err_to_name(err));
            return ESP_FAIL;
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
        gpio_set_level(GPIO_GREEN_LED, 0);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error %d setting green LED GPIO to low: %s", err, esp_err_to_name(err));
            return ESP_FAIL;
        }
        if (i != times - 1)
        {
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
    }
    return ESP_OK;
}

/**
 * @brief Fastly blink green LED for the specified number of times.
 *
 * @param times Number of times to blink the LED.
 * @return esp_err_t
 * @retval ESP_OK if LED is successfully blinked.
 * @retval ESP_FAIL otherwise.
 */
esp_err_t app_gpio__blink_green_led_fast(uint8_t times)
{
    for (uint8_t i = 0; i < times; i++)
    {
        esp_err_t err = gpio_set_level(GPIO_GREEN_LED, 1);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error %d setting green LED GPIO to high: %s", err, esp_err_to_name(err));
            return ESP_FAIL;
        }
        vTaskDelay(pdMS_TO_TICKS(250));
        gpio_set_level(GPIO_GREEN_LED, 0);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error %d setting green LED GPIO to low: %s", err, esp_err_to_name(err));
            return ESP_FAIL;
        }
        if (i != times - 1)
        {
            vTaskDelay(pdMS_TO_TICKS(250));
        }
    }
    return ESP_OK;
}

/**
 * @brief Slowly blink red LED for the specified number of times.
 *
 * @param times Number of times to blink the LED.
 * @return esp_err_t
 * @retval ESP_OK if LED is successfully blinked.
 * @retval ESP_FAIL otherwise.
 */
esp_err_t app_gpio__blink_red_led_slow(uint8_t times)
{
    for (uint8_t i = 0; i < times; i++)
    {
        esp_err_t err = gpio_set_level(GPIO_RED_LED, 1);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error %d setting red LED GPIO to high: %s", err, esp_err_to_name(err));
            return ESP_FAIL;
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
        gpio_set_level(GPIO_RED_LED, 0);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error %d setting red LED GPIO to low: %s", err, esp_err_to_name(err));
            return ESP_FAIL;
        }
        if (i != times - 1)
        {
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
    }
    return ESP_OK;
}

/**
 * @brief Fastly blink red LED for the specified number of times.
 *
 * @param times Number of times to blink the LED.
 * @return esp_err_t
 * @retval ESP_OK if LED is successfully blinked.
 * @retval ESP_FAIL otherwise.
 */
esp_err_t app_gpio__blink_red_led_fast(uint8_t times)
{
    for (uint8_t i = 0; i < times; i++)
    {
        esp_err_t err = gpio_set_level(GPIO_RED_LED, 1);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error %d setting red LED GPIO to high: %s", err, esp_err_to_name(err));
            return ESP_FAIL;
        }
        vTaskDelay(pdMS_TO_TICKS(250));
        gpio_set_level(GPIO_RED_LED, 0);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error %d setting red LED GPIO to low: %s", err, esp_err_to_name(err));
            return ESP_FAIL;
        }
        if (i != times - 1)
        {
            vTaskDelay(pdMS_TO_TICKS(250));
        }
    }
    return ESP_OK;
}

static void IRAM_ATTR app_gpio__isr_handler(void *arg)
{
    if (!button_pressed)
    {
        button_pressed = 1;
        vTaskResume(app_gpio__check_button_task_handle);
    }
}

static void app_gpio__check_button_task(void *arg)
{
    uint16_t button_hold_time_miliseconds = 0;
    uint8_t time_increment_miliseconds = 50;
    esp_err_t err;
    for (;;)
    {
        if (gpio_get_level(GPIO_BUTTON) != 0)
        {
            button_hold_time_miliseconds = 0;
            button_pressed = 0;
            vTaskSuspend(NULL);
        }
        else
        {
            vTaskDelay(time_increment_miliseconds / portTICK_PERIOD_MS);
            button_hold_time_miliseconds += time_increment_miliseconds;
            if (button_hold_time_miliseconds >= (BUTTON_HOLD_TIME_SECS * 1000))
            {
                ESP_LOGI(TAG, "Button pressed for %d seconds, starting web Wi-Fi", BUTTON_HOLD_TIME_SECS);
                err = app_wifi__start();
                if (err != ESP_OK)
                {
                    ESP_LOGE(TAG, "Error starting Wi-Fi from %s", __func__);
                    app_error_handling__restart();
                }
                app_gpio__blink_green_led_slow(1);
                button_hold_time_miliseconds = 0;
                button_pressed = 0;
                vTaskSuspend(NULL);
            }
        }
    }
    vTaskDelete(NULL);
}
