/**
 * @File Name: sensor_sht3x.c
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
#include "sensor_sht3x.h"
/* Private includes ----------------------------------------------------------*/
#include "i2c.h"
/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static void sht3x_reset(I2C_HandleTypeDef *hi2c, sht3x_power_t *power);
static void sht3x_delay(uint32_t delay);
static bool sht3x_i2c_read(I2C_HandleTypeDef *hi2c, uint8_t *data, uint16_t datasize);
static bool sht3x_i2c_write(I2C_HandleTypeDef *hi2c, uint8_t *data, uint16_t datasize);
//传感器操作函数
static const sensor_ops_t sht3x_ops;

static sht3x_driver_cfg_t sht3x_cfg[SHT3X_MAX_NUM] =
{
    [SHT3X_0] = 
    {
        .device = 
        {
            .hi2c       = &hi2c1,
            .power =
            {
                .pin    = SHT3X_VCC_PIN,
                .port   = SHT3X_VCC_GPIO_PORT,
                .level  = SHT3X_VCC_LEVEL,
            },
            .reset      = sht3x_reset,
            .delay_ms   = sht3x_delay,
            .iic_read   = sht3x_i2c_read,
            .iic_write  = sht3x_i2c_write,
        },
        .i2c_init = MX_I2C1_Init,
    },
    [SHT3X_1] = 
    {
        .device = 
        {
            .hi2c       = &hi2c1,
            .power =
            {
                .pin    = SHT3X_VCC_PIN,
                .port   = SHT3X_VCC_GPIO_PORT,
                .level  = SHT3X_VCC_LEVEL,
            },
            .reset      = sht3x_reset,
            .delay_ms   = sht3x_delay,
            .iic_read   = sht3x_i2c_read,
            .iic_write  = sht3x_i2c_write,
        },
        .i2c_init = MX_I2C1_Init,
    },
};

sht3x_device_t sht3x[SHT3X_MAX_NUM] = 
{
    [SHT3X_0] = 
    {
        .parent =
        {
            .name   = "sht3x_0",    //设备名称
            .ops    = &sht3x_ops,   //操作函数
            .module = NULL,         //模块
        },
        .cfg = &sht3x_cfg[SHT3X_0], //配置信息
    },
    [SHT3X_1] = 
    {
        .parent =
        {
            .name   = "sht3x_1",    //设备名称
            .ops    = &sht3x_ops,   //操作函数
            .module = NULL,         //模块
        },
        .cfg = &sht3x_cfg[SHT3X_1], //配置信息
    },
};
/* Private function prototypes -----------------------------------------------*/
static bool sensor_sht3x_init(sensor_device_t dev);
static bool sht3x_open(sensor_device_t dev);
static bool sht3x_collect(sensor_device_t dev);
static bool sht3x_close(sensor_device_t dev);
static bool sht3x_control(sensor_device_t dev, sensor_cmd_e cmd, void *data, void *arg);
static const sensor_ops_t sht3x_ops =
{
    .init       = sensor_sht3x_init,
    .open       = sht3x_open,
    .close      = sht3x_close,
    .collect    = sht3x_collect,
    .control    = sht3x_control,
};
/* Private user code ---------------------------------------------------------*/
/**
 * @brief  寻找配置指针
 * @note   None
 * @param  dev: 设备句柄
 * @retval 返回配置指针
 */
static sht3x_driver_cfg_t *find_cfg(sensor_device_t dev)
{
    sht3x_device_t *sensor = (sht3x_device_t *)dev;
    if(dev == NULL || sensor == NULL || sensor->cfg == NULL) {
        return NULL;
    }
    return sensor->cfg;
}
/**************************************************
 * @brief 重置sht3x设备
 **************************************************/
static void sht3x_reset(I2C_HandleTypeDef *hi2c, sht3x_power_t *power)
{
    HAL_GPIO_WritePin(power->port, power->pin, !power->level);
    HAL_I2C_DeInit(hi2c);
    osDelay(10);
    HAL_GPIO_WritePin(power->port, power->pin, power->level);
    HAL_I2C_Init(hi2c);
    osDelay(10);
}
/**************************************************
 * @brief 系统延时ms
 **************************************************/
static void sht3x_delay(uint32_t delay)
{
    osDelay(delay);
}
/**************************************************
 * @brief IIC读设备
 **************************************************/
static bool sht3x_i2c_read(I2C_HandleTypeDef *hi2c, uint8_t *data, uint16_t datasize)
{
    if (HAL_I2C_Master_Receive(hi2c, (0x44<<1 | 1), data, datasize, 100) != HAL_OK) {
        printf_error("[sht3x]:read failed!\r\n");
        return false;
    } else {
        return true;
    }
}
/**************************************************
 * @brief IIC写设备
 **************************************************/
