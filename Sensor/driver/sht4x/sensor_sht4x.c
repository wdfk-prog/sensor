/**
 * @File Name: sensor_sht4x.c
 * @brief  
 * @Author : 
 * @Version : 1.0
 * @Creat Date : 2024-01-25
 * 
 * @copyright Copyright (c) 2024 
 * @par 修改日志:
 * Date           Version     Author  Description
 * 2024-01-25     v1.0        huagnly 内容
*/
/* Includes ------------------------------------------------------------------*/
#include "sensor_sht4x.h"
/* Private includes ----------------------------------------------------------*/
#include "i2c.h"
/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define I2C_TIMEOUT_MS  (100)
#define SHT4X_I2C       hi2c1
/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static void sht4x_delay(uint32_t delay);
static bool sht4x_i2c_read(uint8_t address, uint8_t *data, uint16_t datasize);
static bool sht4x_i2c_write(uint8_t address, uint8_t *data, uint16_t datasize);

//传感器操作函数
static const sensor_ops_t sht4x_ops;

static sht4x_driver_cfg_t sht4x_cfg =
{
    .handle = 
    {
        .delay_ms   = sht4x_delay,
        .iic_read   = sht4x_i2c_read,
        .iic_write  = sht4x_i2c_write,
    },
    .i2c_init   = MX_I2C1_Init,
};

sht4x_device_t sht4x = 
{
    .parent =
    {
        .name   = "sht4x",      //设备名称
        .ops    = &sht4x_ops,   //操作函数
        .module = NULL,         //模块
    },
    .cfg = &sht4x_cfg, //配置信息
};

static bool sht4x_open(sensor_device_t dev);
static bool sht4x_collect(sensor_device_t dev);
static bool sht4x_close(sensor_device_t dev);
static bool sht4x_control(sensor_device_t dev, sensor_cmd_e cmd, void *data, void *arg);
static const sensor_ops_t sht4x_ops =
{
    .init       = NULL,
    .open       = sht4x_open,
    .close      = sht4x_close,
    .collect    = sht4x_collect,
    .control    = sht4x_control,
};
/* Private function prototypes -----------------------------------------------*/

/* Private user code ---------------------------------------------------------*/
/**
 * @brief  寻找配置指针
 * @note   None
 * @param  dev: 设备句柄
 * @retval 返回配置指针
 */
static sht4x_driver_cfg_t *find_cfg(sensor_device_t dev)
{
    sht4x_device_t *sensor = (sht4x_device_t *)dev;
    if(dev == NULL || sensor == NULL || sensor->cfg == NULL) {
        return NULL;
    }
    return sensor->cfg;
}
/**************************************************
 * @brief 系统延时ms
 **************************************************/
static void sht4x_delay(uint32_t delay)
{
    osDelay(delay);
}
/**
 * @brief  i2c读
 * @note   使用I2C总线从指定地址读取数据
 * @param  address: 目标设备地址
 * @param  *data: 读取数据缓冲区指针
 * @param  datasize: 读取数据字节数
 * @retval None
 */
static bool sht4x_i2c_read(uint8_t address, uint8_t *data, uint16_t datasize)
{
    if (HAL_I2C_Master_Receive(&SHT4X_I2C, (address << 1 | 1), data, datasize, I2C_TIMEOUT_MS) != HAL_OK) {
        printf_error("[sht4x]:read failed!\r\n");
        return false;
    } else {
        return true;
    }
}
/**
 * @brief  i2c写
 * @note   使用I2C总线向指定地址写入数据
 * @param  address: 目标设备地址
 * @param  *data: 待写入的数据缓冲区指针
 * @param  datasize: 待写入的数据字节数
 * @retval None
 */
static bool sht4x_i2c_write(uint8_t address, uint8_t *data, uint16_t datasize)
{
    if (HAL_I2C_Master_Transmit(&SHT4X_I2C, (address << 1 | 0), data, datasize, I2C_TIMEOUT_MS) != HAL_OK) {
        printf_error("[sht4x]:write failed!\r\n");
        return false;
    } else {
        return true;
    }
}

