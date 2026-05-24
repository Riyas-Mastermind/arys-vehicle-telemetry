// imu_driver.c
#include "imu_driver.h"

void IMU_Init(I2C_HandleTypeDef *hi2c) {
    uint8_t data = 0x00;
    // Power management: Wake up MPU6050
    HAL_I2C_Mem_Write(hi2c, 0xD0, 0x6B, 1, &data, 1, 100);
}

void IMU_Read_Data(I2C_HandleTypeDef *hi2c, IMU_Data_t *data) {
    uint8_t buffer[14];
    HAL_I2C_Mem_Read(hi2c, 0xD0, 0x3B, 1, buffer, 14, 100);
    
    // Combine high and low bytes
    data->ax = (buffer[0] << 8) | buffer[1];
    data->ay = (buffer[2] << 8) | buffer[3];
    data->az = (buffer[4] << 8) | buffer[5];
}
