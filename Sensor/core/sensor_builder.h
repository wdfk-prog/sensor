/**
 * @file sensor_builder.h
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
#ifndef __SENSOR_BUILDER_H__
#define __SENSOR_BUILDER_H__

#ifdef __cplusplus
extern "C" {
#endif
/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "rt_list.h"
#include "sensor_driver.h"
/* Exported types ------------------------------------------------------------*/
/**
 * @brief  允许执行任务判断
 * @note   None
 * @param  sensor: 传感器
 * @param  *cfg: 配置
 * @retval true: 允许 false: 不允许
 */
typedef bool (*allow_process_t)(sensor_device_t sensor, void *cfg);
/**
 * @brief  传感器任务处理
 * @note   None
 * @param  sensor: 传感器设备
 * @param  *cfg: 构建器配置
 * @param  num :配置数量
 */
typedef void (*sensor_process_t)(sensor_device_t sensor, void *cfg, uint8_t num);
typedef struct 
{
    allow_process_t     allow;     //允许执行任务判断
    sensor_process_t    handler;   //传感器任务处理
}sensor_process_ops_t;

typedef struct sensor_builder sensor_builder_t;
/**
 * @brief  构建器添加传感器
 * @note   None
 * @param  *builder: 构建器
 * @param  sensor: 传感器
 * @retval true: 成功 false: 失败
 */
typedef bool (*sensor_add_t)(sensor_builder_t *builder, sensor_device_t sensor);
/**
 * @brief  执行传感器初始化
 * @note   None
 * @param  *builder: 构建器
 * @retval true: 成功 false: 失败
 */
typedef bool (*sensor_init_t)(sensor_builder_t *builder);
/**
 * @brief  构建器配置添加
 * @note   None
 * @param  *builder: 构建器
 * @param  *cfg: 配置项
 * @param  default_flag: 是否初始化为默认配置
 * @retval true: 成功 false: 失败
 */
typedef bool (*sensor_config_add_t)(sensor_builder_t *builder, void *cfg, uint8_t len, bool default_flag);
/**
 * @brief  传感器构建接口
 * @note   
 */
typedef struct 
{
    sensor_add_t        sensor_add;
    sensor_init_t       sensor_init;
    sensor_config_add_t config_add;
}sensor_builder_ops_t;
/**
 * @brief  传感器构建类
 * @note   None
 */
struct sensor_builder
{
    rt_list_t node;
    sensor_device_t sensor;

    void* cfg;
    uint8_t cfg_num;

    sensor_builder_ops_t *ops;

    uint8_t current_id;
    uint8_t process_num;
    //true: 每个任务都需要判断 false: 只在第一次执行判断
    bool    allow_mode; 
    sensor_process_ops_t *process;
};
/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported variables ---------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
bool builder_sensor_add(sensor_builder_t *builder, sensor_device_t sensor);
bool builder_config_add(sensor_builder_t *builder, void *cfg, uint8_t len, bool default_flag);

bool sensor_builder_add(sensor_builder_t *builder);
bool sensor_director_init(void);
void sensor_director_process(void);

#ifdef __cplusplus
}
#endif

#endif /* __SENSOR_BUILDER_H__ */