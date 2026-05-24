// sd_driver.c
#include "fatfs.h"

FATFS fs;
FIL fil;

void SD_Log_Init(void) {
    f_mount(&fs, "", 1);
}

void SD_Log_Write(const char* data) {
    if (f_open(&fil, "telemetry.csv", FA_WRITE | FA_OPEN_APPEND) == FR_OK) {
        f_puts(data, &fil);
        f_close(&fil);

    }
}
