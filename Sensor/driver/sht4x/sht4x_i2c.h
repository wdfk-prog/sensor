#ifndef SHT4X_I2C_H
#define SHT4X_I2C_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef struct 
{
    float temperature;
    float humidity;
    /**
     * @brief IIC 读操作
     * @param[out] data 读取的数据块
     * @param[in] datasize 要读取的数据长度
     * @return 读取结果
     */
    bool (*iic_read)(uint8_t address, uint8_t *data, uint16_t datasize);
    /**
     * @brief IIC 写操作
     * @param[in] data 写入的数据块
     * @param[in] datasize 写入的数据长度
     * @return 写入结果
     */
    bool (*iic_write)(uint8_t address, uint8_t *data, uint16_t datasize);
    /**
     * @brief 系统 ms 级延时
     * @param[in] delay 延时时间 ms
     */
    void (*delay_ms)(uint32_t delay);
}sht4x_handle_t;

/**
 * sht4x_measure_high_precision() - SHT4x command for a single shot
 * measurement with high repeatability.
 *
 * @param temperature Temperature in degrees centigrade.
 *
 * @param humidity Humidity in percent relative humidity.
 *
 */
bool sht4x_measure_high_precision(sht4x_handle_t *dev);

/**
 * sht4x_serial_number() - Read out the serial number
 *
 * @note Each sensor has a unique serial number that is assigned by Sensirion
 * during production. It is stored in the one-time-programmable memory and
 * cannot be manipulated after production.
 *
 * @param serial_number Unique serial number
 *
 */
bool sht4x_serial_number(sht4x_handle_t *dev, uint32_t* serial_number);

/**
 * sht4x_soft_reset() - Perform a soft reset.
 *
 * @note A reset of the sensor can be achieved in three ways: By perform a soft
 * reset using this function, by using an I2C general call, at which all devices
 * on the I2C bus will be reset, or by a power down (incl. pulling SCL and SDA
 * low). See the datasheet for more detailed information.
 *
 */
bool sht4x_soft_reset(sht4x_handle_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* SHT4X_I2C_H */
