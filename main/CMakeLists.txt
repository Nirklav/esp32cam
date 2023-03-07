idf_component_register(SRCS "record.c" "time.c" "sdcard.c" "avi.c" "countdown_event.c" "camera.c" "nvs.c" "tcp_server.c" "wifi.c" "main.c"
                    INCLUDE_DIRS "")

target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")
