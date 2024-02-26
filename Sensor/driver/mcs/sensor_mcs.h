/**
 * @File Name: sensor_mcs.h
 * @brief
 * @Author : huangly@milesight.com
 * @Version : 1.0
 * @Creat Date : 2023-07-06
 *
 * @copyright Copyright (c) 2023 星纵物联科技有限公司
 * @par 修改日志:
 * Date           Version     Author  Description
 * 2023-07-06     v1.0        huagnly 内容
 */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SENSOR_MCE_H__
#define __SENSOR_MCE_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "sensor_driver.h"
#include "gpio_sys.h"
#include "stm32wlxx_hal.h"
/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/
#define MCS_DEBUG 1
/* Exported types ------------------------------------------------------------*/
/**
 * @brief  门磁传感器状态
 * @note   None
 */
typedef enum
{
    MCS_STATUS_CLOSE,   //门磁吸合
    MCS_STATUS_OPEN,    //门磁释放
    MCS_STATUS_NONE,
    MCS_STATUS_INIT,
    MCS_STATUS_COLLECT,
}mcs_status_t;
/**
 * @brief  门磁设备配置信息
 * @note   None
 * @retval None
 */
typedef struct
{
    mcs_status_t        status;         //状态
    GPIO_PinState       isr_level;      //中断电平
    GPIO_PinState       current_level;  //当前电平
    uint16_t            filter;         //滤波时间
    void    (*isr_callback)(void);      //中断回调函数

    struct{
        Gpio_t          obj;            //输出控制
        PinNames        pin;            //输出引脚
        GPIO_PinState   on;             //开启电平
    }output;
    struct{
        Gpio_t          obj;            //输入控制
        PinNames        pin;            //输入引脚
        GPIO_PinState   level;          //触发电平
    }input;
}mcs_cfg_t;
/**
 * @brief  门磁设备对象
 * @note   None
 * @retval None
 */
typedef struct
{
    struct sensor_device parent;    //父类实例
    mcs_cfg_t   *cfg;               //配置信息
}mcs_device_t;
/* Exported variables ---------------------------------------------------------*/
extern mcs_device_t mcs;
/* Exported functions prototypes ---------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* __SENSOR_MCE_H__ */
