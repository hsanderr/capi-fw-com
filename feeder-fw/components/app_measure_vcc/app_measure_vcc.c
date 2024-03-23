/**
 * @file app_measure_vcc.c
 * @author Henrique Sander Lourenço (henriquesander27@gmail.com)
 * @brief Contains code (buttons and LEDs) to measure the battery voltage.
 * @version 0.1
 * @date 2024-03-23
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
#include "esp_adc/adc_oneshot.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "app_measure_vcc.h"
#include "app_status.h"

#define VOLTAGE_MEAS_AVG_ARR_SIZE 10 ///< Voltage measurements average array size

static esp_err_t app_measure_vcc__calibrate_adc(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle);
static void app_measure_vcc__adc_read_task(void *args);

static const char *TAG = "app_measure_vcc";  ///< Tag to be used when logging
static adc_oneshot_unit_handle_t adc_handle; ///< ADC handle
static adc_oneshot_unit_init_cfg_t init_config = {
    .unit_id = ADC_UNIT_1,
};                                                                ///< ADC initialization configuration
static adc_cali_handle_t adc_cal_handle = NULL;                   ///< ADC calibration handle
uint8_t calibration_successful = 0;                               ///< Flag to indicate if calibration was successful
int voltage_measurements[VOLTAGE_MEAS_AVG_ARR_SIZE] = {0};        ///< Array to store voltage measurements to calculate average
int voltage_measurements_index = 0;                               ///< Index to keep track of the current measurement
static TaskHandle_t app_measure_vcc__adc_read_task_handle = NULL; ///< Read ADC task handle

/**
 * @brief Initialize VCC measurement.
 *
 * @return esp_err_t
 * @retval ESP_OK if VCC measurement is successfully initialized.
 * @retval ESP_FAIL otherwise.
 */
esp_err_t app_measure_vcc__init(void)
{
    adc_oneshot_chan_cfg_t adc_config = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten = ADC_ATTEN_DB_12,
    };
    esp_err_t err = adc_oneshot_new_unit(&init_config, &adc_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error %d creating new ADC unit: %s", err, esp_err_to_name(err));
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Success creating new ADC unit!");
    err = adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_0, &adc_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error %d configuring ADC channel: %s", err, esp_err_to_name(err));
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Success configuring ADC channel!");

    err = app_measure_vcc__calibrate_adc(ADC_UNIT_1, ADC_CHANNEL_0, ADC_ATTEN_DB_12, &adc_cal_handle);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "Success calibrating ADC!");
        calibration_successful = 1;
    }

    if (xTaskCreate(app_measure_vcc__adc_read_task,
                    "app_measure_vcc__adc_read_task", 2048, NULL, 10,
                    &app_measure_vcc__adc_read_task_handle) != pdPASS)
    {
        ESP_LOGE(TAG, "Error creating app_measure_vcc__adc_read_task");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Created app_measure_vcc__adc_read_task");
    ESP_LOGI(TAG, "Success initializing app_measure_vcc component");
    return ESP_OK;
}

/**
 * @brief Calibrate ADC.
 *
 * @param unit ADC unit.
 * @param channel ADC channel.
 * @param atten ADC attenuation.
 * @param out_handle Output calibration handle, used to store the calibration data and convert the raw reading values to voltage.
 * @return esp_err_t
 * @retval ESP_OK if calibration is successful.
 * @retval ESP_FAIL otherwise.
 */
static esp_err_t app_measure_vcc__calibrate_adc(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t err;

    ESP_LOGI(TAG, "Calibration scheme version is Line Fitting");
    adc_cali_line_fitting_config_t cal_config = {
        .unit_id = unit,
        .atten = atten,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    err = adc_cali_create_scheme_line_fitting(&cal_config, &handle);
    *out_handle = handle;

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error %d calibrating ADC channel: %s", err, esp_err_to_name(err));
        return ESP_FAIL;
    }
    else
    {
        ESP_LOGI(TAG, "Calibration successful");
        return ESP_OK;
    }
}

/**
 * @brief Task to perform the ADC readings.
 *
 * @param args Optional argument (not being used).
 */
static void app_measure_vcc__adc_read_task(void *args)
{
    for (;;)
    {
        int adc_raw;
        int voltage;
        esp_err_t err = adc_oneshot_read(adc_handle, ADC_CHANNEL_0, &adc_raw);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error %d reading ADC: %s", err, esp_err_to_name(err));
        }
        else
        {
            ESP_LOGI(TAG, "Raw ADC value: %d", adc_raw);
            if (calibration_successful)
            {
                err = adc_cali_raw_to_voltage(adc_cal_handle, adc_raw, &voltage);
                ESP_LOGI(TAG, "Voltage: %d mV", voltage);
            }
            else
            {
                voltage = (adc_raw * 3300) / 4095;
            }
            voltage_measurements[voltage_measurements_index] = voltage;
            voltage_measurements_index++;
            if (voltage_measurements_index == VOLTAGE_MEAS_AVG_ARR_SIZE)
            {
                voltage_measurements_index = 0;
                int avg = 0;
                for (int i = 0; i < VOLTAGE_MEAS_AVG_ARR_SIZE; i++)
                {
                    avg += voltage_measurements[i];
                }
                avg /= VOLTAGE_MEAS_AVG_ARR_SIZE;
                ESP_LOGI(TAG, "Average voltage: %d mV", voltage);
                if (avg < 2500)
                {
                    ESP_LOGW(TAG, "Battery voltage is low!");
                    app_status__set_battery_low_status(1);
                }
                else
                {
                    ESP_LOGI(TAG, "Battery voltage is ok!");
                    app_status__set_battery_low_status(0);
                }
            }
        }
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}
