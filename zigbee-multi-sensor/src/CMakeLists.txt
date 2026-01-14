idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       REQUIRES esp-zigbee-lib esp-zboss-lib driver nvs_flash led_strip)
