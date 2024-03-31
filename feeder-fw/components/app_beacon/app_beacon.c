/**
 * @file app_beacon.c
 * @author Henrique Sander Lourenço (henriquesander27@gmail.com)
 * @brief Contains beacon-related code.
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

#include <string.h>
#include <stdio.h>
#include <time.h>

#define LOG_LOCAL_LEVEL ESP_LOG_NONE
#include "esp_log.h"
#include "esp_err.h"
#include "esp_bt.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"

#include "app_beacon.h"
#include "app_status.h"
#include "app_pwm.h"

#define SCAN_FILTER_MAC (1)     ///< Filter scan by MAC address
#define SCAN_FILTER_RSSI (0)    ///< Filter scan by RSSI
#define SCAN_FILTER_EDD_TLM (1) ///< Filter scan by data type (Eddystone TLM)
#define PRINT_ADV_DATA (0)      ///< Print advertisements data

/*! @var typedef struct
    {
        esp_bd_addr_t auth_mac;
        uint8_t found;
        uint8_t times_seen_2_sec;
    } beacon_t;
    @brief Typedef to store beacon information.
 */
typedef struct
{
    esp_bd_addr_t auth_mac;
    uint8_t found;
    uint8_t times_seen;
} beacon_t;

/*! @var typedef enum
    {
        ble_scan_uninit = 0,
        ble_scan_initialing,
        ble_scan_off,
        ble_scan_starting,
        ble_scan_on,
        ble_scan_stopping,
        ble_scan_start_pending,
        ble_scan_stop_pending,
    } ble_scan_status_t;
    @brief Typedef for indicating current BLE status.
 */
typedef enum
{
    ble_scan_uninit = 0,    /**< BLE scan uninitialized */
    ble_scan_initialing,    /**< BLE scan initializing */
    ble_scan_off,           /**< BLE scan off */
    ble_scan_starting,      /**< BLE scan starting */
    ble_scan_on,            /**< BLE scan on */
    ble_scan_stopping,      /**< BLE scan stopping */
    ble_scan_start_pending, /**< BLE scan requested to be started while it could not be instantly started */
    ble_scan_stop_pending,  /**< BLE scan requested to be stopped while it could not be instantly stopped */
} ble_scan_status_t;

static const char *TAG = "app_beacon";                  ///< Tag to be used when logging
static ble_scan_status_t scan_status = ble_scan_uninit; ///< Variable that holds BLE scan status
static esp_ble_scan_params_t ble_scan_params = {
    .scan_type = BLE_SCAN_TYPE_PASSIVE,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval = 1600, // scan interval (ms) = 1600 * 0.625 = 1000 ms
    .scan_window = 800,    // scan window (ms) = 800 * 0.625 = 500 ms
    .scan_duplicate = BLE_SCAN_DUPLICATE_DISABLE,
}; ///< BLE scan parameters
static const char *scan_statuses_str[] = {
    "ble_scan_uninit",
    "ble_scan_initialing",
    "ble_scan_off",
    "ble_scan_starting",
    "ble_scan_on",
    "ble_scan_stopping",
    "ble_scan_start_pending",
    "ble_scan_stop_pending",
};                         ///< BLE scan statuses as strings for debugging
static int min_rssi = -41; ///< Minimum RSSI for detection (dB)
static beacon_t beacon = {
    .auth_mac = {0},
    .found = 0,
    .times_seen = 0,
};                                                               ///< Variable that holds beacon information                                                              ///< Beacon information
static TaskHandle_t app_beacon__beacon_check_task_handle = NULL; ///< Beacon lost check task handle

static void app_beacon__ble_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
static void app_beacon__beacon_check_task(void *arg);

/**
 * @brief Initialize necessary stuff to perform BLE scan.
 *
 * @return esp_err_t
 * @retval ESP_OK on success.
 * @retval Error code on failure.
 */
