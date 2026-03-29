
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

    REQUIRES esp_wifi nvs_flash esp_http_server
    EMBED_FILES "web/index.html" "web/assets/favicon.ico" "web/assets/logo.png" "web/data/address.json" "web/main.js"
)