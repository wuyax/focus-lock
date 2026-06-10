idf_component_register(SRCS "main.c" "config_mgr.c" "pomodoro_engine.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES led_strip nvs_flash esp_timer)
