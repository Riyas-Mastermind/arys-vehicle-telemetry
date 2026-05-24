# Vehicle Telemetry System

A professional-grade, RTOS-based telemetry system for real-time monitoring of vehicle performance. Designed for the **STM32F401RE** microcontroller, this project focuses on robust data acquisition, fault monitoring, and persistent data logging.

## Project Overview
This system utilizes a preemptive, priority-based scheduling model (FreeRTOS) to handle multiple sensor inputs simultaneously. It calculates real-time vehicle dynamics—including Engine RPM, Speed, and G-Forces—and ensures data integrity even in the event of hardware failures.

## Key Features
* **Real-Time Architecture:** Multi-threaded design using FreeRTOS (CMSIS-V2) to separate high-priority sensing tasks from low-priority logging/transmission tasks.
* **Sensor Fusion:** Processes I2C (IMU), ADC (Hall Effect/RPM), and UART (GPS) data to generate a unified telemetry stream.
* **Fault Monitoring:** Integrated fault detection for I2C/SPI bus disconnects, GPS signal loss, and SD card I/O errors.
* **Data Integrity:** Implements Mutexes for shared state protection and Queues for asynchronous data passing between tasks.
* **Reliable Logging:** FatFs-based SD card logging with physical data synchronization to prevent corruption during power cycles.

## System Architecture
* **Microcontroller:** STM32F401RE (ARM Cortex-M4)
* **Sensors:** MPU6050 (IMU), Neo-6M (GPS), Hall Effect (Speed/RPM)
* **Communication:** CAN Transceiver (SPI), ESP32 Telemetry Module (UART)
* **Storage:** SD Card (SPI, FatFs)

## Folder Structure
```text
/Vehicle-Telemetry-System
├── /Core
│   ├── /Inc          # Application header files
│   └── /Src          # Main logic and RTOS task definitions
├── /Drivers
│   ├── /Custom       # Custom drivers (IMU, SD Card)
│   └── /STM32F4xx    # HAL Drivers
├── /Docs             # Project diagrams and test results
├── .gitignore        # Build artifact protection
└── README.md
