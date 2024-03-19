/**
 * @file app_web_server.c
 * @author Henrique Sander Lourenço (henriquesander27@gmail.com)
 * @brief Contains web server-related code.
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

#include <math.h>
#include <stdlib.h>

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"
#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_system.h"

#include "app_web_server.h"
#include "app_nvs.h"

static const char *TAG = "app_web_server"; ///< Tag to be used when logging

static const char home_page_html[] = MAIN_PAGE_GET;                 ///< Home page HTML
static const char form_submission_response_html[] = MAIN_PAGE_POST; ///< Form submission response HTML
static httpd_handle_t httpd_handle = NULL;                          ///< HTTP daemon handler
static httpd_config_t httpd_config = HTTPD_DEFAULT_CONFIG();        ///< HTTP daemon configuration

static esp_err_t app_web_server__get_main_handler(httpd_req_t *req);
static esp_err_t app_web_server__post_main_handler(httpd_req_t *req);

/**
 * @brief Start web server
 *
 * @return esp_err_t
 * @retval ESP_OK if web server is successfully started.
 * @retval ESP_FAIL otherwise.
 */
esp_err_t app_web_server__start(void)
{
    esp_err_t err = httpd_start(&httpd_handle, &httpd_config);

    httpd_uri_t get_main = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = app_web_server__get_main_handler,
        .user_ctx = NULL,
    }; ///< URI handler for GET / request

    httpd_uri_t post_main = {
        .uri = "/",
        .method = HTTP_POST,
        .handler = app_web_server__post_main_handler,
        .user_ctx = NULL,
    }; ///< URI handler for POST / request

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error %d starting HTTP daemon: %s", err, esp_err_to_name(err));
        return ESP_FAIL;
    }
    else
    {
        ESP_LOGI(TAG, "Success starting HTTP daemon");

        err = httpd_register_uri_handler(httpd_handle, &get_main); // register GET / URI handler
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error %d registering URI handler: %s", err, esp_err_to_name(err));
            return ESP_FAIL;
        }
        else
        {
            err = httpd_register_uri_handler(httpd_handle, &post_main); // register POST / URI handler
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "Error %d registering URI handler: %s", err, esp_err_to_name(err));
                return ESP_FAIL;
            }
            else
            {
                ESP_LOGI(TAG, "Success registering URI handlers!");
                ESP_LOGI(TAG, "Success starting web server!");
                return ESP_OK;
            }
        }
    }
}

/**
 * @brief Stop web server
 *
 * @return esp_err_t
 * @retval ESP_OK if web server is successfully stopped.
 * @retval ESP_FAIL otherwise.
 */
esp_err_t app_web_server__stop(void)
{
    esp_err_t err = httpd_stop(httpd_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error %d stopping HTTP daemon: %s", err, esp_err_to_name(err));
        return ESP_FAIL;
    }
    else
    {
        ESP_LOGI(TAG, "Success stopping HTTP daemon");
        return ESP_OK;
    }
}

/**
 * @brief Handler for GET / request.
 *
 * @param req HTTP request data.
 * @return esp_err_t
 * @retval ESP_OK if response is sent successfully.
 * @retval ESP_FAIL otherwise.
 */
static esp_err_t app_web_server__get_main_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Received HTTP request (GET /)");
    esp_err_t err = httpd_resp_send(req, home_page_html, HTTPD_RESP_USE_STRLEN);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error %d sending HTTP response: %s", err, esp_err_to_name(err));
        return ESP_FAIL;
    }
    else
    {
        ESP_LOGI(TAG, "Success sending HTTP response!");
        return ESP_OK;
    }
}

/**
 * @brief Handler for POST / request.
 *
 * @param req HTTP request data.
 * @return esp_err_t
 * @retval ESP_OK if response is sent successfully.
 * @retval ESP_FAIL otherwise.
 */
static esp_err_t app_web_server__post_main_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Received HTTP request (POST /)");
    char request_content[17] = {0};
    char auhtorized_mac_byte_as_str[3] = {0};
    uint8_t authorized_mac[6] = {0};
    esp_err_t err = httpd_req_recv(req, request_content, fminf((float)req->content_len, (float)sizeof(request_content)));
    if (err <= 0)
    {
        ESP_LOGE(TAG, "Error receiving POST request");

        // Check if timeout occurred
        if (err == HTTPD_SOCK_ERR_TIMEOUT)
        {
            ESP_LOGE(TAG, "Timeout receiving POST request");
            err = httpd_resp_send_408(req);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "Error %d sending HTTP response: %s", err, esp_err_to_name(err));
            }
            else
            {
                ESP_LOGI(TAG, "Success sending HTTP response!");
            }
        }

        /* In case of error, returning ESP_FAIL will
         * ensure that the underlying socket is closed */
        return ESP_FAIL;
    }
    else
    {
        ESP_LOGI(TAG, "Succes receiving POST request, content received: %s", request_content);

        err = httpd_resp_send(req, form_submission_response_html, HTTPD_RESP_USE_STRLEN);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error %d sending HTTP response: %s", err, esp_err_to_name(err));
        }
        else
        {
            ESP_LOGI(TAG, "Success sending HTTP response!");
        }

        // convert authorized MAC received as string to array of bytes
        for (uint8_t i = 0; i < sizeof(authorized_mac) / sizeof(authorized_mac[0]); i++)
        {
            auhtorized_mac_byte_as_str[0] = request_content[(i * 2) + 4];
            auhtorized_mac_byte_as_str[1] = request_content[(i * 2) + 5];
            authorized_mac[i] = (uint8_t)strtol(auhtorized_mac_byte_as_str, NULL, 16);
            ESP_LOGI(TAG, "Converted %s to 0x%2.2x", auhtorized_mac_byte_as_str, authorized_mac[i]);
        }
        ESP_LOGI(TAG, "Authorized MAC after converting from str to array of bytes: 0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x",
                 authorized_mac[0], authorized_mac[1], authorized_mac[2], authorized_mac[3], authorized_mac[4], authorized_mac[5]);

        err = app_nvs__set_authorized_mac(authorized_mac);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error %d writing authorized MAC to NVS: %s", err, esp_err_to_name(err));
            return ESP_FAIL;
        }
        else
        {
            ESP_LOGI(TAG, "Success writing authorized MAC to NVS!");
            return ESP_OK;
        }
    }
}