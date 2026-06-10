idf_component_register(SRCS "main.c" "config_mgr.c" "pomodoro_engine.c" "rgb_service.c" "button_service.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES led_strip nvs_flash esp_timer esp_driver_gpio)
