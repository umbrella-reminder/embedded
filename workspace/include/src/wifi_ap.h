#ifndef __WIFI_AP_H__
#define __WIFI_AP_H__
#include "board.h"

#define DNS_PACKET_SIZE_MAX           512
#define CAPTIVE_PORTAL_URL_SIZE       32

#define WIFI_CAPTIVE_PORTAL_ESP_IDF_DNS_LEN 512

#define WIFI_CAPTIVE_PORTAL_ESP_IDF_DNS_FLAG_RD (1 << 0)
#define WIFI_CAPTIVE_PORTAL_ESP_IDF_DNS_FLAG_TC (1 << 1)
#define WIFI_CAPTIVE_PORTAL_ESP_IDF_DNS_FLAG_AA (1 << 2)
#define WIFI_CAPTIVE_PORTAL_ESP_IDF_DNS_FLAG_QR (1 << 7)

#define WIFI_CAPTIVE_PORTAL_ESP_IDF_DNS_QTYPE_A 1
#define WIFI_CAPTIVE_PORTAL_ESP_IDF_DNS_QTYPE_NS 2
#define WIFI_CAPTIVE_PORTAL_ESP_IDF_DNS_QTYPE_CNAME 5
#define WIFI_CAPTIVE_PORTAL_ESP_IDF_DNS_QTYPE_SOA 6
#define WIFI_CAPTIVE_PORTAL_ESP_IDF_DNS_QTYPE_WKS 11
#define WIFI_CAPTIVE_PORTAL_ESP_IDF_DNS_QTYPE_PTR 12
#define WIFI_CAPTIVE_PORTAL_ESP_IDF_DNS_QTYPE_HINFO 13
#define WIFI_CAPTIVE_PORTAL_ESP_IDF_DNS_QTYPE_MINFO 14
#define WIFI_CAPTIVE_PORTAL_ESP_IDF_DNS_QTYPE_MX 15
#define WIFI_CAPTIVE_PORTAL_ESP_IDF_DNS_QTYPE_TXT 16
#define WIFI_CAPTIVE_PORTAL_ESP_IDF_DNS_QTYPE_URI 256

#define WIFI_CAPTIVE_PORTAL_ESP_IDF_DNS_QCLASS_IN 1
#define WIFI_CAPTIVE_PORTAL_ESP_IDF_DNS_QCLASS_ANY 255
#define WIFI_CAPTIVE_PORTAL_ESP_IDF_DNS_QCLASS_URI 256

typedef struct __attribute__((packed))
{
  uint16_t id;
  uint8_t flags;
  uint8_t rcode;
  uint16_t qdcount;
  uint16_t ancount;
  uint16_t nscount;
  uint16_t arcount;
} dns_header;

typedef struct __attribute__((packed))
{
  uint8_t len;
  uint8_t data;
} dns_label;

typedef struct __attribute__((packed))
{
  uint16_t type;
  uint16_t cl;
} dns_question_footer;

typedef struct __attribute__((packed))
{
  uint16_t type;
  uint16_t cl;
  uint32_t ttl;
  uint16_t rdlength;
} dns_resource_footer;

typedef struct __attribute__((packed))
{
  uint16_t prio;
  uint16_t weight;
} dns_uri_hdr;


int wifi_ap_init (void);
#endif /* __WIFI_AP_H__*/