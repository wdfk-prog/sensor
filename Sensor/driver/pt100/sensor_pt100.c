/**
 * @File Name: sensor_pt100.c
 * @brief
 * @Author : 
 * @Version : 1.0
 * @Creat Date : 2023-06-19
 *
 * @copyright Copyright (c) 2023 
 * @par 修改日志:
 * Date           Version     Author  Description
 * 2023-06-19     v1.0        huagnly 内容
 */
/* Includes ------------------------------------------------------------------*/
#include "sensor_pt100.h"
/* Private includes ----------------------------------------------------------*/
#include "node_glbs.h"
#include "module_ntag.h"
/* Private typedef -----------------------------------------------------------*/
/**
 * @brief  电源控制状态
 * @note   None
 */
enum
{
    OFF,
    ON,
};
/* Private define ------------------------------------------------------------*/
//https://us.flukecal.com/pt100-calculator
//https://www.engineeringtoolbox.com/pt100-electrical-resistance-d_1651.html
#define A 3.9083e-3
#define B -5.775e-7
#define C -4.183e-12

#define COLLECT_NUM 5
#define REF_V       1950
/* Private macro -------------------------------------------------------------*/
#define POWER_DEBUG 0
#define POWER_DELAY 300
/* Private variables ---------------------------------------------------------*/
static pt100_cfg_t pt100_cfg[PT100_MAX_NUM] =
{
    //NUM0
    {
        .i2c =
        {
            .Id = ADS1015_IIC,
            .scl = ADS1015_SCL_PIN,
            .sda = ADS1015_SDA_PIN,
        },
        .collect_ch = PT100_0CH,
        .power =
        {
            .port = ADS1015_POWER_PORT,
            .pin = ADS1015_POWER_PIN,
            .on = ADS1015_POWERON_LEVEL,
            .ch = PT100_POWER_0CH,
        },
        .FSR    = FSR_0256,
    },
    //NUM1
    {
        .i2c =
        {
            .Id = ADS1015_IIC,
            .scl = ADS1015_SCL_PIN,
            .sda = ADS1015_SDA_PIN,
        },
        .collect_ch = PT100_1CH,
        .power =
        {
            .port = ADS1015_POWER_PORT,
            .pin = ADS1015_POWER_PIN,
            .on = ADS1015_POWERON_LEVEL,
            .ch = PT100_POWER_1CH,
        },
        .FSR    = FSR_0256,
    }
};
//传感器操作函数
static const sensor_ops_t pt100_ops;
static sensor_module_t ads1015;
pt100_device_t pt100[PT100_MAX_NUM] =
{
    //NUM0
    {
        .parent =
        {
            .name = "pt100_0",                  //设备名称
            .ops    = &pt100_ops,               //操作函数
            .module = &ads1015,                 //模块
        },
        .cfg = &pt100_cfg[PT100_0],             //配置信息
    },
    //NUM1
    {
        .parent =
        {
            .name = "pt100_1",                  //设备名称
            .ops    = &pt100_ops,               //操作函数
            .module = &ads1015,                 //模块
        },
        .cfg = &pt100_cfg[PT100_1],             //配置信息
    },
};
//TODO 此模块初始化应存放与单独文件中,暂时放入此处,无较好办法处理
static bool ads1015_open(sensor_device_t dev);
static bool ads1015_close(sensor_device_t dev);
static sensor_module_t ads1015 =
{
    .sen =
    {
        &pt100[PT100_0].parent,
        &pt100[PT100_1].parent,
    },
    .sen_num    = PT100_MAX_NUM,
    .status     = SENSOR_MODULE_CLOSE,
    .open       = ads1015_open,
    .close      = ads1015_close,
};
/* Private function prototypes -----------------------------------------------*/
static bool pt100_open(sensor_device_t dev);
static bool pt100_close(sensor_device_t dev);
static bool pt100_collect(sensor_device_t dev);
static bool pt100_control(sensor_device_t dev, sensor_cmd_e cmd, void *data, void *arg);
static const sensor_ops_t pt100_ops =
{
    .open       = pt100_open,
    .close      = pt100_close,
    .collect    = pt100_collect,
    .control    = pt100_control,
};
/* Private user code ---------------------------------------------------------*/
/**
 * @brief  pt100电源控制
 * @note   电源控制需延时400ms
 * @param  *config: 配置信息
 * @param  flag: true:开启 false:关闭
 * @retval None
 */
static void power_control(pt100_cfg_t *config, bool flag)
{
    if (flag == true) {
        HAL_GPIO_WritePin(config->power.port, config->power.pin, config->power.on);
        HAL_Delay(POWER_DELAY);
    } else
    {
        HAL_GPIO_WritePin(config->power.port, config->power.pin, !config->power.on);
    }
#if(POWER_DEBUG == 1)
    printf("power %lu\r\n", GpioRead(&config->power.obj));
#endif /* (POWER_DEBUG == 1) */
}
/**
 * @brief  寻找配置指针
 * @note   None
 * @param  dev: 设备句柄
 * @retval 返回配置指针
 */
