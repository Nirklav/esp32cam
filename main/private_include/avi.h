#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

typedef struct AviElement {
    long size_position;
} AviElement;

typedef struct Avi {
    AviElement avi;
    AviElement movi;
} Avi;

Avi avi_mjpeg_start(uint32_t fps, uint32_t frames, uint16_t width, uint16_t height, FILE* file);

void avi_mjpeg_frame(Avi* avi, uint8_t* buffer, size_t len, FILE* file);

void avi_close(Avi* avi, FILE* file);