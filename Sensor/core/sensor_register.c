/**
 * @File Name: sensor_register.c
 * @brief  
 * @Author : 
 * @Version : 1.0
 * @Creat Date : 2024-02-21
 * 
 * @copyright Copyright (c) 2024 
 * @par 修改日志:
 * Date           Version     Author  Description
 * 2024-02-21     v1.0        huagnly 内容
*/
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
#include "sensor_driver.h"
#include "NodeSDKConfig.h"
/* Private includes ----------------------------------------------------------*/
#include "sensor_pt100.h"
#include "sensor_mcs.h"
/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private user code ---------------------------------------------------------*/
/**
 * @brief  传感器注册函数
 * @note   None
 */
__weak void sensor_register(void)
{
   sensor_register_fun(&pt100[0].parent);
   sensor_register_fun(&pt100[1].parent);
   sensor_register_fun(&mcs.parent);
}