static pt100_cfg_t *find_cfg(sensor_device_t dev)
{
    pt100_device_t *sensor = (pt100_device_t *)dev;
    if(dev == NULL || sensor == NULL || sensor->cfg == NULL) {
        return NULL;
    }
    return sensor->cfg;
}
/**
 * @brief  ads1015开启
 * @note   初始化IIC驱动,初始化ads1015
 * @param  dev: 设备句柄
 * @retval 错误码
 */
static bool ads1015_open(sensor_device_t dev)
{
    FIND_CFG(pt100_cfg_t, dev);
    config->i2c.obj = ntag_i2c_init();
    ads1015_init(config->i2c.obj);
    return true;
}
/**
 * @brief  ads1015关闭
 * @note   卸载I2C
 * @param  dev: 设备句柄
 * @retval
 */
static bool ads1015_close(sensor_device_t dev)
{
    FIND_CFG(pt100_cfg_t, dev);

    // I2cDeInit(&config->i2c.obj);
    return true;
}
/**
 * @brief  PT100初始化
 * @note   关闭电源
 * @param  dev: 设备句柄
 * @retval
 */
static bool pt100_open(sensor_device_t dev)
{
    FIND_CFG(pt100_cfg_t, dev);
    power_control(config, ON);
    return true;
}
/**
 * @brief  PT100关闭
 * @note   卸载I2C,关闭电源
 * @param  dev: 设备句柄
 * @retval
 */
static bool pt100_close(sensor_device_t dev)
{
    FIND_CFG(pt100_cfg_t, dev);
    power_control(config, OFF);
    return true;
}
/**
 * @brief  PT100计算
 * @note   检测范围 -200℃ ~ 850℃
 * Rt = R0 (1 + A*t + B*t2 + C (t - 100)*t3)
 * Rt = resistance at temperature t  (ohm)
 * R0 = resistance at temperature 0℃ (ohm)
 * t = temperature (oC)
 * PT100传感器在0°C时的电阻值为100欧姆;在100°C时的电阻值为138.5欧姆
 * 使用迭代公式计算出第二次迭代后的温度值。这个过程会一直重复，直到温度值收敛（即两次迭代后的温度值之差小于0.001）或者达到最大迭代次数（即50次）。
 * @param  resistance: 电阻值
 * @retval 温度值;返回3276.7表示错误
 */
static float pt100_calculation(float resistance)
{
    double fT = 0, fR = 0, fT0 = 0;

    //printf("===resistance = %f\n",resistance);
    fR = resistance;
    fT0 = (fR / 100 - 1) / A;
    if(fR >= 18.52 && fR < 100) { //-200C° ~ 0C°
        //而在0°C以下，使用的是完整版的Callendar-Van Dusen方程，即R(T) = R0(1+AT +B(T)2 +C(T)3(T − 100))。
        for(int i = 0;i < 50;i++) {
            //迭代法来求解温度值
            fT = fT0+(fR-100*(1+A*fT0+B*fT0*fT0-100*C*fT0*fT0*fT0+C*fT0*fT0*fT0*fT0))/(100*(A+2*B*fT0-300*C*fT0*fT0+4*C*fT0*fT0*fT0));
            if(fabs(fT-fT0 < 0.001)) {
                break;
            } else {
                fT0=fT;
            }
        }
//      printf("%.3f \r\n",fT);
    } else if(fR >= 100 && fR <= 390.481) { //0°C ~ 849.99°C
        //在0°C以上，使用的是简化版的Callendar-Van Dusen方程，即R(T) = R0(1+AT +B(T)2)；
        for(int i = 0;i < 50;i++) {
            fT = fT0+(fR-100*(1+A*fT0+B*fT0*fT0))/(100*(A+2*B*fT0));
            if(fabs(fT - fT0 < 0.001)) {
                break;
            } else {
                fT0=fT;
            }
        }
//      printf("%.3f \r\n",fT);
    } else {
        fT = 3276.7;
    }

    return (float)fT;
}
/**
 * @brief  滤波
 * @note   None
 * @param  *data:
 * @param  num:
 * @param  *result:
 * @retval
 */
static bool glbs_filter(ads1015_data_t *data, uint8_t num, float *result)
{
    float buffer[10] = {0};
    uint8_t count = 0;

    for(uint8_t i = 0; i < num && i < 10; i++) {
        if(data[i].succ) {
            buffer[count++] = data[i].value;
        }
    }

    if(count) {
        glbs_process(buffer, count, result);
        return true;
    } else {
        return false;
    }
}
/**
 * @brief  PT100数据采集
 * @note  None
 * @param  dev: 设备句柄
 * @param  *data: 数据存储地址
 * @param  len: 数据长度
 * @retval 错误码
 */