esp_err_t app_beacon__init(void)
{
    esp_err_t err = ESP_OK;
    ESP_LOGI(TAG, "Initializing app_beacon component...");

    if (scan_status != ble_scan_uninit)
    {
        ESP_LOGW(TAG, "BLE scan already initialized");
        return err;
    }

    scan_status = ble_scan_initialing;

    err = esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error releasing memory: %s", esp_err_to_name(err));
        return err;
    }

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    err = esp_bt_controller_init(&bt_cfg);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error initializing BT controller: %s",
                 esp_err_to_name(err));
        return err;
    }

    err = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error enabling BT controller: %s",
                 esp_err_to_name(err));
        return err;
    }

    err = esp_bluedroid_init();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error initializing bluedroid: %s",
                 esp_err_to_name(err));
        return err;
    }

    err = esp_bluedroid_enable();

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error enabling bluedroid %s",
                 esp_err_to_name(err));
        return err;
    }

    err = esp_ble_gap_register_callback(app_beacon__ble_gap_cb);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error registering BLE GAP callback: %s",
                 esp_err_to_name(err));
        return err;
    }

    err = esp_ble_gap_set_scan_params(&ble_scan_params);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "Error setting scan parameters: %s", esp_err_to_name(err));
        return err;
    }

    if (xTaskCreate(app_beacon__beacon_check_task,
                    "app_beacon__beacon_check_task", 2048, NULL, 10,
                    &app_beacon__beacon_check_task_handle) != pdPASS)
    {
        ESP_LOGE(TAG, "Error creating app_beacon__beacon_check_task");
        return ESP_FAIL;
    }
    vTaskSuspend(app_beacon__beacon_check_task_handle);
    ESP_LOGI(TAG, "Created app_beacon__beacon_check_task");

    return err;
}

/**
 * @brief BLE GAP event handler.
 *
 * @param event Tell which event happened.
 * @param param Pointer to union with other event data.
 */
