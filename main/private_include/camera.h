#pragma once

#include <stdbool.h>
#include "esp_camera.h"

typedef struct CameraListener {
    bool registered;
    camera_fb_t* fb;
} CameraListener;

esp_err_t camera_init();

void camera_loop();

void camera_wait(CameraListener* listener);
void camera_return(CameraListener* listener);
void camera_release(CameraListener* listener);