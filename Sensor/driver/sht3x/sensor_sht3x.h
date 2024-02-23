/**
 * @File Name: sensor_sht3x.h
 * @brief  
 * @Author : huangly@milesight.com
 * @Version : 1.0
 * @Creat Date : 2024-01-25
 * 
 * @copyright Copyright (c) 2024 星纵物联科技有限公司
 * @par 修改日志:
 * Date           Version     Author  Description
 * 2024-01-25     v1.0        huagnly 内容
*/
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SENSOR_SHT3X_H__
#define __SENSOR_SHT3X_H__

#ifdef __cplusplus
extern "C" {
#endif
/* Includes ------------------------------------------------------------------*/
#include "sensor_driver.h"
#include "sht3x.h"
/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/
#define SHT3X_DATA_T float
/* Exported types ------------------------------------------------------------*/
/**
 * @brief  SHT3X传感器数量定义
 */
enum
{
    SHT3X_0,
    SHT3X_1,
    SHT3X_MAX_NUM,
};
/**
 * @brief  SHT3X数据类型
 * @note   None
 * @retval None
 */
typedef enum
{
    SHT3X_DATA_TEMPERATURE = 0,
    SHT3X_DATA_HUMIDITY,
    SHT3X_DATA_MAX,
}sht3x_data_e;
/**
 * @brief  SHT3X设备配置信息
 * @note   None
 * @retval None
 */
typedef struct
{
    data_status_e status[SHT3X_DATA_MAX];   //传感器状态
    SHT3X_DATA_T value[SHT3X_DATA_MAX];
    SHT3X_DATA_T raw[SHT3X_DATA_MAX];       //原始数据
    sht3x_handle_t  device;
    void (*i2c_init)(void);
}sht3x_driver_cfg_t;
/**
 * @brief  SHT3X设备对象
 * @note   None
 * @retval None
 */
typedef struct
{
    struct sensor_device    parent; //父类实例
    sht3x_driver_cfg_t      *cfg;   //配置信息
}sht3x_device_t;
/* Exported variables ---------------------------------------------------------*/
extern sht3x_device_t sht3x[SHT3X_MAX_NUM];
/* Exported functions prototypes ---------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif //__SENSOR_SHT3X_H__