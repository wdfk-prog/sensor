/**
 * @File Name: DS18B20.h
 * @brief  
 * @Author : 
 * @Version : 1.0
 * @Creat Date : 2023-10-09
 * 
 * @copyright Copyright (c) 2023 
 * @par 修改日志:
 * Date           Version     Author  Description
 * 2023-10-09     v1.0        huagnly 内容
 */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __DS18B20_H
#define	__DS18B20_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include <stm32wlxx.h>
/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
typedef struct{
    GPIO_TypeDef    *GPIOx;
    uint32_t        GPIO_Pin;
    uint8_t         position;   //引脚位置
}ds18b20_dq_t;
/* Exported variables ---------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
void DS18B20_GPIO_Config(ds18b20_dq_t *dq);
int8_t DS18B20_Init(ds18b20_dq_t *dq);
bool DS18B20_GetTemp_SkipRom(ds18b20_dq_t *dq, float *temperature);

#ifdef __cplusplus
}
#endif

#endif /* __DS18B20_H */