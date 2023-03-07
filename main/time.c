#include "private_include/time.h"
#include <time.h>
#include <sys/time.h>
#include "esp_sntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

static EventGroupHandle_t time_wait_event;

static void time_sync(struct timeval *tv)
{
    if (time_wait_event != NULL)
        xEventGroupSetBits(time_wait_event, 1);
}

void time_setup()
{
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
    sntp_set_time_sync_notification_cb(time_sync);
    sntp_init();

    time_wait_event = xEventGroupCreate();
    while (true)
    {
        EventBits_t bits = xEventGroupWaitBits(time_wait_event, 1, pdTRUE, pdFALSE, 100);
        if (bits == 1)
            break;
    }
    
    vEventGroupDelete(time_wait_event);
    time_wait_event = NULL;
}

void time_str(char* buf, size_t len)
{
    time_t now = time(NULL);
    struct tm* gmnow = gmtime(&now);
    strftime(buf, len, "%Y_%m_%d_%H_%M_%S", gmnow);
}
