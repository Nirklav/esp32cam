#include "private_include/countdown_event.h"

#define BIT_ZERO ( 1 << 0 )
#define BIT_MAX ( 1 << 1 )

const TickType_t wait = 500 / portTICK_PERIOD_MS;

bool countdown_event_create(int max, CountdownEvent* rEvent)
{
    EventGroupHandle_t event_group = xEventGroupCreate();
    if (event_group == NULL)
        return false;

    CountdownEvent e = { max, max, event_group };
    *rEvent = e;
    return true;
}

bool countdown_event_decrement(CountdownEvent* event)
{
    while (true)
    {
        atomic_int current = atomic_load(&event->counter);
        if (current <= 0)
            return false;
        
        int dec = current - 1;
        int r = atomic_compare_exchange_weak(&event->counter, &current, dec);
        if (r == current)
        {
            if (dec == 0)
            {
                xEventGroupSetBits(event->event, BIT_ZERO);
                xEventGroupClearBits(event->event, BIT_MAX);
            }
            return true;
        }
    }
}

void countdown_event_reset(CountdownEvent* event)
{
    atomic_store(&event->counter, event->max);
    xEventGroupSetBits(event->event, BIT_MAX);
    xEventGroupClearBits(event->event, BIT_ZERO);
}

static void countdown_event_wait(CountdownEvent* event, EventBits_t bits, int* value, bool can_be_zero)
{
    while (true)
    {
        atomic_int current = atomic_load(&event->counter);
        int v = *value;

        if (current == v)
        {
            if (v != 0)
                break;

            if (v == 0 && can_be_zero)
                break;
        }

        xEventGroupWaitBits(event->event, bits, pdFALSE, pdFALSE, wait);
    }
}

void countdown_event_wait_max(CountdownEvent* event)
{
    countdown_event_wait(event, BIT_MAX, &event->max, false);
}

void countdown_event_wait_zero(CountdownEvent* event)
{
    int zero = 0;
    countdown_event_wait(event, BIT_ZERO, &zero, true);
}