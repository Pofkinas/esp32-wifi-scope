#include <stdio.h>
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "esp_mac.h"
#include "esp_psram.h"
#include "esp_heap_caps.h"
#include "esp_rom_sys.h"

void app_main(void)
{
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    uint32_t flash_size = 0;
    esp_flash_get_size(NULL, &flash_size);

    size_t psram_heap = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    size_t psram_total = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);

    printf("\n=== ESP32 BOARD INFORMATION ===\n");
    printf("Chip model: ESP32-S3\n");
    printf("CPU cores: %d\n", chip_info.cores);
    printf("Chip revision: %d\n", chip_info.revision);
    printf("Flash size: %lu MB\n", (unsigned long)(flash_size / (1024 * 1024)));
    printf("PSRAM total heap: %lu bytes\n", (unsigned long)psram_total);
    printf("PSRAM free heap: %lu bytes\n", (unsigned long)psram_heap);
    printf("Free internal RAM: %lu bytes\n", (unsigned long)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    printf("Free 8-bit heap: %lu bytes\n", (unsigned long)heap_caps_get_free_size(MALLOC_CAP_8BIT));

    /* CPU frequency */
    printf("CPU frequency: %ld MHz\n", esp_rom_get_cpu_ticks_per_us());
    printf("================================\n\n");

    while(1) {}
}