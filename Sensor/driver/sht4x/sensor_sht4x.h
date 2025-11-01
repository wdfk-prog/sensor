/**
 * @File Name: sensor_sht4x.h
 * @brief  
 * @Author : 
 * @Version : 1.0
 * @Creat Date : 2024-01-25
 * 
 * @copyright Copyright (c) 2024 
 * @par 修改日志:
 * Date           Version     Author  Description
 * 2024-01-25     v1.0        huagnly 内容
*/
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SENSOR_SHT4X_H__
#define __SENSOR_SHT4X_H__

#ifdef __cplusplus
extern "C" {
#endif
/* Includes ------------------------------------------------------------------*/
#include "sensor_driver.h"
#include "sht4x_i2c.h"
#include "stm32wlxx.h"
/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/
#define SHT4X_DATA_T float
/* Exported types ------------------------------------------------------------*/
/**
 * @brief  SHT4X数据类型
 * @note   None
 * @retval None
 */
typedef enum
{
    SHT4X_DATA_TEMPERATURE = 0,
    SHT4X_DATA_HUMIDITY,
    SHT4X_DATA_MAX,
}sht4x_data_e;
/**
 * @brief  SHT4X设备配置信息
 * @note   None
 * @retval None
 */
typedef struct
{
    data_status_e status[SHT4X_DATA_MAX];   //传感器状态
    SHT4X_DATA_T value[SHT4X_DATA_MAX];
    SHT4X_DATA_T raw[SHT4X_DATA_MAX];       //原始数据
    void (*i2c_init)(void);
    sht4x_handle_t      handle;
}sht4x_driver_cfg_t;
/**
 * @brief  SHT4X设备对象
 * @note   None
 * @retval None
 */
typedef struct
{
    struct sensor_device    parent; //父类实例
    sht4x_driver_cfg_t      *cfg;   //配置信息
}sht4x_device_t;
/* Exported variables ---------------------------------------------------------*/
extern sht4x_device_t sht4x;
/* Exported functions prototypes ---------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif //__SENSOR_SHT4X_H__