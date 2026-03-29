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
#include "lwip/sockets.h"


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

extern const unsigned char index_html_start[] asm("_binary_index_html_start");
extern const unsigned char index_html_end[] asm("_binary_index_html_end");

int
root_get_handler (httpd_req_t *req)
{
    size_t size = index_html_end - index_html_start;

    httpd_resp_set_type(req, "text/html");

    if (0 > httpd_resp_send (req, (const char*)index_html_start, size))
    {
        ESP_LOGI("WEB-ERR", "HTTP Response err");
        return -1;
    }

    ESP_LOGI("WEB-INFO", "HTTP root send OK");
    return 0;
}
static const httpd_uri_t uri_handler_3 =
{
    .uri = "/redirect",
    .method = HTTP_GET,
    .handler = root_get_handler,
};

static const httpd_uri_t uri_handler_1 =
{
    .uri = "/quickaccess/config/background.json",
    .method = HTTP_GET,
    .handler = root_get_handler,
};
static const httpd_uri_t uri_handler_2 =
{
    .uri = "/connecttest.txt",
    .method = HTTP_GET,
    .handler = root_get_handler,
};

static const httpd_uri_t uri_handler =
{
    .uri = "/",
    .method = HTTP_GET,
    .handler = root_get_handler,
};

esp_err_t
http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
#if 1
   httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "http://192.168.4.1");
    httpd_resp_send(req, NULL, 0);
#else
    ESP_LOGI ("WEB-INFO", "404 ERR type (%d)", err);
    root_get_handler (req);
#endif
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
        httpd_register_uri_handler(server, &uri_handler_1);
        httpd_register_uri_handler(server, &uri_handler_2);
        httpd_register_uri_handler(server, &uri_handler_3);
        httpd_register_err_handler(server, HTTPD_404_NOT_FOUND,
                                   http_404_error_handler);
    }
    else
    {
        return -1;
    }

    return 0;
}

#if 1
// 수정된 DNS 서버 태스크 (패킷 ID 일치화)
static void dns_server_task(void *pvParameters) {
    uint8_t data[512];
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    struct sockaddr_in servaddr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = htonl(INADDR_ANY),
        .sin_port = htons(53)
    };
    bind(sock, (struct sockaddr *)&servaddr, sizeof(servaddr));

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int length = recvfrom(sock, data, sizeof(data), 0, (struct sockaddr *)&client_addr, &addr_len);
        
        if (length > 12) {
            // 수신 패킷의 ID를 그대로 사용 (매우 중요!)
            data[2] |= 0x80; // QR = 1 (Response)
            data[3] |= 0x80; // RA = 1 (Recursion Available)
            data[7] = 1;     // Answer count = 1

            // 응답 데이터 (모든 질의에 192.168.4.1 응답)
            uint8_t answer[] = {
                0xc0, 0x0c, 0x00, 0x01, 0x00, 0x01,
                0x00, 0x00, 0x00, 0x3c, // TTL 60
                0x00, 0x04, 192, 168, 4, 1
            };
            memcpy(data + length, answer, sizeof(answer));
            sendto(sock, data, length + sizeof(answer), 0, (struct sockaddr *)&client_addr, addr_len);
        }
        vTaskDelay(1);
    }
}
#else

// DNS 서버 태스크: 모든 도메인 질의에 ESP32 IP(192.168.4.1)로 응답
static void dns_server_task(void *pvParameters) {
    uint8_t data[128];
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    struct sockaddr_in servaddr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = htonl(INADDR_ANY),
        .sin_port = htons(53) // DNS 표준 포트
    };
    bind(sock, (struct sockaddr *)&servaddr, sizeof(servaddr));

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int length = recvfrom(sock, data, sizeof(data), 0, (struct sockaddr *)&client_addr, &addr_len);
        if (length > 12) { // DNS 헤더 최소 크기
            data[2] |= 0x80; // Response flag
            data[3] |= 0x80; // Recursion available
            data[7] = 1;     // Answer count = 1
            
            // 응답 패킷 조립 (단순화된 버전: 모든 질의에 192.168.4.1 응답)
            uint8_t answer[] = {
                0xc0, 0x0c,             // Name offset
                0x00, 0x01, 0x00, 0x01, // Type A, Class IN
                0x00, 0x00, 0x00, 0x3c, // TTL 60s
                0x00, 0x04,             // Data length
                192, 168, 4, 1          // IP Address
            };
            memcpy(data + length, answer, sizeof(answer));
            sendto(sock, data, length + sizeof(answer), 0, (struct sockaddr *)&client_addr, addr_len);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
#endif
int
start_dns_server(void)
{
    xTaskCreate(dns_server_task, "dns_server", 4096, NULL, 5, NULL);
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
    snprintf (captive_portal_url, CAPTIVE_PORTAL_URL_SIZE, "%s%s/", "http://", ip_addr);

    /* get dbcp interface */
    netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    if (NULL == netif)
    {
        ESP_LOGI ("MEM-ERR", "Captive portal Memmory allocation fail\n");
        goto _err;
    }

    ESP_LOGI ("WEB-INFO", "CAPTIVE Portal");
    if (dhcp_option_update(netif, ESP_NETIF_CAPTIVEPORTAL_URI,
                           captive_portal_url, sizeof(captive_portal_url),
                           ESP_NETIF_OP_SET))
    {
        goto _err;
    }

    if (start_dns_server())
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
        board.netif_init = TRUE;
    }

    if (board.event_loop == FALSE)
    {
        /* Enable event */
        ESP_ERROR_CHECK(esp_event_loop_create_default());
        board.event_loop = TRUE;
    }

    if (board.nvs_flash == FALSE)
    {
        /* need to init nvs flash for wifi init */
        ESP_ERROR_CHECK(nvs_flash_init());
        board.nvs_flash = TRUE;
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