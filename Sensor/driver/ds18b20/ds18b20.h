/**
 * @File Name: ds18b20.h
 * @brief  
 * @Author : huangly@milesight.com
 * @Version : 1.0
 * @Creat Date : 2024-01-22
 * 
 * @copyright Copyright (c) 2024 星纵物联科技有限公司
 * @par 修改日志:
 * Date           Version     Author  Description
 * 2024-01-22     v1.0        huagnly 内容
*/
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __DS18B20_H__
#define __DS18B20_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "usart.h"
#include "node_crc.h"
/* Exported constants --------------------------------------------------------*/
typedef enum
{
    DS18B20_ERR_OK = 0,     //无错误
    DS18B20_ERR_NO_DEV1,    //无设备1
    DS18B20_ERR_NO_DEV2,    //无设备2
    DS18B20_ERR_CRC,        //CRC校验错误
    DS18B20_ERR_TIMEOUT,    //超时
    DS18B20_ERR_WRITE,      //写入错误
    DS18B20_ERR_READ,       //读取错误
}ds18b20_err_t;
/* Exported macro ------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
typedef struct 
{
    UART_HandleTypeDef  *huart;     //串口句柄
    USART_TypeDef       *Instance;  //串口实例
}ds18b20_t;
/* Exported variables ---------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
ds18b20_err_t ds18b20_reset(ds18b20_t *dev);
ds18b20_err_t ds18b20_get_temp_skiprom(ds18b20_t *dev, float *temperature);

#ifdef __cplusplus
}
#endif

#endif /* __DS18B20_H__ */