static bool pt100_collect(sensor_device_t dev)
{
    FIND_CFG(pt100_cfg_t, dev);

    HAL_StatusTypeDef ret = HAL_OK;

    float ref_voltage = 0;
    float ref_current = 0;
    float voltage = 0;
    float resistance = 0;
    ads1015_data_t ads1015_data[COLLECT_NUM] = {0};
    printf("[%s]collect\r\n", dev->name);
    //采集电源数据
    ret = ads1015_extend_collect(config->power.ch, Continuous_Mode, FSR_2048, COLLECT_NUM, ads1015_data);
    if(ret != HAL_OK) {
        printf("[error]%s collect power error,ret = %d\r\n", dev->name, ret);
        return false;
    }
    //打印
    printf("power.ch = %d,ref glbs:", config->power.ch);
    for(uint8_t i = 0; i < COLLECT_NUM; i++) {
        printf("%d ", ads1015_data[i].value);
    }
    printf("\r\n");
    //滤波
    if(glbs_filter(ads1015_data, COLLECT_NUM, &ref_voltage) == false) {
        printf("[error]%s glbs_filter data error\r\n", dev->name);
        return false;
    }
    //判断为门磁
    if(ref_voltage >= REF_V) {
        config->raw = 32767;
        printf("ref_voltage > %dmv is mcs\r\n", REF_V);
        return true;
    }
    //R10 1.8K
    //由于FSR配置为2.048V,LSB为1mV,所以需要* 1
    ref_current = ref_voltage / 1800 * 1;
    //清零
    memset(ads1015_data, 0, sizeof(ads1015_data));
    //采集温度数据
    ret = ads1015_extend_collect(config->collect_ch, Continuous_Mode, config->FSR, COLLECT_NUM, ads1015_data);
    if(ret != HAL_OK) {
        printf("[error]%s collect data error,ret = %d\r\n", dev->name, ret);
        return false;
    }
    //打印
    printf("FSR = %d, voltage glbs:", config->FSR);
    for(uint8_t i = 0; i < COLLECT_NUM; i++) {
        printf("%d ", ads1015_data[i].value);
    }
    printf("\r\n");
    //滤波
    if(glbs_filter(ads1015_data, COLLECT_NUM, &voltage) == false) {
        printf("[error]%s glbs_filter data error\r\n", dev->name);
        return false;
    }
    if(voltage >= 2000 && config->FSR == FSR_0256) {
        //FSR=256时,计算的电阻值大于250,可以判断为需要切换FSR;
        //FSR=512时,电阻值不能大于500
        config->FSR = FSR_0512;
        ret = ads1015_extend_collect(config->collect_ch, Continuous_Mode, config->FSR, COLLECT_NUM, ads1015_data);
        if(ret != HAL_OK) {
            printf("[error]%s collect data error,ret = %d\r\n", dev->name, ret);
            return false;
        }
        //打印
        printf("FSR = %d, voltage glbs:", config->FSR);
        for(uint8_t i = 0; i < COLLECT_NUM; i++) {
            printf("%d ", ads1015_data[i].value);
        }
        printf("\r\n");
        //滤波
        if(glbs_filter(ads1015_data, COLLECT_NUM, &voltage) == false) {
            printf("[error]%s glbs_filter data error\r\n", dev->name);
            return false;
        }
    }
    if (config->FSR == FSR_0256) {
        //由于FSR配置为256,LSB为0.125mV,所以需要* 0.125
        voltage *= 0.125;
    }else if (config->FSR == FSR_0512) {
        //由于FSR配置为256,LSB为0.25mV,所以需要* 0.25
        voltage *= 0.25;
    }

    resistance = voltage / ref_current;
    config->raw = pt100_calculation(resistance);
    printf("ref_current = %smA\r\n", ftoc(ref_current, 2));
    printf("voltage     = %smV\r\n", ftoc(voltage, 2));
    printf("resistance  = %sΩ\r\n",  ftoc(resistance, 2));
    printf("raw:%s\r\n", ftoc(config->raw, 3));
    return true;
}
/**
 * @brief  传感器数据控制
 * @note   
 * @param  dev: 传感器设备
 * @param  cmd: 控制命令
 * @param  *data: 数据指针
 * @param  *arg: 数据参数
 * @retval true:成功 false:失败
 */
static bool pt100_control(sensor_device_t dev, sensor_cmd_e cmd, void *data, void *arg)
{
    FIND_CFG(pt100_cfg_t, dev);
    if(data == NULL) {
        return false;
    }
    switch(cmd) {
        case SENSOR_CMD_STATUS_SET:
        {
            config->status = *(data_status_e  *)data;
            break;
        }
        case SENSOR_CMD_STATUS_GET:
        {
            *(data_status_e  *)data = config->status;
            break;
        }
        case SENSOR_CMD_DATA_SET:
        {
            uint8_t id = *(uint8_t *)arg;
            if(id == SENSOR_DATA_GET_RAW) {
                config->raw = *(PT100_DATA_T *)data;
            } else{
                config->value = *(PT100_DATA_T *)data;
            }
            break;
        }
        case SENSOR_CMD_DATA_GET:
        {
            uint8_t id = *(uint8_t *)arg;
            if(id == SENSOR_DATA_GET_RAW) {
                *(PT100_DATA_T *)data = config->raw;
            } else{
                *(PT100_DATA_T *)data = config->value;
            }
            break;
        }
        default:
            break;
    }
    return true;
}
