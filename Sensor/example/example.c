/**
 * @File Name: app_sensor.c
 * @brief  
 * @Author : 
 * @Version : 1.0
 * @Creat Date : 2024-01-24
 * 
 * @copyright Copyright (c) 2024 
 * @par 修改日志:
 * Date           Version     Author  Description
 * 2024-01-24     v1.0        huagnly 内容
*/
/* Includes ------------------------------------------------------------------*/
#include "sensor_builder.h"
#include "sensor_default.h"
/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static void temperature_alarm(sensor_default_cfg_t cfg, void *data);
static bool allow_collect(sensor_device_t sensor, void *cfg);
static sensor_process_ops_t default_process[] = 
{
    {   .allow      = &allow_collect,
        .handler    = &default_collect},
    {   .handler    = &default_calibration},
    {   .handler    = &default_range_check},
    {   .handler    = &default_data_check},
    {   .handler    = &default_alarm},
};
};
/* ------------------------------sht3x--------------------------------------- */
#if (SHT3X_NUM != 0)
//传感器动作构建
static sensor_builder_t sht3x_builder[SHT3X_NUM] = 
{
#if(I2C1_ENABLE == 1)
    {
        .process = default_process,
        .process_num = sizeof(default_process) / sizeof(sensor_process_ops_t),
        .ops = &default_builder_ops,
    },
#endif //I2C1_ENABLE
#if(I2C3_ENABLE == 1)
    {
        .process = default_process,
        .process_num = sizeof(default_process) / sizeof(sensor_process_ops_t),
        .ops = &default_builder_ops,
    },
#endif //I2C3_ENABLE
};
static struct sensor_default_cfg sht3x_cfg[SHT3X_NUM][2] = 
{
#if(I2C1_ENABLE == 1)
    {
        [SENSOR_DATA_TEMPERATURE] = 
        {
            .power = SHT3X_CONSUME,
            .unit = 10,//0.1C
            .cal_addr = (uint32_t)&((user_config_t *)FLASH_USR_CONFIG_ADDR)->sensor_cfg[SENSOR_CFG_TEMP],
            .check = 
            {
                .max = 125,
                .min = -44,
            },
            .ops = 
            {
                .alarm_handler = temperature_alarm,
            }
        },
        [SENSOR_DATA_HUMIDITY] = 
        {
            .power = SHT3X_CONSUME,
            .unit = 1,//1%
            .cal_addr = (uint32_t)&((user_config_t *)FLASH_USR_CONFIG_ADDR)->sensor_cfg[SENSOR_CFG_HUMI],
            .check = 
            {
                .max = 100,
                .min = 0,
            },
        },
    },
#endif //I2C1_ENABLE
#if(I2C3_ENABLE == 1)
    {
        [SENSOR_DATA_TEMPERATURE] = 
        {
            .power = SHT3X_CONSUME,
            .unit = 10,//0.1C
            .cal_addr = (uint32_t)&((user_config_t *)FLASH_USR_CONFIG_ADDR)->sensor_cfg[SENSOR_CFG_TEMP],
            .check = 
            {
                .max = 125,
                .min = -44,
            },
            .ops = 
            {
                .alarm_handler = temperature_alarm,
            }
        },
        [SENSOR_DATA_HUMIDITY] = 
        {
            .power = SHT3X_CONSUME,
            .unit = 1,//1%
            .cal_addr = (uint32_t)&((user_config_t *)FLASH_USR_CONFIG_ADDR)->sensor_cfg[SENSOR_CFG_HUMI],
            .check = 
            {
                .max = 100,
                .min = 0,
            },
        },
    },
#endif //I2C3_ENABLE
};
#endif //#(SHT3X_NUM != 0)
/* ----------------------------ds18b20--------------------------------------- */
#if(DS18B20_ENABLE == 1)
//传感器动作构建
static sensor_builder_t ds18b20_builder = 
{
    .allow_mode = false,
    .process = default_process,
    .process_num = sizeof(default_process) / sizeof(sensor_process_ops_t),
    .ops = &default_builder_ops,
};

static struct sensor_default_cfg ds18b20_cfg = 
{
    .power = DS18B20_CONSUME,
    .unit = 10,//0.1C
    .cal_addr = (uint32_t)&((user_config_t *)FLASH_USR_CONFIG_ADDR)->sensor_cfg[SENSOR_CFG_TEMP],
    .check = 
    {
        .max = 85,
        .min = -55,
    },
    .ops = 
    {
        .alarm_handler = temperature_alarm,
    }
};
#endif //DS18B20_ENABLE == 1
/* Private function prototypes -----------------------------------------------*/
extern void sensor_register(void);
/* Private user code ---------------------------------------------------------*/
/* ------------------------------默认-----------------------------------------*/  
/**
 * @brief  允许采集判断
 * @note   仅当传感器初始化完成后允许采集
 * @param  sensor: 传感器
 * @param  *cfg: 配置
 * @retval true: 允许 false: 不允许
 */
