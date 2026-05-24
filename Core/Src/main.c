#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "cmsis_os.h"

/* --- SYSTEM CONSTANTS --- */
#define WHEEL_RADIUS_M        0.3f
#define PI                    3.14159f
#define FAULT_FLAG_GPS_LOSS   0x01
#define FAULT_FLAG_IMU_DISC   0x02
#define FAULT_FLAG_CAN_TMOUT  0x04
#define FAULT_FLAG_SD_FAIL    0x08

/* --- DATA STRUCTURES --- */
// Struct for inter-task communication (sent to SD Card and BLE Queues)
typedef struct {
    uint32_t timestamp_ms;
    
    // Calculated Data
    float speed_kmh;
    float distance_m;
    float g_force;
    float heading_deg;
    float lap_time_s;
    
    // Raw Data
    int16_t accel_x, accel_y, accel_z;
    uint16_t rpm;
    float gps_lat, gps_lon;
} TelemetryPacket_t;

/* --- RTOS HANDLES --- */
osThreadId_t SensorTaskHandle, GPSTaskHandle, CANTaskHandle;
osThreadId_t LoggingTaskHandle, TelemetryTaskHandle, FaultTaskHandle;

osMessageQueueId_t LogQueueHandle;       // Queue for SD Card writes
osMessageQueueId_t StreamQueueHandle;    // Queue for BLE transmission
osEventFlagsId_t FaultEventsHandle;      // Flags for system faults
osMutexId_t StateMutexHandle;            // Protects shared variables

/* --- GLOBAL SHARED STATE --- */
TelemetryPacket_t current_state = {0};
uint32_t system_start_time = 0;
uint32_t lap_start_time = 0;

/* --- SIMULATED HARDWARE FUNCTIONS --- */
// In reality, these would be HAL_SPI_Transmit, HAL_I2C_Mem_Read, etc.
void I2C_Read_IMU(int16_t* ax, int16_t* ay, int16_t* az) {
    *ax = (rand() % 2000) - 1000; // Simulated raw values
    *ay = (rand() % 2000) - 1000;
    *az = 9800 + ((rand() % 500) - 250); // Gravity is 1G (9.8 m/s^2)
}

uint16_t ADC_Read_RPM(void) { return 2500 + (rand() % 500); }
bool SPI_Write_SD(TelemetryPacket_t* pkt) { return (rand() % 100) > 2; } // 3% chance of failure

/* ==================================================================== */
/*                         RTOS TASK DEFINITIONS                        */
/* ==================================================================== */

/* 1. SENSOR ACQUISITION TASK (IMU + ADC) - 100Hz */
void StartSensorTask(void *argument) {
    int sim_disconnect_timer = 0;
    for(;;) {
        uint32_t tick = osKernelGetTickCount();
        
        // --- FAULT INJECTION SIMULATION: IMU Disconnect ---
        if (++sim_disconnect_timer > 1000) { // Every ~10 seconds
            osEventFlagsSet(FaultEventsHandle, FAULT_FLAG_IMU_DISC);
            sim_disconnect_timer = 0;
        }

        int16_t ax, ay, az;
        I2C_Read_IMU(&ax, &ay, &az);
        uint16_t rpm = ADC_Read_RPM();

        // --- CALCULATIONS ---
        // 1. Vehicle Speed (km/h) = RPM * (Circumference) * 60 / 1000
        float speed = (rpm * (2.0f * PI * WHEEL_RADIUS_M) * 60.0f) / 1000.0f;
        
        // 2. G-Force = sqrt(x^2 + y^2 + z^2) scaled to Gs
        float g_force = sqrt((ax*ax) + (ay*ay) + (az*az)) / 9800.0f;
        
        // 3. Sensor Fusion (Simplified Complementary Filter output dummy)
        float pitch = atan2(ay, az) * 180 / PI; 

        // Update Shared State Safely
        if (osMutexAcquire(StateMutexHandle, 10) == osOK) {
            current_state.rpm = rpm;
            current_state.speed_kmh = speed;
            current_state.g_force = g_force;
            current_state.accel_x = ax; current_state.accel_y = ay; current_state.accel_z = az;
            current_state.timestamp_ms = tick;
            osMutexRelease(StateMutexHandle);
        }
        
        // Queue data for telemetry and logging
        osMessageQueuePut(StreamQueueHandle, &current_state, 0, 0);
        osMessageQueuePut(LogQueueHandle, &current_state, 0, 0);

        osDelayUntil(tick + 10); // Strict 10ms execution
    }
}

/* 2. GPS PARSING TASK (UART) - 1Hz */
void StartGPSTask(void *argument) {
    int sim_gps_loss_timer = 0;
    float last_lat = 12.9716, last_lon = 77.5946; // Start in Bengaluru
    
    for(;;) {
        // --- FAULT INJECTION SIMULATION: GPS Loss ---
        if (++sim_gps_loss_timer > 15) { 
            osEventFlagsSet(FaultEventsHandle, FAULT_FLAG_GPS_LOSS);
            sim_gps_loss_timer = 0;
        }

        // Simulate UART NMEA Parse (update coordinates based on speed)
        float current_speed = 0;
        if (osMutexAcquire(StateMutexHandle, 10) == osOK) {
            current_speed = current_state.speed_kmh;
            
            // Calculate Distance (Speed * Time)
            current_state.distance_m += (current_speed * (1000.0f/3600.0f)); // 1 second delta
            
            // Calculate Heading (dummy math based on lat/lon shift)
            current_state.heading_deg = fmod((current_state.heading_deg + 1.5f), 360.0f);
            
            // Lap Timing Logic (If cross start line coordinate)
            if (current_state.distance_m > 4000.0f) { // 4km track
                current_state.lap_time_s = (osKernelGetTickCount() - lap_start_time) / 1000.0f;
                lap_start_time = osKernelGetTickCount();
                current_state.distance_m = 0; // reset
            }
            osMutexRelease(StateMutexHandle);
        }
        osDelay(1000);
    }
}

