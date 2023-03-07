#pragma once

#include "sdmmc_cmd.h"

#define MOUNT_POINT "/sd"

sdmmc_card_t* sdcard_mount();

void sdcard_unmount(sdmmc_card_t* card);