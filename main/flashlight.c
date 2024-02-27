#include <stdbool.h>
#include "driver/gpio.h"

#include "private_include/flashlight.h"

void flashlight_configure()
{
    gpio_reset_pin(FLASHLIGHT_GPIO);
    gpio_set_direction(FLASHLIGHT_GPIO, GPIO_MODE_OUTPUT);
}

void flashlight_set(bool enabled)
{
    gpio_set_level(FLASHLIGHT_GPIO, enabled ? 1 : 0);
}