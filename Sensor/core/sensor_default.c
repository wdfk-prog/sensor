/**
 * @file sensor_default.c
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
#include "sensor_default.h"
/* Private includes ----------------------------------------------------------*/
#include "module_debug.h"

#include "board_system.h"
#include "board_params.h"
/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define DEFAULT_ALLOW_RETRY_COLLECT_CNT 2   //默认最大允许重新采集次数
#define DEFAULT_ALLOW_COLLECT_FAIL_CNT  3   //默认最大允许采集失败次数
/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static bool default_sensor_add(sensor_builder_t *builder, sensor_device_t sensor);
static bool default_sensor_init(sensor_builder_t *builder);
static bool default_config_add(sensor_builder_t *builder, void *cfg, uint8_t len, bool default_flag);
sensor_builder_ops_t default_builder_ops = 
{
    .sensor_add = default_sensor_add,
    .sensor_init = default_sensor_init,
    .config_add = default_config_add,
};
/* Private function prototypes -----------------------------------------------*/
/**
 * @brief  构建器添加传感器
 * @note   必须具有传感器操作函数
 * @param  *builder: 构建器
 * @param  sensor: 传感器
 * @retval true: 成功 false: 失败
 */
static bool default_sensor_add(sensor_builder_t *builder, sensor_device_t sensor)
{
    if(sensor->ops == NULL) {
        return false;
    } else {
        builder->sensor = sensor;
        sensor->arg = builder;
        return true;
    }
}
/**
 * @brief  构建器配置添加
 * @note   启动默认配置,将对所有配置进行初始化
 * @param  *builder: 构建器
 * @param  *cfg: 配置项
 * @param  default_flag: 是否初始化为默认配置
 * @retval true: 成功 false: 失败
 */
static bool default_config_add(sensor_builder_t *builder, void *cfg, uint8_t len, bool default_flag)
{
    builder->cfg = cfg;
    builder->cfg_num = len;

    if(default_flag == true) {
        sensor_default_cfg_t sensor_cfg = (sensor_default_cfg_t)cfg;
        for(uint8_t i = 0; i < builder->cfg_num; i++) {
            sensor_cfg[i].allow_retry_collect_cnt = DEFAULT_ALLOW_RETRY_COLLECT_CNT;
            sensor_cfg[i].allow_collect_fail_cnt = DEFAULT_ALLOW_COLLECT_FAIL_CNT;
            //默认传感器损坏,直到采集到数据
            sensor_cfg[i].collect.normal = false;
            sensor_cfg[i].unit = 1;
        }
    }

    return true;
}
/**
 * @brief  执行传感器初始化
 * @note   None
 * @param  *builder: 构建器
 * @retval true: 成功 false: 失败
 */
static bool default_sensor_init(sensor_builder_t *builder)
{
    return sensor_init(builder->sensor);
}
/**
 * @brief  默认传感器数据采集处理
 * @note   支持单个传感器采集;不支持多个配置运行;多个配置仅对第一个配置进行处理
 * @param  sensor: 传感器设备
 * @param  *cfg: 构建器配置
 */
void default_collect(sensor_device_t sensor, void *cfg, uint8_t num)
{
    if(cfg == NULL) {
        return;
    }
    sensor_default_cfg_t sensor_cfg = (sensor_default_cfg_t)cfg;

    //采集前判断是否允许统计采集次数
    bool allow_flag = true;
    if(sensor_cfg[0].ops.allow_cnt_handler != NULL) {
        allow_flag = sensor_cfg[0].ops.allow_cnt_handler(sensor_cfg);
    }
    if(allow_flag == true) {
        sensor_cfg[0].collect.count++;
    }

    bool ret = sensor_open(sensor);
    if(ret == true) {
        ret = sensor_collect(sensor);
    }
    sensor_close(sensor);

    while (ret != true) {
        if(sensor_cfg[0].collect.err_cnt < sensor_cfg[0].allow_retry_collect_cnt) {
            sensor_cfg[0].collect.err_cnt++;
            printf_info("[%s][retry]collect%d/%d\r\n", sensor->name, sensor_cfg[0].collect.err_cnt, sensor_cfg[0].allow_retry_collect_cnt);
            if(allow_flag == true) {
                sensor_cfg[0].collect.count++;
            }
            //重启传感器
            sensor_close(sensor);
            ret = sensor_open(sensor);
            if(ret == true) {
                ret = sensor_collect(sensor);
            }
            sensor_close(sensor);
        } else {
            if (sensor_cfg[0].collect.normal == false) {
                if(sensor_cfg[0].ops.fault_handler != NULL) {
                    sensor_cfg[0].ops.fault_handler(sensor_cfg);
                } else {
                    //默认处理
                    printf_info("[%s][error]fault\r\n", sensor->name);

                }
            } else {
                if(sensor_cfg[0].ops.fail_handler != NULL) {
                    sensor_cfg[0].ops.fail_handler(sensor_cfg);
                } else {
                    //默认处理
                    sensor_cfg[0].collect.fail_count++;
                    printf_info("[%s][fail]collect%d/%d\r\n", sensor->name, sensor_cfg[0].collect.fail_count, sensor_cfg[0].allow_collect_fail_cnt);
                    if(sensor_cfg[0].collect.fail_count > sensor_cfg[0].allow_collect_fail_cnt) {
                        device_restart();
                    }
                }
            }
            break;
        }
    }
    float data = 0;
    data_status_e status = DATA_STATUS_VALID;
    uint8_t data_id = 0;
    for(uint8_t i = 0; i < num; i++) {
        if(ret == true) {
            sensor_cfg[i].collect.err_cnt = 0;
            sensor_cfg[i].collect.fail_count = 0;
            sensor_cfg[i].collect.normal = true;
            data_id = SENSOR_DATA_GET_RAW - i;
            sensor_control(sensor, SENSOR_CMD_STATUS_SET, &status, &i);
            sensor_control(sensor, SENSOR_CMD_DATA_GET, &data, &data_id);
            sensor_control(sensor, SENSOR_CMD_DATA_SET, &data, &i);
        } else {
            status = DATA_STATUS_INVALID;
            sensor_control(sensor, SENSOR_CMD_STATUS_SET, &status, &i);
        }
    }
}
/**
 * @brief  默认传感器数据校准处理
 * @note   支持多个传感器数据校准float类型校准
 * @param  sensor: 传感器设备
 * @param  *cfg: 构建器配置
 */
