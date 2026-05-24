#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"
#include "cmsis_os.h"

/* Application Constants */
#define FAULT_FLAG_GPS_LOSS   0x01
#define FAULT_FLAG_IMU_DISC   0x02
#define FAULT_FLAG_CAN_TMOUT  0x04
#define FAULT_FLAG_SD_FAIL    0x08

/* RTOS Global Handles (Exposed for task files) */
extern osMessageQueueId_t LogQueueHandle;
extern osMessageQueueId_t StreamQueueHandle;
extern osEventFlagsId_t FaultEventsHandle;
extern osMutexId_t StateMutexHandle;

/* Function Prototypes */
void Error_Handler(void);
void SystemClock_Config(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