/* 3. CAN BUS HANDLING TASK - Interrupt Polled */
void StartCANTask(void *argument) {
    for(;;) {
        // Wait for CAN hardware interrupt or poll
        // Simulate CAN Timeout
        if ((rand() % 100) > 95) {
            osEventFlagsSet(FaultEventsHandle, FAULT_FLAG_CAN_TMOUT);
        }
        osDelay(50);
    }
}

/* 4. DATA LOGGING TASK (SPI SD CARD) - Consumes Queue */
void StartLoggingTask(void *argument) {
    TelemetryPacket_t log_pkt;
    for(;;) {
        // Block until data is available in the queue
        if (osMessageQueueGet(LogQueueHandle, &log_pkt, NULL, osWaitForever) == osOK) {
            
            // Attempt to write to SD Card
            if (!SPI_Write_SD(&log_pkt)) {
                // Write failed! Flag the fault monitor
                osEventFlagsSet(FaultEventsHandle, FAULT_FLAG_SD_FAIL);
            }
            
            // In reality: f_printf(&fil, "%d,%.1f,%.1f\n", log_pkt.timestamp_ms, ...);
            // f_sync(&fil);
        }
    }
}

/* 5. WIRELESS TELEMETRY TASK (UART/BLE) - Consumes Queue */
void StartTelemetryTask(void *argument) {
    TelemetryPacket_t tx_pkt;
    char buffer[128];
    for(;;) {
        // Stream at 10Hz, wait for data
        if (osMessageQueueGet(StreamQueueHandle, &tx_pkt, NULL, 100) == osOK) {
            snprintf(buffer, sizeof(buffer), 
                "{\"T\":%lu,\"Spd\":%.1f,\"G\":%.2f,\"Lap\":%.1f}\r\n", 
                tx_pkt.timestamp_ms, tx_pkt.speed_kmh, tx_pkt.g_force, tx_pkt.lap_time_s);
            
            // HAL_UART_Transmit(&huart_ble, (uint8_t*)buffer, strlen(buffer), 10);
        }
    }
}

/* 6. SYSTEM FAULT MONITOR TASK - Highest Priority */
void StartFaultTask(void *argument) {
    for(;;) {
        // Block until ANY fault flag is raised
        uint32_t flags = osEventFlagsWait(FaultEventsHandle, 
                            FAULT_FLAG_GPS_LOSS | FAULT_FLAG_IMU_DISC | 
                            FAULT_FLAG_CAN_TMOUT | FAULT_FLAG_SD_FAIL, 
                            osFlagsWaitAny, osWaitForever);
                            
        if (flags > 0) {
            if (flags & FAULT_FLAG_GPS_LOSS) {
                printf("[ALARM] GPS Signal Lost! Switching to Dead Reckoning.\r\n");
            }
            if (flags & FAULT_FLAG_IMU_DISC) {
                printf("[ALARM] IMU I2C Bus Disconnected! Re-initializing I2C...\r\n");
                // HAL_I2C_DeInit() & HAL_I2C_Init()
            }
            if (flags & FAULT_FLAG_CAN_TMOUT) {
                printf("[ALARM] Engine ECU CAN Timeout!\r\n");
            }
            if (flags & FAULT_FLAG_SD_FAIL) {
                printf("[ALARM] SD Card Write Error! Attempting remount...\r\n");
                // f_mount(NULL, "", 0); f_mount(&FatFs, "", 1);
            }
        }
    }
}

/* --- MAIN INIT --- */
int main(void) {
    // HAL_Init(); SystemClock_Config(); MX_GPIO_Init(); etc...
    osKernelInitialize();
    
    // 1. Initialize Inter-Task Communication
    LogQueueHandle = osMessageQueueNew(32, sizeof(TelemetryPacket_t), NULL);
    StreamQueueHandle = osMessageQueueNew(10, sizeof(TelemetryPacket_t), NULL);
    FaultEventsHandle = osEventFlagsNew(NULL);
    StateMutexHandle = osMutexNew(NULL);

    // 2. Spawn Tasks
    const osThreadAttr_t fault_attr = {.name = "Fault", .priority = osPriorityRealtime};
    FaultTaskHandle = osThreadNew(StartFaultTask, NULL, &fault_attr);
    
    const osThreadAttr_t sens_attr = {.name = "Sensor", .priority = osPriorityHigh};
    SensorTaskHandle = osThreadNew(StartSensorTask, NULL, &sens_attr);
    
    // ... spawn other tasks ...

    osKernelStart();
    while(1) {}
}
