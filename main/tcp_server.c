#include "private_include/tcp_server.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#define KEEPALIVE_IDLE 5
#define KEEPALIVE_INTERVAL 5
#define KEEPALIVE_COUNT 3

static const char *TAG = "tcp-server";

uint32_t to_int(uint8_t* buffer, int pos)
{
    return buffer[pos] + (buffer[pos + 1] << 8) + (buffer[pos + 2] << 16) + (buffer[pos + 3] << 24);
}

void to_bytes(uint32_t value, uint8_t* buffer, int pos)
{
    buffer[pos] = (uint8_t)(value & 0xFF);
    buffer[pos + 1] = (uint8_t)(value >> 8 & 0xFF);
    buffer[pos + 2] = (uint8_t)(value >> 16 & 0xFF);
    buffer[pos + 3] = (uint8_t)(value >> 24 & 0xFF);
}

bool tcp_server_read_raw(const int sock, uint8_t* buffer, int len)
{
    int read = recv(sock, buffer, len, 0);
    if (read < 0)
    {
        ESP_LOGE(TAG, "Error occurred during receiving: errno %d", errno);
        return false;
    }
    else if (read == 0)
    {
        ESP_LOGW(TAG, "Connection closed");
        return false;
    }
        
    return true;
}

bool tcp_server_write_raw(const int sock, uint8_t* buffer, int len)
{
    int to_write = len;
    while (to_write > 0) 
    {
        int written = send(sock, buffer + (len - to_write), to_write, 0);
        if (written < 0)
        {
            ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
            return false;
        }
        to_write -= written;
    }

    return true;
}

static void tcp_server_call_stream(void *pvParameters)
{
    void** parameters = (void**)pvParameters;
    TcpServerMethod* method = (TcpServerMethod*)parameters[0];
    int sock = (int)parameters[1];

    method->func(sock);

    free(pvParameters);
    shutdown(sock, 0);
    close(sock);
    vTaskDelete(NULL);
}

static void tcp_server_call_method(const int sock, const TcpServer* server)
{
    uint8_t buffer[4];

    if (tcp_server_read_raw(sock, buffer, 4))
    {
        uint32_t id = to_int(buffer, 0);
        ESP_LOGI(TAG, "Call id: %d", id);

        int len = server->methods_len;
        for (int i = 0; i < len; i++)
        {
            TcpServerMethod method = server->methods[i];
            if (method.id != id)
                continue;

            if (method.is_stream)
            {
                void** parameters = calloc(2, sizeof(void*));
                parameters[0] = (void*)&method;
                parameters[1] = (void*)sock;
                xTaskCreate(tcp_server_call_stream, "stream_method", 4096, parameters, 5, NULL);

                // Don't close the socket
                return;
            }
            else
            {
                method.func(sock);
                break;
            }
        }
    }
    
    shutdown(sock, 0);
    close(sock);
}

static void tcp_server_task(void *pvParameters)
{
    esp_task_wdt_add(NULL);

    char addr_str[128];
    TcpServer* server = (TcpServer*)pvParameters;
    int addr_family = AF_INET;
    int ip_protocol = IPPROTO_IP;
    int keepAlive = 1;
    int keepIdle = KEEPALIVE_IDLE;
    int keepInterval = KEEPALIVE_INTERVAL;
    int keepCount = KEEPALIVE_COUNT;
    struct sockaddr_storage dest_addr;

    struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
    dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr_ip4->sin_family = AF_INET;
    dest_addr_ip4->sin_port = htons(CONFIG_TCP_SERVER_PORT);

    int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (listen_sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        free(server);
        vTaskDelete(NULL);
        return;
    }

    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    ESP_LOGI(TAG, "Socket created");

    int flags = fcntl(listen_sock, F_GETFL); 
    if (fcntl(listen_sock, F_SETFL, flags | O_NONBLOCK) == -1) 
    { 
        ESP_LOGE(TAG, "Unable to set socket non blocking %d", errno);
        goto CLEAN_UP;
    }   

    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) 
    {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        ESP_LOGE(TAG, "IPPROTO: %d", addr_family);
        goto CLEAN_UP;
    }
    ESP_LOGI(TAG, "Socket bound, port %d", CONFIG_TCP_SERVER_PORT);

    err = listen(listen_sock, 1);
    if (err != 0) 
    {
        ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
        goto CLEAN_UP;
    }

    struct pollfd pollfd;
    pollfd.fd = listen_sock;
    pollfd.events = POLLIN | POLLPRI;

    while (1) 
    {
        esp_task_wdt_reset();

        int r = poll(&pollfd, 1, 1000);
        if (r > 0 && pollfd.revents & POLLIN)
        {
            struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
            socklen_t addr_len = sizeof(source_addr);

            int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
            if (sock < 0) 
            {
                ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
                continue;
            }

            // Set tcp keepalive option
            setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int));
            setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(int));
            setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(int));
            setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(int));

            // Convert ip address to string
            if (source_addr.ss_family == PF_INET) 
                inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);

            ESP_LOGI(TAG, "Socket accepted ip address: %s", addr_str);

            tcp_server_call_method(sock, server);
        }
    }

CLEAN_UP:
    free(server);
    close(listen_sock);
    vTaskDelete(NULL);
}

void tcp_server_start(TcpServer* server)
{
    xTaskCreate(tcp_server_task, "tcp_server", 4096, (void*)server, 5, NULL);
}

bool tcp_server_read(const int sock, TcpServerData* input)
{
    uint8_t buffer[8];

    if (!tcp_server_read_raw(sock, buffer, sizeof(buffer)))
        return false;

    input->content_len = to_int(buffer, 0);
    input->content_type = to_int(buffer, 4);
    if (input->content_len > (input->buffer_len - 1))
        return false;

    memset(input->buffer, 0, input->buffer_len);
    return tcp_server_read_raw(sock, input->buffer, input->content_len);
}

bool tcp_server_write(const int sock, TcpServerData* output)
{
    uint8_t buffer[8];

    to_bytes(output->content_len, buffer, 0);
    to_bytes(output->content_type, buffer, 4);

    if (!tcp_server_write_raw(sock, buffer, sizeof(buffer)))
        return false;

    if (!tcp_server_write_raw(sock, output->buffer, output->content_len))
        return false;

    return true;
}

cJSON* tcp_server_parse_and_check_password(TcpServerData* input)
{
    if (input == NULL)
    {
        ESP_LOGE(TAG, "Invalid password: input is empty");
        return NULL;
    }

    if (input->content_type != CONTENT_TYPE_JSON)
    {
        ESP_LOGE(TAG, "Invalid password: input is not json");
        return NULL;
    }

    char* str = (char*) input->buffer;
    cJSON* root = cJSON_Parse(str);
    cJSON* password = cJSON_GetObjectItem(root, "password");
    if (password == NULL || password->valuestring == NULL)
    {
        ESP_LOGE(TAG, "Invalid password: absent");
        cJSON_Delete(root);
        return NULL;
    }

    if (strcmp(password->valuestring, CONFIG_TCP_SERVER_PASSWORD) != 0)
    {
        ESP_LOGE(TAG, "Invalid password: %s", password->valuestring);
        cJSON_Delete(root);
        return NULL;
    }

    return root;
}

bool tcp_server_check_password(TcpServerData* input)
{
    cJSON* root = tcp_server_parse_and_check_password(input);
    if (root != NULL)
    {
        cJSON_Delete(root);
        return true;
    }
    
    return false;
}