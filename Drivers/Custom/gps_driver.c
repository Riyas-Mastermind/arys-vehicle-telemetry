// gps_driver.c
#include "main.h"

char gps_rx_buffer[128];

// Called when UART receives data
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        // Simple NMEA Check: Look for $GPGGA
        if (strstr(gps_rx_buffer, "$GPGGA")) {
            // Parse logic (e.g., extract latitude/longitude)
            // Use sscanf to extract coordinates
        }
    }
}
