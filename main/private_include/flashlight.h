#pragma once

#include <stdbool.h>

#define FLASHLIGHT_GPIO 4

void flashlight_configure();
void flashlight_set(bool enabled);