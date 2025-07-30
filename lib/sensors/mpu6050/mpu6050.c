#include "mpu6050.h"
#include "pico/stdlib.h"

// Função para resetar e inicializar o MPU6050
void mpu6050_init(i2c_inst_t *i2c_port) {
    // Dois bytes para reset: primeiro o registrador, segundo o dado
    uint8_t buf[] = {0x6B, 0x80}; 
    i2c_write_blocking(i2c_port, MPU6050_ADDR, buf, 2, false);
    sleep_ms(100); // Aguarda reset e estabilização

    // Sai do modo sleep (registrador 0x6B, valor 0x00)
    buf[1] = 0x00; 
    i2c_write_blocking(i2c_port, MPU6050_ADDR, buf, 2, false);
    sleep_ms(10); // Aguarda estabilização após acordar
}

// Função para ler dados crus do acelerômetro, giroscópio e temperatura
void mpu6050_read_raw(i2c_inst_t *i2c_port, int16_t accel[3], int16_t gyro[3], int16_t *temp) {
    uint8_t reg;
    uint8_t buffer[6];

    // Acelerômetro (0x3B - 0x40)
    reg = 0x3B;
    i2c_write_blocking(i2c_port, MPU6050_ADDR, &reg, 1, true);
    i2c_read_blocking(i2c_port, MPU6050_ADDR, buffer, 6, false);
    for (int i = 0; i < 3; i++)
        accel[i] = (buffer[i * 2] << 8) | buffer[i * 2 + 1];

    // Giroscópio (0x43 - 0x48)
    reg = 0x43;
    i2c_write_blocking(i2c_port, MPU6050_ADDR, &reg, 1, true);
    i2c_read_blocking(i2c_port, MPU6050_ADDR, buffer, 6, false);
    for (int i = 0; i < 3; i++)
        gyro[i] = (buffer[i * 2] << 8) | buffer[i * 2 + 1];

    // Temperatura (0x41 - 0x42)
    reg = 0x41;
    i2c_write_blocking(i2c_port, MPU6050_ADDR, &reg, 1, true);
    i2c_read_blocking(i2c_port, MPU6050_ADDR, buffer, 2, false);
    *temp = (buffer[0] << 8) | buffer[1];
}
