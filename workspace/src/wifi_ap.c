/* common include */
#include "board.h"

#ifdef WIFI_AP_ENABLE

/* application header */
#include "wifi_ap.h"

/* device header */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_mac.h"

char ssid_header[] = "ESP32-AP-";
char wifi_ap_password[] = "12345678"; /* initial password. it can be changed. */

static void
wifi_ap_event_handler (void *arg, esp_event_base_t base,
                       int32_t id, void *data)
{
    switch (id)
    {
    case WIFI_EVENT_AP_STACONNECTED:
        break;
    case WIFI_EVENT_AP_STADISCONNECTED:
        break;
    default:
        return;
    }
}

int
wifi_ap_deinit (void)
{
    int rv = ESP_OK;

    return rv;
}

int
wifi_config_t_init (wifi_config_t *config)
{
    unsigned char mac[6];
    char ssid[32];

    if (NULL == config)
    {
      return -1;
    }

    esp_read_mac(mac, ESP_MAC_WIFI_STA);

    snprintf (ssid, sizeof(ssid), "%s%02x%02x", ssid_header, mac[4], mac[5]);

    snprintf((char *)config->ap.ssid, sizeof(config->ap.ssid), "%s", ssid);

    snprintf((char *)config->ap.password, sizeof(config->ap.password), "%s", wifi_ap_password);

    config->ap.ssid_len = strlen (ssid);

    config->ap.channel = 1; /* temp */

    config->ap.max_connection = 3;

    config->ap.authmode = WIFI_AUTH_WPA2_PSK;

    config->ap.pmf_cfg.required = TRUE;

    config->ap.bss_max_idle_cfg.period = 10;

    config->ap.bss_max_idle_cfg.protected_keep_alive = 1;

    return 0;
}

int
wifi_ap_init (void)
{
    int rv = 0;

    /* set default configuration */
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();

    /* wifi config */
    wifi_config_t wifi_config = {0,};

    if (board.netif_init == FALSE)
    {
        /* enable tcp/ip stack */
        ESP_ERROR_CHECK(esp_netif_init());
    }

    if (board.event_loop == FALSE)
    {
        /* Enable event */
        ESP_ERROR_CHECK(esp_event_loop_create_default());
    }

    /* register wifi ap event handler for default interface.
     * in this function, create default netif and associated with wifi ap */
    esp_netif_create_default_wifi_ap();

    /* wifi init */
    ESP_ERROR_CHECK(esp_wifi_init(&config));

    /* wifi event callback register */
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_ap_event_handler,
                                                        NULL,
                                                        NULL));


    /* ap mode enable for config */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));

    /* wifi config structure init for fiting our system */
    ESP_ERROR_CHECK(wifi_config_t_init(&wifi_config));

    /* Push AP configuration */
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));

    /* wifi on */
    ESP_ERROR_CHECK(esp_wifi_start());

    return rv;
}
#endif /* WIFI_AP_ENABLE */