static bool sht3x_i2c_write(I2C_HandleTypeDef *hi2c, uint8_t *data, uint16_t datasize)
{
    if (HAL_I2C_Master_Transmit(hi2c, (0x44<<1 | 0), data, datasize, 100) != HAL_OK) {
        printf_error("[sht3x]:write failed!\r\n");
        return false;
    } else {
        return true;
    }
}

/**
 * @brief  初始化引脚
 * @note   None
 * @param  dev: 设备句柄
 * @retval true:成功 false:失败
 */
bool sensor_sht3x_init(sensor_device_t dev)
{
    FIND_CFG(sht3x_driver_cfg_t, dev);
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = config->device.power.pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(config->device.power.port, &GPIO_InitStruct);
    HAL_GPIO_WritePin(config->device.power.port, config->device.power.pin, config->device.power.level);

    config->i2c_init();
    sht3x_init(&config->device);

    HAL_GPIO_WritePin(config->device.power.port, config->device.power.pin, !config->device.power.level);

    return true;
}
/**
 * @brief  sht3x初始化
 * @note   开启电源
 * @param  dev: 设备句柄
 * @retval true:成功 false:失败
 */
static bool sht3x_open(sensor_device_t dev)
{
    FIND_CFG(sht3x_driver_cfg_t, dev);
    HAL_GPIO_WritePin(config->device.power.port, config->device.power.pin, config->device.power.level);
    HAL_Delay(50);
    if(HAL_I2C_Init(config->device.hi2c) != HAL_OK) {
        return false;
    } else {
        return true;
    }
}
/**
 * @brief  数据采集
 * @note  None
 * @param  dev: 设备句柄
 * @param  *data: 数据存储地址
 * @param  len: 数据长度
 * @retval true:成功 false:失败
 */
static bool sht3x_collect(sensor_device_t dev)
{
    FIND_CFG(sht3x_driver_cfg_t, dev);

    if(sht3x_collect_process(&config->device) == false) {
        return false;
    } else {
        sht3x_get_current_temp(&config->device, &config->raw[SHT3X_DATA_TEMPERATURE]);
        sht3x_get_current_humi(&config->device, &config->raw[SHT3X_DATA_HUMIDITY]);
        printf("[%s]temp raw:%s\r\n", dev->name, ftoc(config->raw[SHT3X_DATA_TEMPERATURE], 3));
        printf("[%s]humi raw:%s\r\n", dev->name, ftoc(config->raw[SHT3X_DATA_HUMIDITY], 3));
        return true;
    }
}
/**
 * @brief  sht3x关闭
 * @note   卸载关闭电源
 * @param  dev: 设备句柄
 * @retval true:成功 false:失败
 */
static bool sht3x_close(sensor_device_t dev)
{
    FIND_CFG(sht3x_driver_cfg_t, dev);
    HAL_GPIO_WritePin(config->device.power.port, config->device.power.pin, !config->device.power.level);
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
static bool sht3x_control(sensor_device_t dev, sensor_cmd_e cmd, void *data, void *arg)
{
    FIND_CFG(sht3x_driver_cfg_t, dev);
    if(data == NULL) {
        return false;
    }
    switch(cmd) {
        case SENSOR_CMD_STATUS_SET:
        {
            uint8_t id = *(uint8_t *)arg;
            if(id < SHT3X_DATA_MAX){
                config->status[id] = *(data_status_e  *)data;
            } else {
                return false;
            }
            break;
        }
        case SENSOR_CMD_STATUS_GET:
        {
            uint8_t id = *(uint8_t *)arg;
            if(id < SHT3X_DATA_MAX){
                *(data_status_e  *)data = config->status[id];
            } else {
                return false;
            }
            break;
        }
        case SENSOR_CMD_DATA_SET:
        {
            uint8_t id = *(uint8_t *)arg;
            if(id == SENSOR_DATA_GET_RAW - SHT3X_DATA_TEMPERATURE) {
                config->raw[SHT3X_DATA_TEMPERATURE] = *(SHT3X_DATA_T *)data;
            } else if(id == SENSOR_DATA_GET_RAW - SHT3X_DATA_HUMIDITY) {
                config->raw[SHT3X_DATA_HUMIDITY] = *(SHT3X_DATA_T *)data;
            } else if(id < SHT3X_DATA_MAX){
                config->value[id] = *(SHT3X_DATA_T *)data;
            } else {
                return false;
            }
            break;
        }
        case SENSOR_CMD_DATA_GET:
        {
            uint8_t id = *(uint8_t *)arg;
            if(id == SENSOR_DATA_GET_RAW - SHT3X_DATA_TEMPERATURE) {
                *(SHT3X_DATA_T *)data = config->raw[SHT3X_DATA_TEMPERATURE];
            } else if(id == SENSOR_DATA_GET_RAW - SHT3X_DATA_HUMIDITY) {
                *(SHT3X_DATA_T *)data = config->raw[SHT3X_DATA_HUMIDITY];
            } else if(id < SHT3X_DATA_MAX){
                *(SHT3X_DATA_T *)data = config->value[id];
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
