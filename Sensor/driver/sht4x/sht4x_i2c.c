#include "sht4x_i2c.h"

#define SHT4X_I2C_ADDRESS     0x44
#define SHT4X_DRIVER_VERSION  0x11

#define CRC8_POLYNOMIAL 0x31
#define CRC8_INIT 0xFF
#define CRC8_LEN 1

#define SENSIRION_WORD_SIZE 2

static float convert_ticks_to_celsius(uint16_t ticks)
{
    return (float)ticks * 175.0f / 65535.0f - 45.0f;
}

static float convert_ticks_to_percent_rh(uint16_t ticks)
{
    return (float)ticks * 125.0f / 65535.0f - 6.0f;
}

uint16_t sensirion_common_bytes_to_uint16_t(const uint8_t* bytes)
{
    return (uint16_t)bytes[0] << 8 | (uint16_t)bytes[1];
}

uint32_t sensirion_common_bytes_to_uint32_t(const uint8_t* bytes) {
    return (uint32_t)bytes[0] << 24 | (uint32_t)bytes[1] << 16 |
           (uint32_t)bytes[2] << 8 | (uint32_t)bytes[3];
}

bool sensirion_i2c_check_crc(const uint8_t* data, uint16_t count, uint8_t checksum)
{
    uint16_t current_byte;
    uint8_t crc = CRC8_INIT;
    uint8_t crc_bit;

    /* calculates 8-Bit checksum with given polynomial */
    for (current_byte = 0; current_byte < count; ++current_byte) {
        crc ^= (data[current_byte]);
        for (crc_bit = 8; crc_bit > 0; --crc_bit) {
            if (crc & 0x80)
                crc = (crc << 1) ^ CRC8_POLYNOMIAL;
            else
                crc = (crc << 1);
        }
    }

    if (crc != checksum) {
        return false;
    } else {
        return true;
    }
}

bool sensirion_i2c_read_data_inplace(sht4x_handle_t *dev, uint8_t address, uint8_t* buffer, uint16_t expected_data_length)
{
    uint16_t i, j;
    uint16_t size = (expected_data_length / SENSIRION_WORD_SIZE) * (SENSIRION_WORD_SIZE + CRC8_LEN);

    if (expected_data_length % SENSIRION_WORD_SIZE != 0) {
        return false;
    }

    if (dev->iic_read(address, buffer, size) != true) {
        return false;
    }

    for (i = 0, j = 0; i < size; i += SENSIRION_WORD_SIZE + CRC8_LEN) {
        if (sensirion_i2c_check_crc(&buffer[i], SENSIRION_WORD_SIZE, buffer[i + SENSIRION_WORD_SIZE]) != true) {
            return false;
        }
        buffer[j++] = buffer[i];
        buffer[j++] = buffer[i + 1];
    }

    return true;
}

bool sht4x_measure_high_precision_ticks(sht4x_handle_t *dev, uint16_t* temperature_ticks, uint16_t* humidity_ticks)
{
    uint8_t buffer[6];
    uint16_t offset = 0;
    buffer[offset++] = (uint8_t)0xFD;

    if (dev->iic_write(SHT4X_I2C_ADDRESS, &buffer[0], offset) != true) {
        return false;
    }
    dev->delay_ms(10);
    if (sensirion_i2c_read_data_inplace(dev, SHT4X_I2C_ADDRESS, &buffer[0], 4) != true) {
        return false;
    }
    *temperature_ticks = sensirion_common_bytes_to_uint16_t(&buffer[0]);
    *humidity_ticks = sensirion_common_bytes_to_uint16_t(&buffer[2]);

    return true;
}

bool sht4x_measure_high_precision(sht4x_handle_t *dev)
{
    uint16_t temperature_ticks;
    uint16_t humidity_ticks;

    if (sht4x_measure_high_precision_ticks(dev, &temperature_ticks, &humidity_ticks) != true) {
        return false;
    }
    dev->temperature = convert_ticks_to_celsius(temperature_ticks);
    dev->humidity = convert_ticks_to_percent_rh(humidity_ticks);
    if(dev->humidity > 100) {
        dev->humidity = 100;
    }
    return true;
}

bool sht4x_serial_number(sht4x_handle_t *dev, uint32_t* serial_number)
{
    uint8_t buffer[6];
    uint16_t offset = 0;
    buffer[offset++] = (uint8_t)0x89;

    if (dev->iic_write(SHT4X_I2C_ADDRESS, &buffer[0], offset) != true) {
        return false;
    }

    dev->delay_ms(10);

    if (sensirion_i2c_read_data_inplace(dev, SHT4X_I2C_ADDRESS, &buffer[0], 4) != true) {
        return false;
    }
    *serial_number = sensirion_common_bytes_to_uint32_t(&buffer[0]);
    return true;
}

bool sht4x_soft_reset(sht4x_handle_t *dev)
{
    uint8_t buffer[2];
    uint16_t offset = 0;
    buffer[offset++] = (uint8_t)0x94;

    if (dev->iic_write(SHT4X_I2C_ADDRESS, &buffer[0], offset) != true) {
        return false;
    }
    dev->delay_ms(10);
    return true;
}
