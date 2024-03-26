idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES app_nvs app_wifi app_web_server app_gpio app_measure_vcc app_status app_pwm app_beacon)