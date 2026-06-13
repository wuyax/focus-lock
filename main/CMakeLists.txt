idf_component_register(SRCS "rtc_service.c" "main.c" "config_mgr.c" "pomodoro_engine.c" "rgb_service.c" "button_service.c" "usb_hid_service.c" "dns_server.c" "shortcut_parser.c" "test_shortcut_parser.c" "oled_service.c" "i2c_manager.c" "network_service.c" "web_server.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES led_strip nvs_flash esp_timer esp_driver_gpio esp_tinyusb driver esp_driver_i2c nixy4__u8g2 esp_wifi esp_http_server cjson)

target_add_binary_data(${COMPONENT_LIB} "web_ui.html" BINARY)