static void app_beacon__ble_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    esp_err_t err;

    switch (event)
    {
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
    {
        // BLE parameters setting completed

        err = param->scan_param_cmpl.status;
        if (err != ESP_BT_STATUS_SUCCESS)
        {
            scan_status = ble_scan_uninit;
            ESP_LOGE(TAG, "BLE scan parameters setting failed: %s",
                     esp_err_to_name(err));
        }
        else
        {
            // start BLE scan if parameters were set successfully

            if (scan_status != ble_scan_stop_pending)
            {
                // if BLE scan stop was requested, do not start BLE scan
                scan_status = ble_scan_off;
            }

            err = app_beacon__ble_scan_start();
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "Error starting BLE scan: %s",
                         esp_err_to_name(err));
            }
        }
        break;
    }
    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
    {
        err = param->scan_start_cmpl.status;

        if (err != ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGE(TAG, "BLE scan start failed: %s", esp_err_to_name(err));
            scan_status = ble_scan_off;
        }
        else
        {
            ble_scan_status_t scan_status_temp = scan_status;
            scan_status = ble_scan_on;
            ESP_LOGI(TAG, "BLE scan started, scan_status=%s",
                     scan_statuses_str[scan_status]);

            if (scan_status_temp == ble_scan_stop_pending)
            {
                // if BLE scan stop was requested, stop BLE scan right after it was started
                app_beacon__ble_scan_stop();
            }
        }
        break;
    }
    case ESP_GAP_BLE_SCAN_RESULT_EVT:
    {
        // BLE scan results ready

        esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
        if (scan_result->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_RES_EVT)
        {
            if (1
#if SCAN_FILTER_MAC
                // check if advertisement MAC matches authorized MAC address
                && (beacon.auth_mac[0] == scan_result->scan_rst.bda[0] &&
                    beacon.auth_mac[1] == scan_result->scan_rst.bda[1] &&
                    beacon.auth_mac[2] == scan_result->scan_rst.bda[2] &&
                    beacon.auth_mac[3] == scan_result->scan_rst.bda[3] &&
                    beacon.auth_mac[4] == scan_result->scan_rst.bda[4] &&
                    beacon.auth_mac[5] == scan_result->scan_rst.bda[5])
#endif // SCAN_FILTER_MAC
#if SCAN_FILTER_RSSI
                // check if advertisement RSSI is higher than minimum (this is also checked later when detecting beacon)
                && (scan_result->scan_rst.rssi > min_rssi)
#endif // SCAN_FILTER_RSSI
#if SCAN_FILTER_EDD_TLM
                // check if advertisement is in Eddystone TLM format
                // see https://github.com/google/eddystone/blob/master/protocol-specification.md
                && (scan_result->scan_rst.ble_adv[0] == 0x02 &&
                    scan_result->scan_rst.ble_adv[1] == 0x01 &&
                    scan_result->scan_rst.ble_adv[2] == 0x06 &&
                    scan_result->scan_rst.ble_adv[3] == 0x03 &&
                    scan_result->scan_rst.ble_adv[4] == 0x03 &&
                    scan_result->scan_rst.ble_adv[5] == 0xaa &&
                    scan_result->scan_rst.ble_adv[6] == 0xfe &&
                    scan_result->scan_rst.ble_adv[7] == 0x11 &&
                    scan_result->scan_rst.ble_adv[8] == 0x16 &&
                    scan_result->scan_rst.ble_adv[9] == 0xaa &&
                    scan_result->scan_rst.ble_adv[10] == 0xfe &&
                    scan_result->scan_rst.ble_adv[11] == 0x20)
#endif // SCAN_FILTER_EDD_TLM
            )
            {
                ESP_LOGI(TAG,
                         "Device found, MAC: %2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x",
                         scan_result->scan_rst.bda[0],
                         scan_result->scan_rst.bda[1],
                         scan_result->scan_rst.bda[2],
                         scan_result->scan_rst.bda[3],
                         scan_result->scan_rst.bda[4],
                         scan_result->scan_rst.bda[5]);
                ESP_LOGI(TAG, "RSSI: %d dBm",
                         scan_result->scan_rst.rssi);
#if PRINT_ADV_DATA
                ESP_LOGI(TAG, "Adv data:");
                for (uint8_t i = 0; i < scan_result->scan_rst.adv_data_len; i++)
                {
                    printf("%2.2x ", scan_result->scan_rst.ble_adv[i]);
                }
                printf("\n");
#endif // PRINT_ADV_DATA
                uint16_t beacon_bat_mv =
                    (scan_result->scan_rst.ble_adv[10 + 3] << 8) | scan_result->scan_rst.ble_adv[11 + 3]; // get beacon battery level in mV
                int8_t beacon_temp_c_int =
                    scan_result->scan_rst.ble_adv[12 + 3]; // get integer part of beacon temperature in degrees Celsius
                uint8_t beacon_temp_c_dec =
                    (scan_result->scan_rst.ble_adv[13 + 3] * 100) / 255; // get decimal part of beacon temperature in degrees Celsius

                ESP_LOGI(TAG, "Beacon battery: %" PRIu16 " mV",
                         beacon_bat_mv);
                ESP_LOGI(TAG,
                         "Beacon temperature: %" PRId8 ".%" PRIu8
                         " degrees Celsius",
                         beacon_temp_c_int, beacon_temp_c_dec);

                if (beacon_bat_mv < 3000)
                {
                    // set beacon battery to low if battery level is less than 3000 mV
                    app_status__set_beacon_battery_low_status(1);
                }
                else
                {
                    // set beacon battery to ok if battery level is greater or equal than 3000 mV
                    app_status__set_beacon_battery_low_status(0);
                }

                /* The beacon.found flag is set to 1 if the beacon is seen 2 times in a short period of time
                 * and the RSSI is greater than min_rssi. When the beacon is seen 2 times, the lid will be
                 * opened. This is a debouncing scheme so that the lid does not open when the pet just passes
                 * nearby. Everytime this beacon is seen, the task app_beacon__beacon_check_task is resumed.
                 * This task checks if the number of times that the beacon has been seen when the task was
                 * resumed is different from the number of times it was seen some seconds later. If the
                 * numbers are not different, it means that the beacon has not been seen for some time and
                 * in this case the lid will be closed and beacon.found will be set to zero.
                 */
                if (!beacon.found && scan_result->scan_rst.rssi >= min_rssi)
                {
                    beacon.times_seen++;
                    vTaskResume(app_beacon__beacon_check_task_handle);
                    if (beacon.times_seen == 2)
                    {
                        ESP_LOGI(TAG, "Beacon detected, opening lid");
                        beacon.found = 1;
                        app_pwm__set_duty_max();
                    }
                }
                else if (scan_result->scan_rst.rssi >= min_rssi)
                {
                    beacon.times_seen++;
                    vTaskResume(app_beacon__beacon_check_task_handle);
                }
            }
        }
        break;
    }
    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
    {
        err = param->scan_stop_cmpl.status;

        if (err != ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGE(TAG, "BLE scan stop failed: %s", esp_err_to_name(err));
            scan_status = ble_scan_on;
        }
        else
        {
            ESP_LOGI(TAG, "BLE scan stopped");
            ble_scan_status_t scan_status_temp = scan_status;
            scan_status = ble_scan_off;

            if (scan_status_temp == ble_scan_start_pending)
            {
                // if BLE scan start was requested, start BLE scan right after it was stopped
                app_beacon__ble_scan_start();
            }
        }
        break;
    }
    default:
        break;
    }
}

