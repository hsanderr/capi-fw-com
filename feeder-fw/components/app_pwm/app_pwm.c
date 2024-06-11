/**
 * @file app_pwm.c
 * @author Henrique Sander Lourenço (henriquesander27@gmail.com)
 * @brief Contains PWM-related code to control the feeder lid.
 * @version 0.1
 * @date 2024-03-24
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
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.'
    See the License for the specific language governing permissions and
    limitations under the License.
 */

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"

#include "app_pwm.h"

#define PWM_TIMER_TIME_TO_PAUSE_MS (500) ///< Time to wait after resuming PWM timer to pause it, in order to save energy

static const char *TAG = "app_pwm"; ///< Tag to be used when logging

static TaskHandle_t app_pwm__pwm_timer_pause_task_handle = NULL; ///< PWM timer pause task handle

static void app_pwm__pwm_timer_pause_task(void *arg);

/**
 * @brief Initialize PWM and pause related timer.
 *
 * @return esp_err_t
 * @retval ESP_OK if PWM is successfully initialized.
 * @retval ESP_FAIL otherwise.
 */
esp_err_t app_pwm__init(void)
{
    ledc_timer_config_t pwm_timer_config = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_20_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 50,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_channel_config_t pwm_channel_config = {
        .gpio_num = 5,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0};
    esp_err_t err = ledc_timer_config(&pwm_timer_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error configuring PWM timer");
        return ESP_FAIL;
    }

    err = ledc_channel_config(&pwm_channel_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error configuring PWM channel");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Setting duty cycle to 0.5 ms...");

    err = ledc_timer_pause(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error pausing PWM timer");
        return ESP_FAIL;
    }

    if (xTaskCreate(app_pwm__pwm_timer_pause_task,
                    "app_pwm__pwm_timer_pause_task", 2048, NULL, 10,
                    &app_pwm__pwm_timer_pause_task_handle) != pdPASS)
    {
        ESP_LOGE(TAG, "Error creating app_pwm__pwm_timer_pause_task");
        return ESP_FAIL;
    }

    return app_pwm__set_duty_min();
}

/**
 * @brief Set PWM duty cycle to minimum (close feeder lid), resume and pause timer.
 *
 * @return esp_err_t
 * @retval ESP_OK if PWM duty cycle is successfully set to minimum.
 * @retval ESP_FAIL otherwise.
 */
esp_err_t app_pwm__set_duty_min(void)
{
    esp_err_t err = ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 26214); // 0.5 ms (min)
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error setting PWM duty cycle");
        return ESP_FAIL;
    }
    err = ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error updating PWM duty cycle");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Duty cycle set to minimum");

    err = ledc_timer_resume(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error resuming PWM timer");
        return ESP_FAIL;
    }

    vTaskResume(app_pwm__pwm_timer_pause_task_handle);

    return ESP_OK;
}

/**
 * @brief Set PWM duty cycle to maximum (open feeder lid), resume and pause timer.
 *
 * @return esp_err_t
 * @retval ESP_OK if PWM duty cycle is successfully set to maximum.
 * @retval ESP_FAIL otherwise.
 */
esp_err_t app_pwm__set_duty_max(void)
{
    esp_err_t err = ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 78000);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error setting PWM duty cycle");
        return ESP_FAIL;
    }
    err = ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error updating PWM duty cycle");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Duty cycle set to maximum");

    err = ledc_timer_resume(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error resuming PWM timer");
        return ESP_FAIL;
    }

    vTaskResume(app_pwm__pwm_timer_pause_task_handle);

    return ESP_OK;
}

/**
 * @brief Task to pause PWM timer after a specified time.
 *
 * @param arg
 */
static void app_pwm__pwm_timer_pause_task(void *arg)
{
    vTaskSuspend(NULL);
    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(PWM_TIMER_TIME_TO_PAUSE_MS));
        esp_err_t err = ledc_timer_pause(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error pausing PWM timer");
        }
        vTaskSuspend(NULL);
    }
    vTaskDelete(NULL);
}