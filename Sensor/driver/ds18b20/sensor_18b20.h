/**
 * @File Name: sensor_18b20.h
 * @brief  GXHT3W温度传感器应用函数接口
 * @Author : huangly@milesight.com
 * @Version : 1.0
 * @Creat Date : 2023-10-09
 * 
 * @copyright Copyright (c) 2023 星纵物联科技有限公司
 * @par 修改日志:
 * Date           Version     Author  Description
 * 2023-10-09     v1.0        huagnly 内容
 */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SENSOR_18B20_H
#define	__SENSOR_18B20_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "sensor_driver.h"
#include "gpio_sys.h"
#include "ds18b20.h"
/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/
#define DS18B20_DATA_T float
/* Exported types ------------------------------------------------------------*/
/**
 * @brief  GXHT3W设备配置信息
 * @note   None
 * @retval None
 */
typedef struct
{
    struct
    {
        GPIO_TypeDef    *port;  //控制端口
        uint32_t        pin;    //控制引脚
        GPIO_PinState   level;  //控制电平
    }power;
    data_status_e status;       //传感器状态
    DS18B20_DATA_T value;       //数据值
    DS18B20_DATA_T raw;         //原始数据
    ds18b20_t dq;               //信号引脚
}ds18b20_driver_cfg_t;
/**
 * @brief  GXHT3W设备对象
 * @note   None
 * @retval None
 */
typedef struct
{
    struct sensor_device parent;    //父类实例
    ds18b20_driver_cfg_t   *cfg;    //配置信息
}ds18b20_device_t;
/* Exported variables ---------------------------------------------------------*/
extern ds18b20_device_t ds18b20;
/* Exported functions prototypes ---------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* __SENSOR_18B20_H */