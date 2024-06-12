/**
 * @file app_nvs.c
 * @author Henrique Sander Lourenço (henriquesander27@gmail.com)
 * @brief Contains NVS-related code.
 * @version 0.1
 * @date 2024-03-16
 *
 * @copyright Copyright (c) 2024 PetDog
 *
 */

#define LOG_LOCAL_LEVEL ESP_LOG_NONE
#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"

#include "app_nvs.h"
#include "app_beacon.h"

#define MAIN_NVS_NAMESPACE "nvs_main"       ///< Main NVS namespace
#define AUTHORIZED_MAC_ENTRY_KEY "auth_mac" ///< Authorized MAC entry key

static const char *TAG = "app_nvs"; ///< Tag to be used when logging

/**
 * @brief Initialize NVS.
 *
 * @return esp_err_t
 * @retval ESP_OK if NVS is initialized successfully.
 * @retval ESP_FAIL otherwise.
 */
esp_err_t app_nvs__init(void)
{
    ESP_LOGI(TAG, "Initializing NVS");
    esp_err_t err = nvs_flash_init();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error %d initializing NVS: %s", err, esp_err_to_name(err));
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Success initializing NVS!");
    return ESP_OK;
}

/**
 * @brief Read all relevant data from NVS.
 *
 * @return esp_err_t
 * @retval ESP_OK if data is successfully read from NVS.
 * @retval ESP_ERR_NOT_FOUND if some data is not found in NVS.
 * @retval ESP_FAIL otherwise.
 */
esp_err_t app_nvs__get_data(void)
{
    ESP_LOGI(TAG, "Getting NVS data");
    uint8_t authorized_mac[6] = {0};
    esp_err_t err = app_nvs__get_authorized_mac(authorized_mac);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGE(TAG, "Error %d getting authorized MAC from NVS: %s", err, esp_err_to_name(err));
        return ESP_FAIL;
    }
    else if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGW(TAG, "Could not find authorized MAC in NVS");
        return ESP_ERR_NOT_FOUND;
    }
    {
        ESP_LOGI(TAG, "Success getting MAC address from NVS: 0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x",
                 authorized_mac[0], authorized_mac[1], authorized_mac[2], authorized_mac[3], authorized_mac[4], authorized_mac[5]);
        ESP_LOGI(TAG, "Success getting data from NVS!");
        return ESP_OK;
    }
}

/**
 * @brief Write authorized MAC to NVS.
 *
 * @param authorized_mac 6 bytes array with authorized MAC to be written.
 * @return esp_err_t
 * @retval ESP_OK if authorized MAC is sucessfully written to NVS.
 * @retval ESP_FAIL otherwise.
 */
esp_err_t app_nvs__set_authorized_mac(uint8_t authorized_mac[6])
{
    ESP_LOGI(TAG, "Setting authorized MAC address in NVS");
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(MAIN_NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error %d opening NVS: %s", err, esp_err_to_name(err));
        return ESP_FAIL;
    }
    else
    {
        ESP_LOGD(TAG, "Success opening NVS!");
        err = nvs_set_blob(nvs_handle, AUTHORIZED_MAC_ENTRY_KEY, authorized_mac, 6);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error %d setting blob in NVS: %s", err, esp_err_to_name(err));
            return ESP_FAIL;
        }
        else
        {
            ESP_LOGD(TAG, "Success setting blob in NVS, authorized MAC: 0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x",
                     authorized_mac[0], authorized_mac[1], authorized_mac[2], authorized_mac[3], authorized_mac[4], authorized_mac[5]);
            app_beacon__set_auth_mac(authorized_mac);
            return ESP_OK;
        }
    }
}

/**
 * @brief Reads authorized MAC from NVS.
 *
 * @param authorized_mac 6 bytes array where the authorized MAC will be stored.
 * @return esp_err_t
 * @retval ESP_OK if authorized MAC is successfully read from NVS.
 * @retval ESP_ERR_NVS_NOT_FOUND if authorized MAC is not found in NVS.
 * @retval ESP_FAIL otherwise.
 */
esp_err_t app_nvs__get_authorized_mac(uint8_t authorized_mac[6])
{
    ESP_LOGI(TAG, "Getting authorized MAC address from NVS");
    nvs_handle_t nvs_handle;
    uint8_t authorized_mac_len = 6;
    esp_err_t err = nvs_open(MAIN_NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error %d opening NVS: %s", err, esp_err_to_name(err));
        return ESP_FAIL;
    }
    else
    {
        ESP_LOGD(TAG, "Success opening NVS!");
        err = nvs_get_blob(nvs_handle, AUTHORIZED_MAC_ENTRY_KEY, authorized_mac, (size_t *)&authorized_mac_len);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
        {
            ESP_LOGE(TAG, "Error %d getting blob from NVS: %s", err, esp_err_to_name(err));
            return ESP_FAIL;
        }
        else if (err == ESP_ERR_NVS_NOT_FOUND)
        {
            ESP_LOGW(TAG, "No MAC address written to NVS yet");
            return ESP_ERR_NVS_NOT_FOUND;
        }
        else
        {
            ESP_LOGD(TAG, "Success getting blob from NVS, authorized MAC: 0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x",
                     authorized_mac[0], authorized_mac[1], authorized_mac[2], authorized_mac[3], authorized_mac[4], authorized_mac[5]);
            app_beacon__set_auth_mac(authorized_mac);
            return ESP_OK;
        }
    }
}
