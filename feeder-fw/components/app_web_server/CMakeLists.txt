idf_component_register(SRCS "app_web_server.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_http_server app_nvs)
