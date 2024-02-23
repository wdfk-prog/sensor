/**
 * @File Name: sensor_driver.c
 * @brief
 * @Author : huangly@milesight.com
 * @Version : 1.0
 * @Creat Date : 2023-06-19
 *
 * @copyright Copyright (c) 2023 星纵物联科技有限公司
 * @par 修改日志:
 * Date           Version     Author  Description
 * 2023-06-19     v1.0        huagnly 内容
 */
/* Includes ------------------------------------------------------------------*/
#include "sensor_driver.h"
/* Private includes ----------------------------------------------------------*/
#include "string.h"
/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static rt_list_t _sensor_list = RT_LIST_OBJECT_INIT(_sensor_list);
/* Private function prototypes -----------------------------------------------*/
/**
 * @brief 传感器注册函数
 * @note   供驱动注册
 * @param dev: 传感器设备
 * @retval 错误码
 */
bool sensor_register_fun(sensor_device_t dev)
{
    if(dev == NULL || dev->name == NULL) {
        return false;
    }else {
        rt_list_insert_before(&_sensor_list, &dev->node);
        return true;
    }
}
/**
 * @brief  传感器驱动查找
 * @note   根据名称挂钩需要调用的传感器设备对象
 * @param  *reg_name: 传感器名称
 * @retval 传感器设备对象
 */
sensor_device_t sensor_obj_get(const char *reg_name)
{
    if(reg_name == NULL) {
        return NULL;
    }
    if(rt_list_isempty(&_sensor_list)) {
        return false;
    }

    sensor_device_t sensor = NULL;
    rt_list_for_each_entry(sensor, &_sensor_list, node) {
        if(strcmp(sensor->name, reg_name) == 0) {
            return sensor;
        }
    }
    return NULL;
}
/**
 * @brief  传感器初始化
 * @note   None
 * @param  dev: 传感器设备
 * @retval 错误码
 */
bool sensor_init(sensor_device_t dev)
{
    if(dev == NULL || dev->ops == NULL || dev->ops->init == NULL) {
        return false;
    }

    bool err = true;
    if (dev->module != NULL && dev->module->init != NULL) {
        if (dev->module->status != SENSOR_MODULE_INIT) {
            dev->module->status = SENSOR_MODULE_INIT;
            err = dev->module->init(dev);
        }
    }

    if(err == true) {
        err = dev->ops->init(dev);
    }

    return err;
}
/**
 * @brief  传感器打开
 * @note   None
 * @param  dev: 传感器设备
 * @retval 错误码
 */
bool sensor_open(sensor_device_t dev)
{
    if(dev == NULL || dev->ops == NULL || dev->ops->open == NULL) {
        return false;
    }

    bool err = true;

    if (dev->module != NULL && dev->module->open != NULL) {
        if (dev->module->status != SENSOR_MODULE_OPEN) {
            dev->module->status = SENSOR_MODULE_OPEN;
            err = dev->module->open(dev);
        }
    }

    if(err == true) {
        err = dev->ops->open(dev);
    }

    return err;
}
/**
 * @brief  传感器关闭
 * @note   None
 * @param  dev: 传感器设备
 * @retval 错误码
 */
bool sensor_close(sensor_device_t dev)
{
    if(dev == NULL || dev->ops == NULL || dev->ops->close == NULL) {
        return false;
    }

    bool err = true;
    if (dev->module->close != NULL && dev->module != NULL) {
        if (dev->module->status != SENSOR_MODULE_CLOSE) {
            dev->module->status = SENSOR_MODULE_CLOSE;
            err = dev->module->close(dev);
        }
    }

    if(err == true) {
        err = dev->ops->close(dev);
    }

    return err;
}
/**
 * @brief  传感器读取
 * @note   None
 * @param  dev: 传感器设备
 * @retval 错误码
 */
bool sensor_collect(sensor_device_t dev)
{
    if(dev == NULL || dev->ops == NULL || dev->ops->collect == NULL) {
        return false;
    }

    bool err = true;
    if (dev->module != NULL) {
        if(dev->module->status != SENSOR_MODULE_READ) {
            dev->module->status = SENSOR_MODULE_READ;
            err = dev->ops->collect(dev);
        }
    } else {
        err = dev->ops->collect(dev);
    }

    return err;
}
/**
 * @brief  传感器低功耗处理
 * @note   None
 * @param  dev: 传感器设备
 * @param  lpm_flag: true:进入低功耗,fasle:退出低功耗
 * @retval None
 */
bool sensor_lpm(sensor_device_t dev, bool lpm_flag)
{
    if(dev == NULL || dev->ops == NULL || dev->ops->close == NULL || dev->ops->open == NULL) {
        return false;
    }

    bool ret = true;
    if(lpm_flag == true) {
        if (dev->module != NULL && dev->module->close != NULL) {
            if (dev->module->status != SENSOR_MODULE_LPM_IN) {
                dev->module->status = SENSOR_MODULE_LPM_IN;
                ret = dev->module->close(dev);
            }
        }
        if(ret == true) {
            ret = dev->ops->close(dev);
        }
    } else {
        if (dev->module != NULL && dev->module->open != NULL) {
            if (dev->module->status != SENSOR_MODULE_LPM_OUT) {
                dev->module->status = SENSOR_MODULE_LPM_OUT;
                ret = dev->module->open(dev);
            }
        }
        if(ret == true) {
            ret = dev->ops->open(dev);
        }
    }

    return ret;
}
/**
 * @brief  传感器数据控制
 * @note   
 * @param  dev: 传感器设备
 * @param  cmd: 控制命令
 * @param  *data: 数据指针
 * @param  *arg: 数据参数
 * @retval 错误码
 */
bool sensor_data_control(sensor_device_t dev, sensor_cmd_e cmd, void *data, void *arg)
{
    if(dev == NULL || dev->ops == NULL || dev->ops->control == NULL) {
        return false;
    }

    return dev->ops->control(dev, cmd, data, arg);
}