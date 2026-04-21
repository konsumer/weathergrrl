#include <Wire.h>

#define AHT10_ADDR 0x38

static bool aht10_read(float& humidity, float& temperature) {
    uint8_t err;
    for (int i = 0; i < 5; i++) {
        Wire.beginTransmission(AHT10_ADDR);
        Wire.write(0xAC); Wire.write(0x33); Wire.write(0x00);  // trigger measurement
        err = Wire.endTransmission();
        if (err == 0) break;
    }
    if (err != 0) return false;

    if (Wire.requestFrom(AHT10_ADDR, 6) != 6) return false;
    uint8_t buf[6];
    for (auto& b : buf) b = Wire.read();

    uint32_t raw_hum  = ((uint32_t)buf[1] << 12) | ((uint32_t)buf[2] << 4) | (buf[3] >> 4);
    uint32_t raw_temp = (((uint32_t)buf[3] & 0x0F) << 16) | ((uint32_t)buf[4] << 8) | buf[5];

    humidity    = raw_hum  / 1048576.0f * 100.0f;
    temperature = raw_temp / 1048576.0f * 200.0f - 50.0f;
    return true;
}

static bool aht10_begin() {
    Wire.beginTransmission(AHT10_ADDR);
    if (Wire.endTransmission() != 0) return false;  // device not present
    Wire.beginTransmission(AHT10_ADDR);
    Wire.write(0xE1); Wire.write(0x08); Wire.write(0x00);  // calibrate (may NACK if already calibrated)
    Wire.endTransmission();
    return true;
}