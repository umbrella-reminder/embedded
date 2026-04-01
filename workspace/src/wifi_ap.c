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
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "http://192.168.4.1/");
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache, no-store, must-revalidate");
    httpd_resp_set_hdr(req, "Pragma", "no-cache");
    httpd_resp_set_hdr(req, "Expires", "0");
    httpd_resp_send(req, NULL, 0);

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

int
dns_recive_hdr_check (dns_header *hdr, int size)
{
    if (NULL == hdr)
    {
        return 0;
    }

    if (size > DNS_PACKET_SIZE_MAX)
    {
        return 0;
    }

    if (size < sizeof (dns_header))
    {
        return 0;
    }

    if (hdr->ancount || hdr->nscount || hdr->arcount)
    {
        return 0;
    }

    if (hdr->flags & WIFI_CAPTIVE_PORTAL_ESP_IDF_DNS_FLAG_TC)
    {
        return 0;
    }

    return 1;
}

static char *label_to_str(char *packet, char *labelPtr, int packetSz, char *res, int resMaxLen)
{
    int i = 0, j, k;
    char *endPtr = NULL;

    while (labelPtr >= packet && (labelPtr - packet) < packetSz && *labelPtr != 0)
    {
        if ((*labelPtr & 0xC0) == 0)
        {
            j = *labelPtr++; 
            if (i < resMaxLen - 1 && i != 0)
            {
                res[i++] = '.';
            }
            
            for (k = 0; k < j; k++)
            {
                if ((labelPtr - packet) >= packetSz)
                {
                    return NULL;
                }
                if (i < resMaxLen - 1)
                {
                    res[i++] = *labelPtr++;
                }
                else
                {
                    labelPtr++;
                }
            }
        }
        else if ((*labelPtr & 0xC0) == 0xC0)
        {
            if (endPtr == NULL)
            {
                endPtr = labelPtr + 2;
            }
            int offset = ntohs(*(uint16_t *)labelPtr) & 0x3FFF;
            if (offset >= packetSz)
            {
                return NULL;
            }
            labelPtr = &packet[offset];
        }
        else
        {
          return NULL;
        }
    }

    res[i] = 0; 
    return (endPtr == NULL) ? labelPtr + 1 : endPtr;
}

static char *str_to_label(char *str, char *label, int maxLen)
{
    char *len = label;   
    char *p = label + 1; 
    
    while (*str != 0)
    {
        if ((p - label) >= maxLen)
        {
            return NULL;
        }

        if (*str == '.')
        {
            *len = (uint8_t)((p - len) - 1);
            len = p;                
            p++;                    
            str++;
        }
        else
        {
            *p++ = *str++;
        }
    }
    
    *len = (uint8_t)((p - len) - 1);
    if ((p - label) >= maxLen)
    {
        return NULL; 
    }
    *p++ = 0; 
    
    return p;
}

