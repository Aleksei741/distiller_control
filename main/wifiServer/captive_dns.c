//******************************************************************************
//include
//******************************************************************************
#include "captive_dns.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include "string.h"
#include "esp_log.h"
//******************************************************************************
// Constants
//******************************************************************************
#define DNS_PORT 53
#define DNS_BUF_SIZE 512
//******************************************************************************
// Type
//******************************************************************************

//******************************************************************************
// Variables
//******************************************************************************
//------------------------------------------------------------------------------
// Global
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Local Variable
//------------------------------------------------------------------------------
static const char *TAG = "[captive_dns]";

static TaskHandle_t dns_task_handle = NULL;
static uint8_t dns_redirect_ip[4] = {192,168,4,1};
static int dns_sock = -1;
static volatile bool dns_running = false;
//------------------------------------------------------------------------------
// Local Class
//------------------------------------------------------------------------------

//******************************************************************************
// Function prototype
//******************************************************************************
static void dns_task(void *pvParameters);
//******************************************************************************
// Function
//******************************************************************************
esp_err_t captive_dns_init(const uint8_t redirect_ip[4])
{
    if (redirect_ip) {
        memcpy(dns_redirect_ip, redirect_ip, 4);
    }

    if (dns_task_handle != NULL) {
        ESP_LOGW(TAG, "DNS task already running");
        return ESP_ERR_INVALID_STATE;
    }

    dns_running = true;

    BaseType_t ret = xTaskCreate(
        dns_task,
        "captive_dns",
        4096,
        NULL,
        5,
        &dns_task_handle
    );

    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create DNS task");
        return ESP_FAIL;
    }

    return ESP_OK;
}
//------------------------------------------------------------------------------
void captive_dns_stop(void)
{
    if (dns_task_handle) {
        dns_running = false;

        if (dns_sock >= 0) {
            shutdown(dns_sock, SHUT_RDWR);
            close(dns_sock);
            dns_sock = -1;
        }

        vTaskDelete(dns_task_handle);
        dns_task_handle = NULL;
        ESP_LOGI(TAG, "Captive DNS server stopped");
    }
}
//------------------------------------------------------------------------------
static void dns_task(void *pvParameters)
{
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        ESP_LOGE(TAG, "Failed to create socket");
        dns_running = false;
        vTaskDelete(NULL);
        return;
    }

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(DNS_PORT),
        .sin_addr.s_addr = htonl(INADDR_ANY)
    };

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        ESP_LOGE(TAG, "Bind failed");
        close(sock);
        dns_sock = -1;
        dns_running = false;
        vTaskDelete(NULL);
        return;
    }

    uint8_t buf[DNS_BUF_SIZE];
    struct sockaddr_in client;
    socklen_t clen = sizeof(client);

    ESP_LOGI(TAG, "Captive DNS server started");

    while (dns_running) {
        int len = recvfrom(sock, buf, sizeof(buf), 0,
                           (struct sockaddr*)&client, &clen);

        if (!dns_running) break; 

        if (len < 12) continue;  // DNS header min size

        uint16_t id = (buf[0] << 8) | buf[1];
        uint16_t flags = (1 << 15) | (1 << 10); // QR=1, AA=1
        uint16_t qdcount = 1;
        uint16_t ancount = 1;

        uint8_t resp[DNS_BUF_SIZE];
        int p = 0;

        // header
        resp[p++] = id >> 8;
        resp[p++] = id & 0xFF;
        resp[p++] = flags >> 8;
        resp[p++] = flags & 0xFF;
        resp[p++] = qdcount >> 8;
        resp[p++] = qdcount & 0xFF;
        resp[p++] = ancount >> 8;
        resp[p++] = ancount & 0xFF;
        resp[p++] = 0; resp[p++] = 0; // NSCOUNT
        resp[p++] = 0; resp[p++] = 0; // ARCOUNT

        // copy query section
        int qlen = len - 12;
        memcpy(resp + p, buf + 12, qlen);
        int qname_offset = 12;
        p += qlen;

        // answer section
        resp[p++] = 0xC0;
        resp[p++] = qname_offset;   // pointer to QNAME
        resp[p++] = 0x00; resp[p++] = 0x01; // TYPE A
        resp[p++] = 0x00; resp[p++] = 0x01; // CLASS IN
        resp[p++] = 0x00; resp[p++] = 0x00;
        resp[p++] = 0x00; resp[p++] = 0x3C; // TTL=60
        resp[p++] = 0x00; resp[p++] = 0x04; // IPv4 length
        memcpy(resp + p, dns_redirect_ip, 4);
        p += 4;

        sendto(sock, resp, p, 0, (struct sockaddr*)&client, clen);
    }

    if (dns_sock >= 0) {
        close(dns_sock);
        dns_sock = -1;
    }

    dns_task_handle = NULL;
    dns_running = false;

    vTaskDelete(NULL);
}
//------------------------------------------------------------------------------
