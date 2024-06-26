/**
 * @file app_gpio.c
 * @author Henrique Sander Lourenço (henriquesander27@gmail.com)
 * @brief Contains GPIO-related code (buttons and LEDs).
 * @version 0.1
 * @date 2024-03-20
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

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"
#include "esp_err.h"
#include "esp_intr_alloc.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "app_gpio.h"
#include "app_wifi.h"

#define GPIO_BLUE_LED (2)                                                                        ///< Blue LED GPIO
#define GPIO_RED_LED (4)                                                                         ///< Red LED GPIO
#define GPIO_BUTTON (16)                                                                         ///< Button GPIO
#define GPIO_OUTPUT_PIN_SEL ((((uint64_t)1) << GPIO_BLUE_LED) | (((uint64_t)1) << GPIO_RED_LED)) ///< LEDs pin mask
#define GPIO_INPUT_PIN_SEL (((uint64_t)1) << GPIO_BUTTON)                                        ///< Button pin mask
#define BUTTON_HOLD_TIME_SECS (3)

static const char *TAG = "app_gpio";                           ///< Tag to be used when logging
static uint8_t button_pressed = 0;                             ///< Button state, 1 if pressed, 0 otherwise
static TaskHandle_t app_gpio__check_button_task_handle = NULL; ///< Button state check task handle

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
        .intr_type = GPIO_INTR_NEGEDGE};
    esp_err_t err = gpio_config(&config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error %d configuring input GPIOs: %s", err, esp_err_to_name(err));
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Success configuring input GPIOs!");

    // gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1 | ESP_INTR_FLAG_EDGE | ESP_INTR_FLAG_SHARED | ESP_INTR_FLAG_IRAM);
    gpio_install_isr_service(0);

    gpio_isr_handler_add(GPIO_BUTTON, app_gpio__isr_handler, NULL);

    config.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    config.mode = GPIO_MODE_OUTPUT;
    config.pull_up_en = GPIO_PULLUP_DISABLE;
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    config.intr_type = GPIO_INTR_DISABLE;
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

    gpio_set_level(GPIO_BLUE_LED, 0);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error %d setting blue LED GPIO to low: %s", err, esp_err_to_name(err));
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Set blue LED GPIO to low");

    if (xTaskCreate(app_gpio__check_button_task,
                    "app_gpio__check_button_task", 2048, NULL, 10,
                    &app_gpio__check_button_task_handle) != pdPASS)
    {
        ESP_LOGE(TAG, "Error creating app_gpio__check_button_task");
        return ESP_FAIL;
    }
    vTaskSuspend(app_gpio__check_button_task_handle);
    ESP_LOGI(TAG, "Created app_gpio__check_button_task");

    return ESP_OK;
}

/**
 * @brief Slowly blink blue LED for the specified number of times.
 *
 * @param times Number of times to blink the LED.
 * @return esp_err_t
 * @retval ESP_OK if LED is successfully blinked.
 * @retval ESP_FAIL otherwise.
 */
esp_err_t app_gpio__blink_blue_led_slow(uint8_t times)
{
    for (uint8_t i = 0; i < times; i++)
    {
        esp_err_t err = gpio_set_level(GPIO_BLUE_LED, 1);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error %d setting blue LED GPIO to high: %s", err, esp_err_to_name(err));
            return ESP_FAIL;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
        gpio_set_level(GPIO_BLUE_LED, 0);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error %d setting blue LED GPIO to low: %s", err, esp_err_to_name(err));
            return ESP_FAIL;
        }
        if (i != times - 1)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    return ESP_OK;
}

/**
 * @brief Fastly blink blue LED for the specified number of times.
 *
 * @param times Number of times to blink the LED.
 * @return esp_err_t
 * @retval ESP_OK if LED is successfully blinked.
 * @retval ESP_FAIL otherwise.
 */
esp_err_t app_gpio__blink_blue_led_fast(uint8_t times)
{
    for (uint8_t i = 0; i < times; i++)
    {
        esp_err_t err = gpio_set_level(GPIO_BLUE_LED, 1);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error %d setting blue LED GPIO to high: %s", err, esp_err_to_name(err));
            return ESP_FAIL;
        }
        vTaskDelay(pdMS_TO_TICKS(250));
        gpio_set_level(GPIO_BLUE_LED, 0);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error %d setting blue LED GPIO to low: %s", err, esp_err_to_name(err));
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
        vTaskDelay(pdMS_TO_TICKS(1000));
        gpio_set_level(GPIO_RED_LED, 0);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error %d setting red LED GPIO to low: %s", err, esp_err_to_name(err));
            return ESP_FAIL;
        }
        if (i != times - 1)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
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
    gpio_intr_disable(GPIO_BUTTON);
    gpio_isr_handler_remove(GPIO_BUTTON);
    if (!button_pressed)
    {
        button_pressed = 1;
        vTaskResume(app_gpio__check_button_task_handle);
    }
    gpio_isr_handler_add(GPIO_BUTTON, app_gpio__isr_handler, NULL);
    gpio_intr_enable(GPIO_BUTTON);
}

/**
 * @brief Task to check if button is being holded for a specified amount of time.
 *
 * @param arg Optional argument (not being used).
 */
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
                ESP_LOGI(TAG, "Button pressed for %d seconds, (re)starting web Wi-Fi", BUTTON_HOLD_TIME_SECS);
                err = app_wifi__start();
                if (err != ESP_OK)
                {
                    ESP_LOGE(TAG, "Error starting Wi-Fi from %s", __func__);
                    app_error_handling__restart();
                }
                button_hold_time_miliseconds = 0;
                button_pressed = 0;
                vTaskSuspend(NULL);
            }
        }
    }
    vTaskDelete(NULL);
}
