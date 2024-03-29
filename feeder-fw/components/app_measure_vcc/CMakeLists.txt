idf_component_register(SRCS "app_measure_vcc.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES esp_adc app_status)