static bool allow_collect(sensor_device_t sensor, void *cfg)
{
#if (INIT_UART1_ENABLE == 0)
    return true;
#else
    printf("[%s]allow %s\r\n", sensor->name, (g_sensor_init_flag == true) ? "true" : "false");
    return g_sensor_init_flag;
#endif //INIT_UART1_ENABLE == 0
}
/**
 * @brief 外部调用传感器校准
 * @param *name: 传感器名称
 * @return void
 */
void sensor_change_to_update(char *name)
{
    sensor_device_t sensor = sensor_obj_get(name);
    if(sensor != NULL) {
        sensor_builder_t *builder = (sensor_builder_t *)sensor->arg;
        sensor_default_cfg_t cfg = (sensor_default_cfg_t )builder->cfg;
        default_calibration(sensor, cfg, builder->cfg_num);
    }
}
/**
 * @brief  传感器采集功耗获取
 * @note   获取后清除本次采集次数
 * @param  *name: 传感器名称
 * @param  *cnt: 采集次数
 * @param  *power: 采集功耗
 * @retval 采集次数
 */
void sensor_get_consume(char *name, uint32_t *cnt, float *power)
{
    sensor_device_t sensor = sensor_obj_get(name);
    if(sensor != NULL) {
        sensor_builder_t *builder = (sensor_builder_t *)sensor->arg;
        sensor_default_cfg_t cfg = (sensor_default_cfg_t )builder->cfg;
        *cnt = cfg->collect.count;
        *power = cfg->power;
    }
}
/**
 * @brief  获取传感器数据单位
 * @note   
 * @param  *name: 传感器名称
 * @retval 数据单位 成功返回单位,失败返回0XFF
 */
uint8_t sensor_unit_get(char *name, sensor_data_e id)
{
    sensor_device_t sensor = sensor_obj_get(name);
    if(sensor != NULL) {
        sensor_builder_t *builder = (sensor_builder_t *)sensor->arg;
        sensor_default_cfg_t cfg = (sensor_default_cfg_t )builder->cfg;
        return cfg[id].unit;
    } else {
        return 0XFF;
    }
}
/**
 * @brief  获取传感器数据
 * @note   
 * @param  *data: 数据
 */
data_status_e sensor_data_get(char *name, float *data, sensor_data_e id)
{
    sensor_device_t sensor = sensor_obj_get(name);
    if(sensor != NULL) {
        data_status_e status = DATA_STATUS_NONE;
        sensor_control(sensor, SENSOR_CMD_STATUS_GET, &status, &id);
        sensor_control(sensor, SENSOR_CMD_DATA_GET, data, &id);
        return status;
    } else {
        return DATA_STATUS_INVALID;
    }
}
/**
 * @brief  传感器应用任务
 * @note   None
 * @param  *argument:
 * @retval None
 */
void StartSensorTask(void *argument)
{
    sensor_device_t sensor = NULL;

    sensor_register();
#if(DS18B20_ENABLE == 1)
    //注册DS18B20传感器
    sensor = sensor_obj_get("ds18b20");
    if(sensor != NULL) {
        builder_sensor_add(&ds18b20_builder, sensor);
        builder_config_add(&ds18b20_builder, &ds18b20_cfg, sizeof(ds18b20_cfg) / sizeof(struct sensor_default_cfg), true);//使用默认配置
        ds18b20_cfg.unit = 10;//0.1C
        sensor_builder_add(&ds18b20_builder);
    }
#endif  //DS18B20_ENABLE
#if (INIT_UART1_ENABLE == 0)
#if (I2C1_ENABLE == 1)
    //注册SHT3X_0传感器
    sensor = sensor_obj_get("sht3x_0");
    if(sensor != NULL) {
        builder_config_add(&sht3x_builder[SHT3X_ID_I2C1], &sht3x_cfg[SHT3X_ID_I2C1], sizeof(sht3x_cfg[SHT3X_ID_I2C1]) / sizeof(struct sensor_default_cfg),true);//使用默认配置
        sht3x_cfg[SHT3X_ID_I2C1][SENSOR_DATA_TEMPERATURE].unit = 10;//0.1C
        builder_sensor_add(&sht3x_builder[SHT3X_ID_I2C1], sensor);
        sensor_builder_add(&sht3x_builder[SHT3X_ID_I2C1]);
    }
#endif  //I2C1_ENABLE
#endif //INIT_UART1_ENABLE
#if (I2C3_ENABLE == 1)
    //注册SHT3X_1传感器
    sensor = sensor_obj_get("sht3x_1");
    if(sensor != NULL) {
        builder_config_add(&sht3x_builder[SHT3X_ID_I2C3], &sht3x_cfg[SHT3X_ID_I2C3], sizeof(sht3x_cfg[SHT3X_ID_I2C3]) / sizeof(struct sensor_default_cfg),true);//使用默认配置
        sht3x_cfg[SHT3X_ID_I2C3][SENSOR_DATA_TEMPERATURE].unit = 10;//0.1C
        builder_sensor_add(&sht3x_builder[SHT3X_ID_I2C3], sensor);
        sensor_builder_add(&sht3x_builder[SHT3X_ID_I2C3]);
    }
#endif //I2C3_ENABLE
    sensor_director_init();

    while (1) {
        sensor_director_process();
    }
}
