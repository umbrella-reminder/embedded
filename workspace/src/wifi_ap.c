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
#include "esp_http_server.h"
#include "nvs_flash.h"
#include "esp_netif.h"


char *captive_portal_url = NULL;
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
dhcp_option_update (esp_netif_t *dhcp_if, int option, void *op_v, unsigned int op_len, int oper)
{
    if (NULL == dhcp_if)
    {
        return -1;   
    }

    /* update option */
    ESP_ERROR_CHECK(esp_netif_dhcps_stop(dhcp_if));
    ESP_ERROR_CHECK(esp_netif_dhcps_option(dhcp_if, oper, option, op_v, op_len));
    ESP_ERROR_CHECK(esp_netif_dhcps_start(dhcp_if));

    return 0;
}

int
root_get_handler (httpd_req_t *req)
{
    FILE* fp = fopen("web/index.html", "r");
    char chunk[512];
    size_t read_bytes;

    if (fp == NULL)
    {
        httpd_resp_send_404(req);
    }

    httpd_resp_set_type(req, "text/html");
    while ((read_bytes = fread(chunk, 1, sizeof(chunk), fp)) > 0)
    {
        if (httpd_resp_send_chunk(req, chunk, read_bytes) != ESP_OK)
        {
            fclose(fp);
            return ESP_FAIL;
        }
    }

    fclose(fp);
    return 0;
}

static const httpd_uri_t uri_handler =
{
    .uri = "/",
    .method = HTTP_GET,
    .handler = root_get_handler,
};

esp_err_t
http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_send(req, "Redirect to the captive portal", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

int
start_web_server (void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  
    config.max_open_sockets = 2;
    config.lru_purge_enable = TRUE;

    if (httpd_start (&server, &config) == ESP_OK)
    {
        ESP_LOGI ("WEB-START", "Web registering");

        httpd_register_uri_handler(server, &uri_handler);
        httpd_register_err_handler(server, HTTPD_404_NOT_FOUND,
                                   http_404_error_handler);
    }
    else
    {
        return -1;
    }

    return 0;
}

int
captive_portal_start (void)
{
    char ip_addr[16];

    esp_netif_t *netif = NULL;
    esp_netif_ip_info_t ip_info;

    /* get currnet ip address for wifi ap */
    esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_AP_DEF"), &ip_info);
    inet_ntoa_r(ip_info.ip.addr, ip_addr, 16);

    /* memory allocation for url string */
    captive_portal_url = malloc (CAPTIVE_PORTAL_URL_SIZE * sizeof(char));
    if (NULL == captive_portal_url)
    {
        ESP_LOGI ("MEM-ERR", "Captive portal Memmory allocation fail\n");
        return -1;
    }

    /* build url string with ip address */
    snprintf (captive_portal_url, CAPTIVE_PORTAL_URL_SIZE, "%s%s", "http://", ip_addr);

    /* get dbcp interface */
    netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    if (NULL == netif)
    {
        ESP_LOGI ("MEM-ERR", "Captive portal Memmory allocation fail\n");
        goto _err;
    }

    if (dhcp_option_update(netif, ESP_NETIF_CAPTIVEPORTAL_URI,
                           captive_portal_url, sizeof(captive_portal_url),
                           ESP_NETIF_OP_SET))
    {
        goto _err;
    }

    if (start_web_server ())
    {
        goto _err;
    }

    return 0;

_err:
    free(captive_portal_url);
    return -1;
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

    if (board.nvs_flash == FALSE)
    {
        /* need to init nvs flash for wifi init */
        ESP_ERROR_CHECK(nvs_flash_init());
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

    /* captive portal init */
    ESP_ERROR_CHECK(captive_portal_start());

    return rv;
}
#endif /* WIFI_AP_ENABLE */