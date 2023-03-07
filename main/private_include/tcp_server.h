#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <cJson.h>

#define CONTENT_TYPE_JSON (1)
#define CONTENT_TYPE_BIN (2)

typedef struct TcpServerMethod {
    uint32_t id;
    bool is_stream;
    void (*func)(int sock);
} TcpServerMethod;

typedef struct TcpServer {
    TcpServerMethod* methods;
    int methods_len;
} TcpServer;

typedef struct TcpServerData {
    int content_type;
    int content_len;
    uint8_t* buffer;
    int buffer_len;
} TcpServerData;

void tcp_server_start(TcpServer* server);
bool tcp_server_read(const int sock, TcpServerData* input);
bool tcp_server_write(const int sock, TcpServerData* output);

bool tcp_server_read_raw(const int sock, uint8_t* buffer, int len);
bool tcp_server_write_raw(const int sock, uint8_t* buffer, int len);

cJSON* tcp_server_parse_and_check_password(TcpServerData* input);
bool tcp_server_check_password(TcpServerData* input);

uint32_t to_int(uint8_t* buffer, int pos);
void to_bytes(uint32_t value, uint8_t* buffer, int pos);