int
dns_response_build (struct sockaddr_in *remote, uint8_t *data, int size, uint8_t *resp)
{
    char domain[DNS_PACKET_SIZE_MAX];
    uint16_t question_count, i;

    dns_header *data_hdr = (dns_header *) data;
    dns_header *reply_hdr = (dns_header *) resp;
    char *data_p = NULL;
    char *resp_p = NULL;

    /* check null ptr */
    if (NULL == remote || NULL == data || NULL == resp)
    {
        return 0;
    }

    /* check common dns header */
    if (! dns_recive_hdr_check (data_hdr, size))
    {
        return 0;
    }

    /* copy origin data */
    memcpy (resp, data, size);

    /* set reply packet body */
    resp_p = (char *)resp + size;
 
    /* set dns packet body */
    data_p = (char *)data + sizeof(dns_header);

    /* get reuqest count */
    question_count = ntohs(data_hdr->qdcount);

    reply_hdr->flags |= WIFI_CAPTIVE_PORTAL_ESP_IDF_DNS_FLAG_QR;

    for (i = 0; i < question_count; i++)
    {
        /* dns label change to string */
        void *next_q = label_to_str((char *)data, data_p, size, domain, sizeof(domain));
        if (next_q == NULL) 
        {
            return 0;
        }

        dns_question_footer *dns_footer = (dns_question_footer *)next_q;
        data_p = (char *)(dns_footer + 1);

        ESP_LOGI("DNS-INFO", "CAPTIVE Portal DNS query %s", domain);

        /* change to label */
        char *next_r = str_to_label(domain, resp_p, (DNS_PACKET_SIZE_MAX - (resp_p - (char *)resp)));
        if (next_r == NULL)
        {
            ESP_LOGI("DNS-ERR", "Max size over");
            return 0;
        }

        dns_resource_footer *dns_res_footer = (dns_resource_footer *)next_r;

        resp_p = (char *)(dns_res_footer + 1);

        /* build response packet */
        if (ntohs(dns_footer->type) == WIFI_CAPTIVE_PORTAL_ESP_IDF_DNS_QTYPE_A)
        {
            esp_netif_ip_info_t ip_info;

            dns_res_footer->type = htons(WIFI_CAPTIVE_PORTAL_ESP_IDF_DNS_QTYPE_A);
            dns_res_footer->cl = htons(WIFI_CAPTIVE_PORTAL_ESP_IDF_DNS_QCLASS_IN);
            dns_res_footer->ttl = htonl(0);
            dns_res_footer->rdlength = htons(4);

            /* get wifi ip */
            esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_AP_DEF"), &ip_info);

            /* set wifi ip */
            resp_p[0] = ip4_addr1(&ip_info.ip);
            resp_p[1] = ip4_addr2(&ip_info.ip);
            resp_p[2] = ip4_addr3(&ip_info.ip);
            resp_p[3] = ip4_addr4(&ip_info.ip);
            resp_p += 4;

        }
        else if (ntohs(dns_footer->type) == WIFI_CAPTIVE_PORTAL_ESP_IDF_DNS_QTYPE_NS)
        {
            dns_res_footer->type = htons(WIFI_CAPTIVE_PORTAL_ESP_IDF_DNS_QTYPE_NS);
            dns_res_footer->cl = htons(WIFI_CAPTIVE_PORTAL_ESP_IDF_DNS_QCLASS_IN);
            dns_res_footer->ttl = htonl(0);
            dns_res_footer->rdlength = htons(4);

            /* set wifi ip */
            resp_p[0] = 2;
            resp_p[1] = 'n';
            resp_p[2] = 's';
            resp_p[3] = 0;
            resp_p += 4;
        }
        else if (ntohs(dns_footer->type) == WIFI_CAPTIVE_PORTAL_ESP_IDF_DNS_QTYPE_URI)
        {
            dns_uri_hdr *uri_hdr = (dns_uri_hdr *) resp_p;

            resp_p += sizeof (dns_uri_hdr);

            dns_res_footer->type = htons(WIFI_CAPTIVE_PORTAL_ESP_IDF_DNS_QTYPE_URI);
            dns_res_footer->cl = htons(WIFI_CAPTIVE_PORTAL_ESP_IDF_DNS_QCLASS_URI);
            dns_res_footer->ttl = htonl(0);
            dns_res_footer->rdlength = htons(4 + 16);

            uri_hdr->prio = htons(10);
            uri_hdr->weight = htons(1);

            memcpy (resp_p, "http://esp.nonet", 16);

            resp_p += 16;
        }
        else
        {
            ESP_LOGI ("DNS-INFO", "Type not match");
            return 0;
        }

        reply_hdr->ancount = htons(ntohs(reply_hdr->ancount) + 1);
    }

    return (int) (resp_p - (char *)resp);
}

static void
dns_server_task(void *arg)
{
    unsigned int count;
    int sock = -1, ret;

    uint8_t *data = NULL;
    uint8_t *resp = NULL;
    socklen_t len;

    struct sockaddr_in remote_addr;
    struct sockaddr_in server_addr =
    {
            .sin_family = AF_INET,
            .sin_addr.s_addr = INADDR_ANY,
            .sin_port = htons(53),
            .sin_len = sizeof(struct sockaddr_in),
    };

    /* create socket */
    count = 0;
    do
    {
        sock = socket (AF_INET, SOCK_DGRAM, IPPROTO_IP);
        count++;

        if (count > 100)
        {
            ESP_LOGI ("DNS-ERR", "socket create fail");
            goto _err;
        }

        vTaskDelay (1000 / portTICK_PERIOD_MS);
     } while (sock == -1);

    ESP_LOGI ("DNS-INFO", "create socket(0x%x) for dns server ", sock);

    /* bind socket */
    count = 0;
    do
    {
        ret = bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
        count++;

        if (count > 100)
        {
            ESP_LOGI ("DNS-ERR", "socket bind erorr");
            goto _err;
        }

        vTaskDelay (1000 / portTICK_PERIOD_MS);
    }
    while (ret != 0);

    ESP_LOGI ("DNS-INFO", "create socket(0x%x) bind ok ", sock);

    
    data = malloc (DNS_PACKET_SIZE_MAX);
    if (NULL == data)
    {
        ESP_LOGI ("DNS-ERR", "Memory allocation fail");
        goto _err;
    }

    resp = malloc (DNS_PACKET_SIZE_MAX);
    if (NULL == data)
    {
        ESP_LOGI ("DNS-ERR", "Memory allocation fail");
        goto _err;
    }

    len = sizeof (remote_addr);

    while (1) {
        int size;
        int send_size;
        memset (&remote_addr, 0, len);

        size = recvfrom(sock, data, DNS_PACKET_SIZE_MAX,
                        0, (struct sockaddr *)&remote_addr, (socklen_t *)&len);

        send_size = dns_response_build (&remote_addr, data, size, resp);

        if (size > 0 && send_size > 0)
        {
            ESP_LOGI ("DNS-INFO", "Send DNS Response");
            /* send */
            sendto (sock, resp, send_size, 0, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr_in));
        }
        vTaskDelay(1);
    }

_err:
    if (data)
    {
      free (data);
    }

    close(sock);
    vTaskDelete(NULL);
}

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