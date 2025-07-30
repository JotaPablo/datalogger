#ifndef MPU6050_H
#define MPU6050_H

#include <stdint.h>
#include "hardware/i2c.h"

// Endereço padrão do MPU6050
#define MPU6050_ADDR 0x68

// Funções da biblioteca
void mpu6050_init(i2c_inst_t *i2c_port);
void mpu6050_read_raw(i2c_inst_t *i2c_port, int16_t accel[3], int16_t gyro[3], int16_t *temp);

#endif
