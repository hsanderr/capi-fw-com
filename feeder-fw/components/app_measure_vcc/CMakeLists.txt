idf_component_register(SRCS "app_measure_vcc.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES app_gpio esp_adc)
