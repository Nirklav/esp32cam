#pragma once

#include <stdint.h>
#include <stddef.h>

typedef struct Record {
    char name[30];

    uint8_t sec;
    uint8_t min;
    uint8_t hour;

    uint8_t day;
    uint8_t month;
    uint16_t year;
} Record;

typedef struct Records {
    Record array[50];
    size_t len;
} Records;

Records* record_read_all(char* dir_path);

void record_sort(Records* records);