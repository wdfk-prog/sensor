/**
 * @File Name: sensor_mcs.c
 * @brief
 * @Author : huangly@milesight.com
 * @Version : 1.0
 * @Creat Date : 2023-07-06
 *
 * @copyright Copyright (c) 2023 星纵物联科技有限公司
 * @par 修改日志:
 * Date           Version     Author  Description
 * 2023-07-06     v1.0        huagnly 内容
 */
/* Includes ------------------------------------------------------------------*/
#include "sensor_mcs.h"
/* Private includes ----------------------------------------------------------*/
#include <stdio.h>
#include "critical_platform.h"
/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define MCS_FILTER_TIME 50 //滤波时间  ms
/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static mcs_cfg_t mcs_cfg =
{
    .status         = MCS_STATUS_NONE,
    .filter         = MCS_FILTER_TIME,
    .output = {
        .pin = MCS_OUT_PIN,
        .on = MCS_OUT_ON,
    },
    .input = {
        .pin = MCS_IN_PIN,
        .level = MCS_IN_LEVEL,
    },
};
//传感器操作函数
static const sensor_ops_t mcs_ops;
mcs_device_t mcs =
{
    .parent =
    {
        .name = "mcs",  //设备名称
        .ops    = &mcs_ops, //操作函数
        .module = NULL,     //模块
    },
    .cfg = &mcs_cfg,        //配置信息
};
/* Private function prototypes -----------------------------------------------*/
static bool mcs_init(sensor_device_t dev);
static bool mcs_collect(sensor_device_t dev);
static bool mcs_control(sensor_device_t dev, sensor_cmd_e cmd, void *data, void *arg);
static void mcs_isr(void* context);
static const sensor_ops_t mcs_ops =
{
    .init       = mcs_init,
    .collect    = mcs_collect,
    .control    = mcs_control,
};
/* Private user code ---------------------------------------------------------*/
/**
 * @brief  寻找配置指针
 * @note   None
 * @param  dev: 设备句柄
 * @retval 返回配置指针
 */
static mcs_cfg_t *find_cfg(sensor_device_t dev)
{
    mcs_device_t *sensor = (mcs_device_t *)dev;
    if(dev == NULL || sensor == NULL || sensor->cfg == NULL) {
        return NULL;
    }
    return sensor->cfg;
}
/**
 * @brief  门磁初始化
 * @note   设置输出引脚与输入引脚
 * @param  dev: 设备句柄
 * @retval 错误码
 */
static bool mcs_init(sensor_device_t dev)
{
    FIND_CFG(mcs_cfg_t, dev);

    GpioInit(&config->output.obj, config->output.pin, PIN_OUTPUT, PIN_PUSH_PULL, PIN_NO_PULL, config->output.on);
    GpioInit(&config->input.obj,  config->input.pin,  PIN_INPUT,  PIN_PUSH_PULL, PIN_NO_PULL, config->input.level);
    GpioSetInterrupt(&config->input.obj, IRQ_RISING_FALLING_EDGE, 5, mcs_isr);
    GpioSetContext(&config->input.obj, (void*)dev);
    
    config->current_level = GpioRead(&config->input.obj);
    config->status = MCS_STATUS_INIT;
    config->input.level = config->output.on;

    return true;
}
/**
 * @brief  门磁数据采集
 * @note  None
 * @param  dev: 设备句柄
 * @param  *data: 数据存储地址
 * @param  len: 数据长度
 * @retval 错误码
 */
static bool mcs_collect(sensor_device_t dev)
{
    FIND_CFG(mcs_cfg_t, dev);

    if(config->status != MCS_STATUS_COLLECT) {
        return false;
    }

    bool ret = true;
    HAL_Delay(config->filter);
    config->status = MCS_STATUS_INIT;
    config->current_level = GpioRead(&config->input.obj);
    if(config->isr_level == config->current_level) {
        if(config->input.level == config->current_level) {
            config->status = MCS_STATUS_CLOSE;
        } else {
            config->status = MCS_STATUS_OPEN;
        }

        // printf("[mcs]delay:%dms,status:%d\r\n", config->filter, *data);
        ret = true;
    } else  {
        ret = false;
    }
    return ret;
}
/**
 * @brief  门磁控制
 * @note   None
 * @param  dev: 设备句柄
 * @param  cmd: 控制命令
 * @param  *arg: 参数
 * @retval 错误码
 */
static bool mcs_control(sensor_device_t dev, sensor_cmd_e cmd, void *data, void *arg)
{
    FIND_CFG(mcs_cfg_t, dev);
    if(data == NULL) {
        return false;
    }
    bool ret = true;
    NODE_CRITICAL_SECTION_BEGIN();
    switch (cmd)
    {
        case SENSOR_CMD_DATA_SET:
        {
            mcs_status_t *pStatus = (mcs_status_t *)data;
            config->status = *pStatus;
            break;
        }
        case SENSOR_CMD_DATA_GET:
        {
            mcs_status_t *pStatus = (mcs_status_t *)data;
            *pStatus = config->status;
            break;
        }
        case SENSOR_CMD_GET_OTHER:  //获取当前电平
        {
            GPIO_PinState *p = (GPIO_PinState *)data;
            *p = config->current_level;
            break;
        }
        case SENSOR_CMD_SET_ISR_BACK:
        {
            config->isr_callback = data;
            break;
        }
        default:
            break;
    }
    NODE_CRITICAL_SECTION_END();

    return ret;
}
/**
 * @brief  门磁中断服务函数
 * @note   None
 * @param  context: 设备句柄
 * @retval None
 */
static void mcs_isr(void* context)
{
    sensor_device_t dev = (sensor_device_t)context;
    mcs_cfg_t *config = find_cfg(dev);
    if(config == NULL) {
        return;
    }

    if (config->status == MCS_STATUS_NONE) {
        return;
    }

    config->isr_level = GpioRead(&config->input.obj);
    config->status = MCS_STATUS_COLLECT;
    printf("[mcs]isr,level:%d\r\n", config->isr_level);
    if(config->isr_callback != NULL) {
        config->isr_callback();
    }
}
