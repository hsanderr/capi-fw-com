idf_component_register(SRCS "app_wifi.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES esp_wifi app_web_server app_gpio)
