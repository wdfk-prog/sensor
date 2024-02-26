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
#include "sensor_group.h"
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
static rt_list_t _sensor_list = RT_LIST_OBJECT_INIT(_sensor_list);

static bool group_sensor_add(sensor_builder_t *builder, sensor_device_t sensor);
static bool group_sensor_init(sensor_builder_t *builder);
static bool group_config_add(sensor_builder_t *builder, void *cfg, uint8_t len, bool group_flag);
sensor_builder_ops_t group_builder_ops = 
{
    .sensor_add = group_sensor_add,
    .sensor_init = group_sensor_init,
    .config_add = group_config_add,
};
/* Private function prototypes -----------------------------------------------*/
/**
 * @brief  构建器添加传感器
 * @note   必须具有传感器操作函数
 * @param  *builder: 构建器
 * @param  sensor: 传感器
 * @retval true: 成功 false: 失败
 */
static bool group_sensor_add(sensor_builder_t *builder, sensor_device_t sensor)
{
    if(sensor->ops == NULL) {
        return false;
    } else {
        rt_list_insert_before(&_sensor_list, &sensor->cfg_node);
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
 * @param  group_flag: 是否初始化为默认配置
 * @retval true: 成功 false: 失败
 */
static bool group_config_add(sensor_builder_t *builder, void *cfg, uint8_t len, bool group_flag)
{
    builder->cfg = cfg;
    builder->cfg_num = len;

    if(group_flag == true) {
        sensor_group_cfg_t sensor_cfg = (sensor_group_cfg_t)cfg;
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
static bool group_sensor_init(sensor_builder_t *builder)
{
    if(rt_list_isempty(&_sensor_list)) {
        return false;
    }

    bool ret = true;
    sensor_device_t sensor;
    rt_list_for_each_entry(sensor, &_sensor_list, cfg_node) {
        ret = sensor_init(builder->sensor);
        if(ret != true) {
            break;
        }
    }
    return ret;
}
/**
 * @brief  默认传感器数据采集处理
 * @note   支持多个传感器数据采集
 * @param  sensor: 传感器设备
 * @param  *cfg: 构建器配置
 */
void group_collect(sensor_device_t input, void *cfg, uint8_t num)
{
    if(cfg == NULL) {
        return;
    }
    if(rt_list_isempty(&_sensor_list)) {
        return;
    }

    sensor_device_t sensor;
    rt_list_for_each_entry(sensor, &_sensor_list, cfg_node) {
        sensor_builder_t *builder = (sensor_builder_t *)sensor->arg;
        sensor_group_cfg_t sensor_cfg = (sensor_group_cfg_t )builder->cfg;
        //采集前判断是否允许统计采集次数
        bool allow_flag = true;
        if(sensor_cfg->ops.allow_cnt_handler != NULL) {
            allow_flag = sensor_cfg->ops.allow_cnt_handler(sensor_cfg);
        }
        if(allow_flag == true) {
            sensor_cfg->collect.count++;
        }

        bool ret = true;
        if(sensor_cfg->global_power != true) {
            ret = sensor_open(sensor);
        }
        if(ret == true) {
            ret = sensor_collect(sensor);
        }
        if(sensor_cfg->global_power != true) {
            sensor_close(sensor);
        }

        while (ret != true) {
            if(sensor_cfg->collect.err_cnt < sensor_cfg->allow_retry_collect_cnt) {
                sensor_cfg->collect.err_cnt++;
                printf_info("[%s][retry]collect%d/%d\r\n", sensor->name, sensor_cfg->collect.err_cnt, sensor_cfg->allow_retry_collect_cnt);
                if(allow_flag == true) {
                    sensor_cfg->collect.count++;
                }
                //重启传感器
                sensor_close(sensor);
                ret = sensor_open(sensor);
                if(ret == true) {
                    ret = sensor_collect(sensor);
                }
                sensor_close(sensor);
            } else {
                if (sensor_cfg->collect.normal == false) {
                    if(sensor_cfg->ops.fault_handler != NULL) {
                        sensor_cfg->ops.fault_handler(sensor_cfg);
                    } else {
                        //默认处理
                        printf_info("[%s][error]fault\r\n", sensor->name);

                    }
                } else {
                    if(sensor_cfg->ops.fail_handler != NULL) {
                        sensor_cfg->ops.fail_handler(sensor_cfg);
                    } else {
                        //默认处理
                        sensor_cfg->collect.fail_count++;
                        printf_info("[%s][fail]collect%d/%d\r\n", sensor->name, sensor_cfg->collect.fail_count, sensor_cfg->allow_collect_fail_cnt);
                        if(sensor_cfg->collect.fail_count > sensor_cfg->allow_collect_fail_cnt) {
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
}
/**
 * @brief  默认传感器数据校准处理
 * @note   支持多个传感器数据校准float类型校准
 * @param  sensor: 传感器设备
 * @param  *cfg: 构建器配置
 */
void group_calibration(sensor_device_t input, void *cfg, uint8_t num)
{
    if(cfg == NULL) {
        return;
    }
    sensor_device_t sensor;
    rt_list_for_each_entry(sensor, &_sensor_list, cfg_node) {
        sensor_builder_t *builder = (sensor_builder_t *)sensor->arg;
        sensor_group_cfg_t sensor_cfg = (sensor_group_cfg_t )builder->cfg;

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
                int16_t temp = (int16_t)(data * sensor_cfg[i].unit);
                printf_debug("[%s]num[%d][original]%d[cal]%d\r\n", sensor->name, i, temp, sensor_params.calibration_value);
                data = (float)(temp + sensor_params.calibration_value) / sensor_cfg[i].unit;
                sensor_control(sensor, SENSOR_CMD_DATA_SET, &data, &i);
            }
        }
    }
}
/**
 * @brief  默认传感器范围检测处理
 * @note   支持多个传感器数据校准float类型范围检查
 * @param  sensor: 传感器设备
 * @param  *cfg: 构建器配置
 */
void group_range_check(sensor_device_t input, void *cfg, uint8_t num)
{
    if(cfg == NULL) {
        return;
    }
    sensor_device_t sensor;
    rt_list_for_each_entry(sensor, &_sensor_list, cfg_node) {
        sensor_builder_t *builder = (sensor_builder_t *)sensor->arg;
        sensor_group_cfg_t sensor_cfg = (sensor_group_cfg_t )builder->cfg;

        float data = 0;
        uint8_t id = 0;
        printf_debug("[%s]range[%d ~ %d]\r\n", sensor->name, sensor_cfg[id].check.min, sensor_cfg[id].check.max);

        sensor_control(sensor, SENSOR_CMD_DATA_GET, &data, &id);
        int16_t temp = data * sensor_cfg[id].unit;
        if(sensor_cfg[id].check.min * sensor_cfg[id].unit <= temp && temp <= sensor_cfg[id].check.max * sensor_cfg[id].unit) {
            sensor_cfg[id].check.fail_count = 0;
        } else {
            data_status_e status = DATA_STATUS_OUTRANGE;
            sensor_control(sensor, SENSOR_CMD_STATUS_SET, &status, &id);
            sensor_cfg[id].check.fail_count++;
        }
    }
}
/**
 * @brief  默认传感器数据检查处理
 * @note   支持多个传感器数据校准float类型数据检查
 * @param  sensor: 传感器设备
 * @param  *cfg: 构建器配置
 */
void group_data_check(sensor_device_t input, void *cfg, uint8_t num)
{
    float data = 0;
    uint8_t id = 0;
    data_status_e status = DATA_STATUS_NONE;
    sensor_device_t sensor;
    rt_list_for_each_entry(sensor, &_sensor_list, cfg_node) {
        sensor_control(sensor, SENSOR_CMD_STATUS_GET, &status, &i);
        if(status == DATA_STATUS_INVALID) {
            data = SENSOR_ERROR_DATA / sensor_cfg[i].unit;
            sensor_control(sensor, SENSOR_CMD_DATA_SET, &data, &i);
        } else if(status == DATA_STATUS_OUTRANGE) {
            data = SENSOR_OUTRANGE_DATA / sensor_cfg[i].unit;
            sensor_control(sensor, SENSOR_CMD_DATA_SET, &data, &i);
        }

        sensor_control(sensor, SENSOR_CMD_DATA_GET, &data, &id);
        printf_debug("[%s]data[%s]\r\n", sensor->name, ftoc(data, 3));
    }
}
/**
 * @brief  默认传感器数据报警处理
 * @note   
 * @param  sensor: 传感器设备
 * @param  *cfg: 构建器配置
 * @retval None
 */
void group_alarm(sensor_device_t input, void *cfg, uint8_t num)
{
    if(cfg == NULL) {
        return;
    }
    uint8_t id = 0;
    sensor_device_t sensor;
    data_status_e status = DATA_STATUS_NONE;
    rt_list_for_each_entry(sensor, &_sensor_list, cfg_node) {
        sensor_builder_t *builder = (sensor_builder_t *)sensor->arg;
        sensor_group_cfg_t sensor_cfg = (sensor_group_cfg_t )builder->cfg;
        sensor_control(sensor, SENSOR_CMD_STATUS_GET, &status, &id);
        if(sensor_cfg[id].ops.alarm_handler == NULL || status != DATA_STATUS_VALID) {
            return;
        }

        float data = 0;
        sensor_control(sensor, SENSOR_CMD_DATA_GET, &data, &id);
        sensor_cfg[id].ops.alarm_handler(sensor_cfg, &data);
    }
}
