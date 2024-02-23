/**
 * @file sensor_builder_default.h
 * @brief 
 * @author huangly 
 * @version 1.0
 * @date 2024-02-07
 * 
 * @copyright Copyright (c) 2024  
 * 
 * @note :
 * @par 修改日志:
 * Date       Version Author      Description
 * 2024-02-07 1.0     huangly     first version
 */
#ifndef __SENSOR_DEFAULT_H__
#define __SENSOR_DEFAULT_H__

#ifdef __cplusplus
extern "C" {
#endif
/* Includes ------------------------------------------------------------------*/
#include "sensor_builder.h"
/* Exported types ------------------------------------------------------------*/
typedef struct sensor_default_cfg *sensor_default_cfg_t;
/**
 * @brief  传感器默认配置接口
 * @note   
 */
typedef struct 
{
    /**
     * @brief  允许采集计数
     * @note   例如USB接入时,不统计采集次数
     * @retval 返回OK表示成功,其他表示失败
     */
    bool    (*allow_cnt_handler)(sensor_default_cfg_t cfg);
    /**
     * @brief  损坏处理函数
     * @note   框架已有合法性检测传入
     * @retval 返回OK表示数据有效,其他表示数据无效
     */
    bool    (*fault_handler)(sensor_default_cfg_t cfg);
    /**
     * @brief  失败处理函数
     * @note   框架已有合法性检测传入
     * @retval 返回OK表示数据有效,其他表示数据无效
     */
    bool    (*fail_handler)(sensor_default_cfg_t cfg);
    /**
     * @brief  报警处理函数
     * @note   框架已有合法性检测传入
     * @retval 返回OK表示数据有效,其他表示数据无效
     */
    void    (*alarm_handler)(sensor_default_cfg_t cfg, void *data);
}sensor_default_ops_t;
/**
 * @brief  传感器默认配置类
 * @note   
 */
struct sensor_default_cfg
{
    //配置项
    float  power;                       //传感器功耗
    uint8_t unit;                       //传感器单位
    uint8_t allow_retry_collect_cnt;    //允许重采次数
    uint8_t allow_collect_fail_cnt;     //允许采集失败次数
    uint32_t cal_addr;                  //校准数据存储地址
    struct 
    {
        int16_t max;            //检测最大值
        int16_t min;            //检测最小值
        uint32_t fail_count;    //检测失败次数
    }check;
    struct 
    {
        bool        normal;     //正常标志 false:损坏 true:正常
        uint8_t     err_cnt;    //采集错误次数
        uint8_t     fail_count; //采集失败次数
        uint32_t    count;      //采集次数
    }collect;
    sensor_default_ops_t ops;
};
/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported variables ---------------------------------------------------------*/
extern sensor_builder_ops_t default_builder_ops;
/* Exported functions prototypes ---------------------------------------------*/
void default_collect(sensor_device_t sensor, void *cfg, uint8_t num);
void default_calibration(sensor_device_t sensor, void *cfg, uint8_t num);
void default_range_check(sensor_device_t sensor, void *cfg, uint8_t num);
void default_data_check(sensor_device_t sensor, void *cfg, uint8_t num);
void default_alarm(sensor_device_t sensor, void *cfg, uint8_t num);

#ifdef __cplusplus
}
#endif

#endif /* __SENSOR_DEFAULT_H__ */
