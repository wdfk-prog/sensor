/**
 * @File Name: sensor_driver.h
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
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SENOSR_DRIVER_H__
#define __SENOSR_DRIVER_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "cmsis_os2.h"
#include "rt_list.h"

#include "node_convert.h"
/* Exported constants --------------------------------------------------------*/
#define SENSOR_MODULE_MAX       (3)         //传感器模块的最大成员数
#define SENSOR_ERROR_DATA       0XFFFFFFFF  //错误数据
#define SENSOR_OUTRANGE_DATA    0XFFFFFFFD  //超量程数据
/* Exported macro ------------------------------------------------------------*/
#define  SENSOR_DEBUG_ENABLE    0           //传感器调试使能
#if (SENSOR_DEBUG_ENABLE == 1)
#define  sensor_printf(...)      printf(__VA_ARGS__)
#else

#define  sensor_printf(...)
#endif

/**
 * @brief  寻找配置指针
 * @param  cfg_t: 配置类型
 * @param  dev: 设备指针
 * @note   find_cfg函数需在传感器驱动中自行编写
 */
#define FIND_CFG(cfg_t, dev)                                            \
    cfg_t *config = find_cfg(dev);                                      \
    if(config == NULL) {                                                \
        return false;                                                   \
    }                                                                   \
    sensor_printf("[dev:%s] %s\r\n", dev->name, __func__);
/* Exported enum -------------------------------------------------------------*/
/**
 * @brief 传感器数据获取ID
 */
typedef enum
{
    SENSOR_DATA_DEFAULT = 0,    //默认数据
    SENSOR_DATA_GET_RAW = 0XFF, //获取原始数据
}sensor_data_id_e;
/**
 * @brief 传感器数据状态
 */
typedef enum 
{
    DATA_STATUS_INVALID = 0, //数据无效
    DATA_STATUS_VALID,       //数据有效
    DATA_STATUS_OUTRANGE,    //数据超量程
    DATA_STATUS_NONE = 0XFF, //无状态
} data_status_e;
/**
 * @brief  传感器模块状态
 * @note   None
 */
typedef enum
{
    SENSOR_MODULE_INIT,     //模块初始化
    SENSOR_MODULE_OPEN,     //模块开启
    SENSOR_MODULE_CLOSE,    //模块关闭
    SENSOR_MODULE_READ,     //模块读取
    SENSOR_MODULE_WRITE,    //模块写入
    SENSOR_MODULE_CTRL,     //模块控制
    SENSOR_MODULE_LPM_IN,   //模块低功耗进入
    SENSOR_MODULE_LPM_OUT,  //模块低功耗退出
}sensor_module_e;
/**
 * @brief  传感器控制命令
 * @note   None
 */
typedef enum
{
    SENSOR_CMD_NONE,
    SENSOR_CMD_STATUS_SET,  //状态设置
    SENSOR_CMD_STATUS_GET,  //状态获取
    SENSOR_CMD_DATA_SET,    //数据设置
    SENSOR_CMD_DATA_GET,    //数据获取
}sensor_cmd_e;
/* Exported types ------------------------------------------------------------*/
typedef struct sensor_device *sensor_device_t;
/**
 * @brief  传感器模块
 * @note   不同传感器在同一模块中使用,需要填写此内容
 */
typedef struct
{
    sensor_module_e status;                 //模块状态
    sensor_device_t sen[SENSOR_MODULE_MAX]; //模块包含的传感器
    uint8_t         sen_num;                //模块中包含的传感器数量

    bool    (*init)(sensor_device_t dev);
    bool    (*open)(sensor_device_t dev);
    bool    (*close)(sensor_device_t dev);
}sensor_module_t;
/**
 * @brief  传感器接口
 * @note   None
 */
typedef struct
{
    bool (*init)(sensor_device_t dev);
    bool (*open)(sensor_device_t dev);
    bool (*close)(sensor_device_t dev);
    bool (*collect)(sensor_device_t dev);
    bool (*lpm)(sensor_device_t dev, bool lpm_flag);
    bool (*control)(sensor_device_t dev, sensor_cmd_e cmd, void *data, void *arg);
}sensor_ops_t;
/**
 * @brief  传感器设备
 * @note   None
 */
struct sensor_device
{
    char *name;

    rt_list_t node;

    const sensor_ops_t  *ops;
    sensor_module_t     *module;    //模块,不同传感器在同一模块中使用,需要填写此内容
    void                *arg;       //传感器参数
};
/* Exported variables --------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
bool sensor_register_fun(sensor_device_t dev);
sensor_device_t sensor_obj_get(const char *reg_name);

bool sensor_init(sensor_device_t dev);
bool sensor_open(sensor_device_t dev);
bool sensor_close(sensor_device_t dev);
bool sensor_collect(sensor_device_t dev);
bool sensor_lpm(sensor_device_t dev, bool lpm_flag);
bool sensor_data_control(sensor_device_t dev, sensor_cmd_e cmd, void *data, void *arg);

#ifdef __cplusplus
extern "C" }
#endif

#endif  /* __SENOSR_DRIVER_H__ */
