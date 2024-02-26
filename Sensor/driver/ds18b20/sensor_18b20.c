/**
 * @File Name: sensor_18b20.c
 * @brief  DS18B20温度传感器应用函数接口
 * @Author : huangly@milesight.com
 * @Version : 1.0
 * @Creat Date : 2023-10-09
 * 
 * @copyright Copyright (c) 2023 星纵物联科技有限公司
 * @par 修改日志:
 * Date           Version     Author  Description
 * 2023-10-09     v1.0        huagnly 内容
 */
/* Includes ------------------------------------------------------------------*/
#include "sensor_18b20.h"
/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static ds18b20_driver_cfg_t ds18b20_cfg  =
{
    .power = 
    {
        .port = DS18B20_VCC_GPIO_PORT,
        .pin = DS18B20_VCC_PIN,
        .level = DS18B20_VCC_LEVEL,
    },
    .dq = 
    {
        .huart = &huart1,
        .Instance = USART1,
    },
};
//传感器操作函数
static const sensor_ops_t ds18b20_ops;
ds18b20_device_t ds18b20 =
{
    .parent =
    {
        .name   = "ds18b20",    //设备名称
        .ops    = &ds18b20_ops, //操作函数
        .module = NULL,         //模块
    },
    .cfg = &ds18b20_cfg,        //配置信息
};
/* Private function prototypes -----------------------------------------------*/
static bool ds18b20_open(sensor_device_t dev);
static bool ds18b20_close(sensor_device_t dev);
static bool ds18b20_collect(sensor_device_t dev);
static bool dsb1820_control(sensor_device_t dev, sensor_cmd_e cmd, void *data, void *arg);
static const sensor_ops_t ds18b20_ops =
{
    .init       = NULL,
    .open       = ds18b20_open,
    .close      = ds18b20_close,
    .collect    = ds18b20_collect,
    .control    = dsb1820_control,
    .lpm        = NULL,
};
/* Private user code ---------------------------------------------------------*/
/**
 * @brief  寻找配置指针
 * @note   None
 * @param  dev: 设备句柄
 * @retval 返回配置指针
 */
static ds18b20_driver_cfg_t *find_cfg(sensor_device_t dev)
{
    ds18b20_device_t *sensor = (ds18b20_device_t *)dev;
    if(dev == NULL || sensor == NULL || sensor->cfg == NULL) {
        return NULL;
    }
    return sensor->cfg;
}
/**
 * @brief  DS18B20开启
 * @note   开启电源,并发送
 * @param  dev: 设备句柄
 * @retval true:成功 false:失败
 */
static bool ds18b20_open(sensor_device_t dev)
{
    FIND_CFG(ds18b20_driver_cfg_t, dev);

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = config->power.pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(config->power.port, &GPIO_InitStruct);

    HAL_GPIO_WritePin(config->power.port, config->power.pin, config->power.level);
    HAL_Delay(50);

    ds18b20_err_t ret = ds18b20_reset(&config->dq);
    if(ret == DS18B20_ERR_OK) {
        return true;
    } else {
        printf("[%s][error]open:%d\r\n", dev->name, ret);
        return false;
    }
}
/**
 * @brief  ds18b20数据采集
 * @note  None
 * @param  dev: 设备句柄
 * @param  *data: 数据存储地址
 * @param  len: 数据长度
 * @retval true:成功 false:失败
 */
static bool ds18b20_collect(sensor_device_t dev)
{
    FIND_CFG(ds18b20_driver_cfg_t, dev);
    float temperature = 0;

    ds18b20_err_t ret = ds18b20_get_temp_skiprom(&config->dq, &temperature);
    if(ret == DS18B20_ERR_OK) {
        config->raw = temperature;
        printf("[%s]raw:%s\r\n", dev->name, ftoc(config->raw, 3));
        return true;
    } else {
        printf("[%s][error]collect:%d\r\n", dev->name, ret);
        return false;
    }
}
/**
 * @brief  DS18B20关闭
 * @note   关闭电源,并设置为模拟输入
 * @param  dev: 设备句柄
 * @retval true:成功 false:失败
 */
static bool ds18b20_close(sensor_device_t dev)
{
    FIND_CFG(ds18b20_driver_cfg_t, dev);

    HAL_GPIO_WritePin(config->power.port, config->power.pin, !config->power.level);

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = config->power.pin;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(config->power.port, &GPIO_InitStruct);

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
static bool dsb1820_control(sensor_device_t dev, sensor_cmd_e cmd, void *data, void *arg)
{
    FIND_CFG(ds18b20_driver_cfg_t, dev);
    if(data == NULL) {
        return false;
    }
    switch(cmd) {
        case SENSOR_CMD_STATUS_SET:
        {
            config->status = *(data_status_e  *)data;
            break;
        }
        case SENSOR_CMD_STATUS_GET:
        {
            *(data_status_e  *)data = config->status;
            break;
        }
        case SENSOR_CMD_DATA_SET:
        {
            uint8_t id = *(uint8_t *)arg;
            if(id == SENSOR_DATA_GET_RAW) {
                config->raw = *(DS18B20_DATA_T *)data;
            } else{
                config->value = *(DS18B20_DATA_T *)data;
            }
            break;
        }
        case SENSOR_CMD_DATA_GET:
        {
            uint8_t id = *(uint8_t *)arg;
            if(id == SENSOR_DATA_GET_RAW) {
                *(DS18B20_DATA_T *)data = config->raw;
            } else{
                *(DS18B20_DATA_T *)data = config->value;
            }
            break;
        }
        default:
            break;
    }
    return true;
}
