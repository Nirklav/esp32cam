#pragma once

#include <stdatomic.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

typedef struct CountdownEvent {
    int max;
    atomic_int counter;
    EventGroupHandle_t event;
} CountdownEvent;

bool countdown_event_create(int max, CountdownEvent* rEvent);

bool countdown_event_decrement(CountdownEvent* event);
void countdown_event_reset(CountdownEvent* event);

void countdown_event_wait_max(CountdownEvent* event);
void countdown_event_wait_zero(CountdownEvent* event);