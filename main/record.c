#include "private_include/record.h"
#include <string.h>
#include <dirent.h>
#include <esp_log.h>

static const char* TAG = "record";

static uint16_t parse(char* str, size_t offset, size_t len)
{
    uint16_t num = 0;
    uint16_t rank = 1;

    int o = offset;
    int s = offset + len - 1;
    for (int i = s; i >= o; i--)
    {
        int n = (str[i] - '0');
        num += n * rank;
        rank *= 10;
    }

    return num;
}

static Record record_parse(char* path)
{
    Record r;
    strcpy(r.name, path);
    r.year = parse(path, 0, 4);
    r.month = parse(path, 5, 2);
    r.day = parse(path, 8, 2);
    r.hour = parse(path, 11, 2);
    r.min = parse(path, 14, 2);
    r.sec = parse(path, 17, 2);
    return r;
}

Records* record_read_all(char* dir_path)
{
    DIR* dir = opendir(dir_path);
    if (dir != NULL)
    {
        Records* records = malloc(sizeof(Records));
        records->len = 0;

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL)
        {
            if (strcmp(entry->d_name, ".") == 0)
                continue;

            if (strcmp(entry->d_name, "..") == 0)
                continue;

            Record r = record_parse(entry->d_name);
            records->array[records->len] = r;
            records->len++;
        }

        closedir(dir);
        return records;
    }
    else
    {
        ESP_LOGE(TAG, "Couldn't open the directory %s", dir_path);
        return NULL;
    }
}

static int cmp(const void* l, const void* r)
{
    Record* left = (Record*)l;
    Record* right = (Record*)r;

    if (left->year != right->year)
        return left->year - right->year;

    if (left->month != right->month)
        return left->month - right->month;
    
    if (left->day != right->day)
        return left->day - right->day;

    if (left->hour != right->hour)
        return left->hour - right->hour;

    if (left->min != right->min)
        return left->min - right->min;

    return left->sec - right->sec;
}

void record_sort(Records* records)
{
    qsort(records->array, records->len, sizeof(Record), cmp);
}