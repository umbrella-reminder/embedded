
set(source_dirs
    "src/main.c"
    "src/wifi_ap.c"
)

set(include_dirs
    "include"
    "include/src"
)

idf_component_register(
    SRCS "${source_dirs}"
    INCLUDE_DIRS "${include_dirs}"


    REQUIRES esp_wifi nvs_flash esp_http_server spiffs
)