#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <unistd.h>
#include <esp_netif.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cJson.h>

#include "private_include/wifi.h"
#include "private_include/nvs.h"
#include "private_include/tcp_server.h"
#include "private_include/camera.h"
#include "private_include/countdown_event.h"
#include "private_include/sdcard.h"
#include "private_include/avi.h"
#include "private_include/time.h"
#include "private_include/record.h"

static const char* TAG = "application";
#define RECORDS_PATH MOUNT_POINT"/rec"

void files(int sock)
{
    size_t buffer_len = 1500;
    uint8_t* buffer = calloc(buffer_len, sizeof(uint8_t));

    TcpServerData input = { 0, 0, buffer, buffer_len };
    if (tcp_server_read(sock, &input) && tcp_server_check_password(&input))
    {
        char* char_buffer = (char*)buffer;
        strcpy(char_buffer, "{\"files\":[");

        Records* records = record_read_all(RECORDS_PATH);
        if (records != NULL)
        {
            for (int i = 0; i < records->len; i++)
            {
                if (i > 0)
                    strcat(char_buffer, ",");

                strcat(char_buffer, "\"");
                strcat(char_buffer, records->array[i].name);
                strcat(char_buffer, "\"");
            }
            
            free(records);
        }

        strcat(char_buffer, "]}");

        TcpServerData output = { 1, strlen(char_buffer), buffer, buffer_len };
        tcp_server_write(sock, &output);
    }

    free(buffer);
}

void stream_file(int sock)
{
    uint8_t buffer[128];

    TcpServerData input = { 0, 0, buffer, sizeof(buffer) };
    if (tcp_server_read(sock, &input))
    {
        cJSON* root = tcp_server_parse_and_check_password(&input);
        if (root == NULL)
            return;

        char* file_name = cJSON_GetObjectItem(root, "file")->valuestring;
        if (file_name == NULL)
            return;

        char name[40];
        strcpy(name, RECORDS_PATH);
        strcat(name, "/");
        strcat(name, file_name);

        cJSON_Delete(root);

        ESP_LOGI(TAG, "Request file name: %s", name);

        struct stat st;
        if (stat(name, &st) != 0)
            return;

        FILE *file = fopen(name, "r");
        if (file == NULL)
            return;

        to_bytes(st.st_size, buffer, 0);
        to_bytes(CONTENT_TYPE_BIN, buffer, 4);
        if (!tcp_server_write_raw(sock, buffer, 8))
            return;

        size_t file_buffer_len = 16384;
        uint8_t* file_buffer = calloc(file_buffer_len, sizeof(uint8_t));
        while (true)
        {
            size_t read = fread(file_buffer, sizeof(uint8_t), file_buffer_len, file);
            if (read == 0)
                break;

            if (!tcp_server_write_raw(sock, file_buffer, read))
                break;
        }
        free(file_buffer);
    }
}

void stream_camera(int sock)
{
    uint8_t buffer[128];
    TcpServerData input = { 0, 0, buffer, sizeof(buffer) };

    if (tcp_server_read(sock, &input) && tcp_server_check_password(&input))
    {
        CameraListener listener = { false, NULL };
        while (true)
        {
            camera_wait(&listener);

            to_bytes(listener.fb->len, buffer, 0);
            
            if (!tcp_server_write_raw(sock, buffer, 4))
                break;

            if (!tcp_server_write_raw(sock, listener.fb->buf, listener.fb->len))
                break;
            
            camera_return(&listener);
        }

        camera_release(&listener);
    }
}

void restart(int sock)
{
    uint8_t buffer[128];
    TcpServerData input = { 0, 0, buffer, sizeof(buffer) };

    if (tcp_server_read(sock, &input) && tcp_server_check_password(&input))
    {
        char* char_buffer = (char*)buffer;
        strcpy(char_buffer, "{\"r\":\"OK\"}");
        TcpServerData output = { 1, strlen(char_buffer), buffer, sizeof(buffer) };
        tcp_server_write(sock, &output);
        esp_restart();
    }
}

TcpServer* tcp_server_create()
{
    const int size = 4;

    TcpServer *tcp_server = malloc(sizeof (TcpServer));

    TcpServerMethod files_method = {0, false, &files};
    TcpServerMethod stream_file_method = {1, true, &stream_file};
    TcpServerMethod stream_camera_method = {2, true, &stream_camera};
    TcpServerMethod restart_method = {3, false, &restart};

    tcp_server->methods = calloc(size, sizeof(TcpServerMethod));
    tcp_server->methods[0] = files_method;
    tcp_server->methods[1] = stream_file_method;
    tcp_server->methods[2] = stream_camera_method;
    tcp_server->methods[3] = restart_method;
    tcp_server->methods_len = size;

    return tcp_server;
}

static void delete_old_files()
{
    const int max_files = 40;

    Records* records = record_read_all(RECORDS_PATH);
    if (records == NULL)
        return;

    record_sort(records);

    int left = max_files;
    for (int i = records->len - 1; i >= 0 ; i--)
    {
        char name[40];
        strcpy(name, RECORDS_PATH);
        strcat(name, "/");
        strcat(name, records->array[i].name);

        struct stat st;
        if (stat(name, &st) != 0)
            continue;

        if (st.st_size == 0)
        {
            int r = unlink(name);
            ESP_LOGI(TAG, "Deleted empty: %s (%d)", name, r);
        }
        else if (left <= 0)
        {
            int r = unlink(name);
            ESP_LOGI(TAG, "Deleted: %s (%d)", name, r);
        }
        else
            left --;
    }

    free(records);
}

static void write_next_file(int fps, int frames)
{
    char path[64];
    char time[48];
    time_str(time, sizeof(time));
    strcpy(path, RECORDS_PATH);
    strcat(path, "/");
    strcat(path, time);
    strcat(path, ".avi");
    ESP_LOGI(TAG, "Start writing file: \"%s\"", path);

    FILE *file = fopen(path, "w");
    if (file == NULL) 
    {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return;
    }

    CameraListener listener = { false, NULL };
    uint32_t left = frames;
    Avi avi = avi_mjpeg_start(fps, frames, 1600, 1200, file);
    while (true)
    {
        camera_wait(&listener);
        avi_mjpeg_frame(&avi, listener.fb->buf, listener.fb->len, file);
        camera_return(&listener);

        fflush(file);
        left--;
        if (left <= 0)
            break;
    }

    avi_close(&avi, file);
    camera_release(&listener);
    fclose(file);
}

static void write_records_task(void *pvParameters)
{
    const uint32_t fps = 5;
    const uint32_t time_sec = 300;
    const uint32_t frames = fps * time_sec;

    while (true)
    {
        delete_old_files();
        write_next_file(fps, frames);
    }
    
    vTaskDelete(NULL);
}

static void start_write_records()
{
    xTaskCreate(write_records_task, "records", 4096, NULL, 5, NULL);
}

void app_main()
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    nvs_initialize();
    sdcard_mount();
    
    bool connected = wifi_connect();
    if (!connected)
    {
        esp_restart();
        return;
    }

    time_setup();

    esp_err_t initialized = camera_init();
    if (initialized != ESP_OK)
    {
        esp_restart();
        return;
    }

    tcp_server_start(tcp_server_create());
    start_write_records();
    camera_loop();
}