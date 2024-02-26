/**
 * @file sensor_builder.c
 * @brief 
 * @author huangly 
 * @version 1.0
 * @date 2024-01-21
 * 
 * @copyright Copyright (c) 2024  
 * 
 * @note :
 * @par 修改日志:
 * Date       Version Author      Description
 * 2024-01-21 1.0     huangly     first version
 */
/* Includes ------------------------------------------------------------------*/
#include "sensor_builder.h"
/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static rt_list_t _builder_list = RT_LIST_OBJECT_INIT(_builder_list);
/* Private function prototypes -----------------------------------------------*/
/**
 * @brief  构建器添加传感器
 * @note   必须具有传感器操作函数与名称 构建器操作函数
 * @param  *builder: 构建器
 * @param  sensor: 传感器
 * @retval true: 成功 false: 失败
 */
bool builder_sensor_add(sensor_builder_t *builder, sensor_device_t sensor)
{
    if(builder == NULL || builder->ops == NULL ||builder->ops->sensor_add == NULL 
    || sensor->ops == NULL || sensor->name == NULL) {
        return false;
    }

    return builder->ops->sensor_add(builder, sensor);
}
/**
 * @brief  构建器配置添加
 * @note   必须具有配置添加函数
 * @param  *builder: 构建器
 * @param  *cfg: 配置项
 * @param  default_flag: 是否初始化为默认配置
 * @retval true: 成功 false: 失败
 */
bool builder_config_add(sensor_builder_t *builder, void *cfg, uint8_t len, bool default_flag)
{
    if(builder == NULL || builder->ops == NULL|| builder->ops->config_add == NULL 
    || cfg == NULL || len == 0) {
        return false;
    }

    return builder->ops->config_add(builder, cfg, len, default_flag);
}
/**
 * @brief  添加构建器至构建器链表
 * @note   必须具有构建器程序与数量
 * @param  *builder: 构建器
 * @retval true: 成功 false: 失败
 */
bool sensor_builder_add(sensor_builder_t *builder)
{
    if(builder == NULL || builder->process == NULL || builder->process_num == 0) {
        return false;
    }

    rt_list_insert_before(&_builder_list, &builder->node);
    return true;
}
/**
 * @brief  所有添加的传感器初始化
 * @note   具有初始化函数的传感器执行初始化
 *         没有添加构建器返回失败
 *         传感器初始化失败一次立刻退出
 * @retval true: 成功 false: 失败
 */
bool sensor_director_init(void)
{
    if(rt_list_isempty(&_builder_list)) {
        return false;
    }

    bool ret = true;
    sensor_builder_t *builder = NULL;

    rt_list_for_each_entry(builder, &_builder_list, node) {
        if(builder->ops != NULL && builder->ops->sensor_init != NULL) {
            ret = builder->ops->sensor_init(builder);
            if(ret == false) {
                return ret;
            }
        }
    }
    return ret;
}
/**
 * @brief  传感器任务执行
 * @note   没有添加构建器退出
 *         传感器执行函数为空跳过
 *         传感器不允许执行跳过当前执行任务
 */
void sensor_director_process(void)
{
    if(rt_list_isempty(&_builder_list)) {
        return;
    }

    sensor_builder_t *builder = NULL;
    rt_list_for_each_entry(builder, &_builder_list, node) {
        if(builder->sensor == NULL) {
            continue;
        }
        if(builder->allow_mode == false) {
            if(builder->process->allow != NULL) {
                if(builder->process->allow(builder->sensor, builder->cfg) == false) {
                    continue;
                }
            }
        }
        for(uint8_t i = 0; i < builder->process_num; i++) {
            builder->current_id = i;
            if(builder->allow_mode == true) {
                if(builder->process[i].allow != NULL) {
                    if(builder->process[i].allow(builder->sensor, builder->cfg) == false) {
                        continue;
                    }
                }
            }

            if(builder->process[i].handler != NULL) {
                builder->process[i].handler(builder->sensor, builder->cfg, builder->cfg_num);
            }
        }
    }
}
