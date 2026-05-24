// imu_driver.h
#include "main.h"

typedef struct {
    int16_t ax, ay, az;
    int16_t gx, gy, gz;
} IMU_Data_t;

void IMU_Init(I2C_HandleTypeDef *hi2c);
void IMU_Read_Data(I2C_HandleTypeDef *hi2c, IMU_Data_t *data);