void default_calibration(sensor_device_t sensor, void *cfg, uint8_t num)
{
    if(cfg == NULL) {
        return;
    }
    sensor_default_cfg_t sensor_cfg = (sensor_default_cfg_t)cfg;

    float data = 0;
    sensor_params_t sensor_params = {0};
    data_status_e status = DATA_STATUS_NONE;
    for(uint8_t i = 0; i < num; i++) {
        sensor_control(sensor, SENSOR_CMD_STATUS_GET, &status, &i);
        if(status != DATA_STATUS_VALID) {
            return;
        }
        if(sensor_cfg[i].cal_addr == 0) {
            printf_error("[%s]num[%d]calibration addr is invalid\r\n", sensor->name, i);
            return;
        }

        read_data_from_flash((uint32_t *)&sensor_params, sizeof(sensor_params), sensor_cfg[i].cal_addr);
        if(sensor_params.calibration_enable == true) {
            uint8_t data_id = SENSOR_DATA_GET_RAW - i;
            sensor_control(sensor, SENSOR_CMD_DATA_GET, &data, &data_id);
            float cal = (float)sensor_params.calibration_value / sensor_cfg[i].unit;
            printf_debug("[%s]num[%d][cal]%s\r\n", sensor->name, i, ftoc(cal, 3));
            data += cal;
            sensor_control(sensor, SENSOR_CMD_DATA_SET, &data, &i);
        }
    }
}
/**
 * @brief  默认传感器范围检测处理
 * @note   支持多个传感器数据校准float类型范围检查
 * @param  sensor: 传感器设备
 * @param  *cfg: 构建器配置
 */
void default_range_check(sensor_device_t sensor, void *cfg, uint8_t num)
{
    if(cfg == NULL) {
        return;
    }
    float data = 0;
    for(uint8_t i = 0; i < num; i++) {
        data_status_e status = DATA_STATUS_NONE;
        sensor_control(sensor, SENSOR_CMD_STATUS_GET, &status, &i);
        if(status != DATA_STATUS_VALID) {
            continue;
        }
        sensor_default_cfg_t sensor_cfg = (sensor_default_cfg_t)cfg;
        printf_debug("[%s]num[%d]range[%d ~ %d]\r\n", sensor->name, i, sensor_cfg[i].check.min, sensor_cfg[i].check.max);

        sensor_control(sensor, SENSOR_CMD_DATA_GET, &data, &i);
        int16_t temp = data * sensor_cfg[i].unit;
        if(sensor_cfg[i].check.min * sensor_cfg[i].unit <= temp && temp <= sensor_cfg[i].check.max * sensor_cfg[i].unit) {
            sensor_cfg[i].check.fail_count = 0;
        } else {
            data_status_e status = DATA_STATUS_OUTRANGE;
            sensor_control(sensor, SENSOR_CMD_STATUS_SET, &status, &i);
            sensor_cfg[i].check.fail_count++;
        }
    }
}
/**
 * @brief  默认传感器数据检查处理
 * @note   支持多个传感器数据校准float类型数据检查
 * @param  sensor: 传感器设备
 * @param  *cfg: 构建器配置
 */
void default_data_check(sensor_device_t sensor, void *cfg, uint8_t num)
{
    float data = 0;
    data_status_e status = DATA_STATUS_NONE;
    for(uint8_t i = 0; i < num; i++) {
        sensor_default_cfg_t sensor_cfg = (sensor_default_cfg_t)cfg;
        sensor_control(sensor, SENSOR_CMD_STATUS_GET, &status, &i);
        if(status == DATA_STATUS_INVALID) {
            data = sensor_cfg[i].data_status.error;
            sensor_control(sensor, SENSOR_CMD_DATA_SET, &data, &i);
        } else if(status == DATA_STATUS_OUTRANGE) {
            data = sensor_cfg[i].data_status.outrange;
            sensor_control(sensor, SENSOR_CMD_DATA_SET, &data, &i);
        }

        sensor_control(sensor, SENSOR_CMD_DATA_GET, &data, &i);
        printf_debug("[%s]num[%d]data[%s]\r\n", sensor->name, i, ftoc(data, 3));
    }
}
/**
 * @brief  默认传感器数据报警处理
 * @note   
 * @param  sensor: 传感器设备
 * @param  *cfg: 构建器配置
 * @retval None
 */
void default_alarm(sensor_device_t sensor, void *cfg, uint8_t num)
{
    if(cfg == NULL) {
        return;
    }
    sensor_default_cfg_t sensor_cfg = (sensor_default_cfg_t)cfg;
    for(uint8_t i = 0; i < num; i++) {
        if(sensor_cfg[i].ops.alarm_handler == NULL) {
            return;
        }
        float data = 0;
        sensor_control(sensor, SENSOR_CMD_DATA_GET, &data, &i);
        sensor_cfg[i].ops.alarm_handler(sensor, sensor_cfg, &data);
    }
}