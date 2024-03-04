/**
 * @File Name: sensor_pt100.h
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
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SENSOR_PT100_H__
#define __SENSOR_PT100_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "sensor_driver.h"
#include "i2c_sys.h"
#include "ads1015.h"
#include "stm32wlxx_hal.h"
/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/
#define PT100_DATA_T float
/* Exported types ------------------------------------------------------------*/
/**
 * @brief  PT100传感器
 * @note   数量定义
 * @retval None
 */
typedef enum
{
    PT100_0,
    PT100_1,
    PT100_MAX_NUM,
}pt100_e;
/**
 * @brief  PT100设备配置信息
 * @note   None
 * @retval None
 */
typedef struct
{
    data_status_e status;       //传感器状态
    PT100_DATA_T value;         //数据值
    PT100_DATA_T raw;           //原始数据
    struct{
        I2c_t      *obj;        //I2C对象
        I2cId_t     Id;         //I2C编号
        PinNames    scl;        //SCL引脚
        PinNames    sda;        //SDA引脚
    }i2c;
    ads1015_mux_t   collect_ch; //采集通道
    struct{
        GPIO_TypeDef    *port;  //控制端口
        uint32_t        pin;    //控制引脚
        GPIO_PinState   on;     //开启电平
        ads1015_mux_t   ch;     //电源通道
    }power;
    uint8_t             FSR;    //满量程范围
}pt100_cfg_t;
/**
 * @brief  PT100设备对象
 * @note   None
 * @retval None
 */
typedef struct
{
    struct sensor_device parent;    //父类实例
    pt100_cfg_t     *cfg;           //配置信息
}pt100_device_t;
/* Exported variables ---------------------------------------------------------*/
extern pt100_device_t pt100[PT100_MAX_NUM];
/* Exported functions prototypes ---------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* BOARD_SYSTEM_H */
