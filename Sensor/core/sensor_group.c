/**
 * @file sensor_group.c
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
static void group_collect(sensor_device_t sensor, void *cfg, uint8_t num);
static void group_calibration(sensor_device_t sensor, void *cfg, uint8_t num);
static void group_check(sensor_device_t sensor, void *cfg, uint8_t num);
static void group_alarm(sensor_device_t sensor, void *cfg, uint8_t num);
static void group_report(sensor_device_t sensor, void *cfg, uint8_t num);
static void group_storage(sensor_device_t sensor, void *cfg, uint8_t num);

static bool group_sensor_add(sensor_builder_t *builder, sensor_device_t sensor);
static bool group_sensor_init(sensor_builder_t *builder);
//组合传感器执行策略
//实现可添加传感器,实现多种传感器一起运行同一动作
static sensor_process_ops_t group_process[] = 
{
    {   .handler    = &group_collect,           },
    {   .handler    = &group_calibration,       },
    {   .handler    = &group_check,             },
    {   .handler    = &group_alarm,             },
    {   .handler    = &group_report,            },
    {   .handler    = &group_storage,           },
};
static sensor_builder_ops_t builder_ops = 
{
    .sensor_add = group_sensor_add,
    .sensor_init = group_sensor_init,
};
sensor_builder_t group_builder = 
{
    .process = group_process,
    .process_num = sizeof(group_process) / sizeof(sensor_process_ops_t),
    .ops = &builder_ops,
};

static rt_list_t _sensor_list = RT_LIST_OBJECT_INIT(_sensor_list);
/* Private function prototypes -----------------------------------------------*/
static bool group_sensor_add(sensor_builder_t *builder, sensor_device_t sensor)
{
    rt_list_insert_before(&_sensor_list, &sensor->node);
    return true;
}
static bool group_sensor_init(sensor_builder_t *builder)
{
    if(rt_list_isempty(&_sensor_list)) {
        return false;
    }

    sensor_device_t sensor;
    rt_list_for_each_entry(sensor, &_sensor_list, node) {
        sensor_init(builder->sensor);
    }
    return true;
}

static void group_collect(sensor_device_t sensor, void *cfg, uint8_t num)
{
    rt_list_for_each_entry(sensor, &_sensor_list, node) {
        sensor_collect(sensor);
    }
}
static void group_calibration(sensor_device_t sensor, void *cfg, uint8_t num)
{
    printf("%s\r\n",__func__);
}
static void group_check(sensor_device_t sensor, void *cfg, uint8_t num)
{
    printf("%s\r\n",__func__);
}
static void group_alarm(sensor_device_t sensor, void *cfg, uint8_t num)
{
    printf("%s\r\n",__func__);
}
static void group_report(sensor_device_t sensor, void *cfg, uint8_t num)
{
    printf("%s\r\n",__func__);
}
static void group_storage(sensor_device_t sensor, void *cfg, uint8_t num)
{
    printf("%s\r\n",__func__);
}