/**
 * @brief Start BLE scan. If scan is stopping, scan start is put into pending
 * state, so it is started after it finishes stopping.
 *
 * @return esp_err_t
 * @retval ESP_OK on success.
 * @retval Error code on failure.
 */
esp_err_t app_beacon__ble_scan_start(void)
{
    esp_err_t err = ESP_OK;

    if (scan_status == ble_scan_uninit)
    {
        err = app_beacon__init();

        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "BLE scan initialization failed: %s",
                     esp_err_to_name(err));
        }
    }
    else if (scan_status == ble_scan_off)
    {
        scan_status = ble_scan_starting;
        err = esp_ble_gap_start_scanning(0);

        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "BLE GAP scan start failed: %s",
                     esp_err_to_name(err));
        }
    }
    else if (scan_status == ble_scan_stopping)
    {
        scan_status = ble_scan_start_pending;
    }
    else if (scan_status == ble_scan_stop_pending)
    {
        ESP_LOGI(TAG, "BLE scan stop already pending, not starting scan");
        scan_status = ble_scan_off;
    }
    else
    {
        ESP_LOGI(TAG, "Cannot start BLE scan right now, scan_status=%d (%s)",
                 scan_status, scan_statuses_str[scan_status]);
    }

    return err;
}

/**
 * @brief Stop BLE scan. If scan is starting, scan stop is put into pending
 * state, so it is stopped after it finishes starting.
 *
 * @return esp_err_t
 * @retval ESP_OK on success.
 * @retval Error code on failure.
 */
esp_err_t app_beacon__ble_scan_stop(void)
{
    esp_err_t err = ESP_OK;

    if (scan_status == ble_scan_uninit)
    {
        ESP_LOGW(TAG, "BLE scan is not initialized yet");
    }
    else if (scan_status == ble_scan_on)
    {
        scan_status = ble_scan_stopping;
        err = esp_ble_gap_stop_scanning();

        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "BLE GAP scan stop failed: %s",
                     esp_err_to_name(err));
        }
    }
    else if ((scan_status == ble_scan_starting) ||
             (scan_status == ble_scan_initialing))
    {
        scan_status = ble_scan_stop_pending;
    }
    else
    {
        ESP_LOGI(TAG, "Cannot stop BLE scan right now, scan_status=%d (%s)",
                 scan_status, scan_statuses_str[scan_status]);
    }
    return err;
}

/**
 * @brief Sets authorized MAC address in beacon structure.
 *
 * @param mac_addr 6 bytes array with authorized MAC address.
 */
void app_beacon__set_auth_mac(uint8_t mac_addr[6])
{
    memcpy(beacon.auth_mac, mac_addr, ESP_BD_ADDR_LEN);
}

/**
 * @brief This task checks if the number of times that the beacon has been seen when the task was
 * resumed is different from the number of times it was seen some seconds later. If the
 * numbers are not different, it means that the beacon has not been seen for some time and
 * in this case the lid will be closed and beacon.found will be set to zero.
 *
 * @param arg
 */
static void app_beacon__beacon_check_task(void *arg)
{
    for (;;)
    {
        uint8_t beacon_times_seen_prev = beacon.times_seen;
        vTaskDelay(pdMS_TO_TICKS(3000));
        if (beacon.times_seen == beacon_times_seen_prev)
        {
            beacon.times_seen = 0;
            if (beacon.found)
            {
                beacon.found = 0;
                ESP_LOGI(TAG, "Beacon lost, closing lid");
                app_pwm__set_duty_min();
            }
            vTaskSuspend(NULL);
        }
    }
    vTaskDelete(NULL);
}