/**
 * @brief  sht4x初始化
 * @note   开启电源
 * @param  dev: 设备句柄
 * @retval true:成功 false:失败
 */
static bool sht4x_open(sensor_device_t dev)
{
    FIND_CFG(sht4x_driver_cfg_t, dev);
    config->i2c_init();
    return true;
}
/**
 * @brief  数据采集
 * @note  None
 * @param  dev: 设备句柄
 * @param  *data: 数据存储地址
 * @param  len: 数据长度
 * @retval true:成功 false:失败
 */
static bool sht4x_collect(sensor_device_t dev)
{
    FIND_CFG(sht4x_driver_cfg_t, dev);
    bool ret = sht4x_measure_high_precision(&config->handle);
    if(ret == true) {
        config->raw[SHT4X_DATA_TEMPERATURE] = config->handle.temperature;
        config->raw[SHT4X_DATA_HUMIDITY] = config->handle.humidity;
        printf("[%s]temp raw:%s\r\n", dev->name, ftoc(config->raw[SHT4X_DATA_TEMPERATURE], 3));
        printf("[%s]humi raw:%s\r\n", dev->name, ftoc(config->raw[SHT4X_DATA_HUMIDITY], 3));
        return true;
    } else {
        printf_error("[%s]collect failed\r\n", dev->name);
        return false;
    }

}
/**
 * @brief  sht4x关闭
 * @note   卸载关闭电源
 * @param  dev: 设备句柄
 * @retval true:成功 false:失败
 */
static bool sht4x_close(sensor_device_t dev)
{
    FIND_CFG(sht4x_driver_cfg_t, dev);
    return true;
}
/**
 * @brief  传感器数据控制
 * @note   
 * @param  dev: 传感器设备
 * @param  cmd: 控制命令
 * @param  *data: 数据指针
 * @param  *arg: 数据参数
 * @retval true:成功 false:失败
 */
static bool sht4x_control(sensor_device_t dev, sensor_cmd_e cmd, void *data, void *arg)
{
    FIND_CFG(sht4x_driver_cfg_t, dev);
    if(data == NULL) {
        return false;
    }
    switch(cmd) {
        case SENSOR_CMD_STATUS_SET:
        {
            uint8_t id = *(uint8_t *)arg;
            if(id < SHT4X_DATA_MAX){
                config->status[id] = *(data_status_e  *)data;
            } else {
                return false;
            }
            break;
        }
        case SENSOR_CMD_STATUS_GET:
        {
            uint8_t id = *(uint8_t *)arg;
            if(id < SHT4X_DATA_MAX){
                *(data_status_e  *)data = config->status[id];
            } else {
                return false;
            }
            break;
        }
        case SENSOR_CMD_DATA_SET:
        {
            uint8_t id = *(uint8_t *)arg;
            if(id == SENSOR_DATA_GET_RAW - SHT4X_DATA_TEMPERATURE) {
                config->raw[SHT4X_DATA_TEMPERATURE] = *(SHT4X_DATA_T *)data;
            } else if(id == SENSOR_DATA_GET_RAW - SHT4X_DATA_HUMIDITY) {
                config->raw[SHT4X_DATA_HUMIDITY] = *(SHT4X_DATA_T *)data;
            } else if(id < SHT4X_DATA_MAX){
                config->value[id] = *(SHT4X_DATA_T *)data;
            } else {
                return false;
            }
            break;
        }
        case SENSOR_CMD_DATA_GET:
        {
            uint8_t id = *(uint8_t *)arg;
            if(id == SENSOR_DATA_GET_RAW - SHT4X_DATA_TEMPERATURE) {
                *(SHT4X_DATA_T *)data = config->raw[SHT4X_DATA_TEMPERATURE];
            } else if(id == SENSOR_DATA_GET_RAW - SHT4X_DATA_HUMIDITY) {
                *(SHT4X_DATA_T *)data = config->raw[SHT4X_DATA_HUMIDITY];
            } else if(id < SHT4X_DATA_MAX){
                *(SHT4X_DATA_T *)data = config->value[id];
            } else {
                return false;
            }
            break;
        }
        default:
            break;
    }
    return true;
}
