/**
 * +@File Name: DS18B20.c
 * @brief  
 * ! 8MHz测量,1us延时实际为7.61us;采样500us高电平,无延时实际采集到点数为105
 * @Author : 
 * @Version : 1.0
 * @Creat Date : 2023-10-09
 * 
 * @copyright Copyright (c) 2023 
 * @par 修改日志:
 * Date           Version     Author  Description
 * 2023-10-09     v1.0        huagnly 内容
 */
/* Includes ------------------------------------------------------------------*/
#include "ds18b20.h"
/* Private includes ----------------------------------------------------------*/
#include "node_crc.h"
#include "critical_platform.h"
/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
//ROM 指令
#define DS18B20_CMD_SEARCH_ROM      0XF0    //搜索ROM
#define DS18B20_CMD_READ_ROM	    0X33    //读ROM
#define DS18B20_CMD_MATCH_ROM       0X55    //匹配ROM
#define DS18B20_CMD_SKIP_ROM	    0XCC    //跳过ROM
#define DS18B20_CMD_ALARM_SEARCH    0XEC    //报警搜索
//功能命令
#define DS18B20_CMD_CONVERT_T       0X44    //温度转换
#define DS18B20_CMD_WRITE_SCRPAD    0X4E    //写入暂存器
#define DS18B20_CMD_READ_SCRPAD     0XBE    //读取暂存器
#define DS18B20_CMD_COPY_SCRPAD     0X48    //复制暂存器
#define DS18B20_CMD_RECALL_E2       0XB8    //EEPROM数据回收
#define DS18B20_CMD_READ_POWER      0XB4    //读取电源
/* Private macro -------------------------------------------------------------*/
#define DS18B20_DELAY_US(x)   for(volatile uint32_t i = 0; i < x; i++);//DelayUs（1） 为2.6us
#define DS18B20_DELAY_MS(ms)  HAL_Delay(ms)
//DS18B20 函数宏定义
#define DS18B20_DQ_0     dq->GPIOx->BRR = (uint32_t)dq->GPIO_Pin;
#define DS18B20_DQ_1     dq->GPIOx->BSRR = (uint32_t)dq->GPIO_Pin;

#define DS18B20_DQ_IN()  HAL_GPIO_ReadPin(dq->GPIOx, dq->GPIO_Pin)
//(dq->GPIOx->IDR & dq->GPIO_Pin)
/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private user code ---------------------------------------------------------*/
/*
 * 函数名：DS18B20_GPIO_Config
 * 描述  ：配置DS18B20用到的I/O口
 * 输入  ：无
 * 输出  ：无
 */
void DS18B20_GPIO_Config(ds18b20_dq_t *dq)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = dq->GPIO_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(dq->GPIOx, &GPIO_InitStruct);
}
/*
 * 函数名：DS18B20_Mode_IPU
 * 描述  ：使DS18B20-DATA引脚变为输入模式,注意配置为了输入上拉
 * 输入  ：无
 * 输出  ：无
 */
static void DS18B20_Mode_IPU(ds18b20_dq_t *dq)
{
    // GpioInit(&dq->obj, dq->pin, PIN_INPUT, PIN_PUSH_PULL, PIN_PULL_UP, 0);
    /* Activate the Pull-up or Pull down resistor for the current IO */
    // uint32_t temp = dq->GPIOx->PUPDR;
    // temp &= ~(GPIO_PUPDR_PUPD0 << (dq->position * 2U));
    // temp |= ((GPIO_PULLUP) << (dq->position * 2U));
    // dq->GPIOx->PUPDR = temp;

    MODIFY_REG(dq->GPIOx->MODER, ((dq->GPIO_Pin * dq->GPIO_Pin) * GPIO_MODER_MODE0), ((dq->GPIO_Pin * dq->GPIO_Pin) * GPIO_MODE_INPUT));
}
/*
 * 函数名：DS18B20_Mode_Out_PP
 * 描述  ：使DS18B20-DATA引脚变为输出模式
 * 输入  ：无
 * 输出  ：无
 */
static void DS18B20_Mode_Out_PP(ds18b20_dq_t *dq)
{
    // GpioInit(&dq->obj, dq->pin, PIN_OUTPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0);
    /* Activate the Pull-up or Pull down resistor for the current IO */
    // uint32_t temp = dq->GPIOx->PUPDR;
    // temp &= ~(GPIO_PUPDR_PUPD0 << (dq->position * 2U));
    // temp |= ((GPIO_NOPULL) << (dq->position * 2U));
    // dq->GPIOx->PUPDR = temp;

    MODIFY_REG(dq->GPIOx->MODER, ((dq->GPIO_Pin * dq->GPIO_Pin) * GPIO_MODER_MODE0), ((dq->GPIO_Pin * dq->GPIO_Pin) * GPIO_MODE_OUTPUT_PP));
    dq->GPIOx->BRR = (uint32_t)dq->GPIO_Pin;//写入低电平
}

/**
 * @brief  主机给从机发送复位脉冲;初始化序列开始
 * @note   存在阻塞延时780us
 *         在初始化序列期间，总线控制器拉低总线并保持 480us 以发出（TX）一个复位脉冲信号， 然后释放总线，进入接收状态（RX）。
 *         初始化序列如图 17 所示
 * @param  dq: 传感器引脚
 * @retval None
 */
static void DS18B20_Rst(ds18b20_dq_t *dq)
{
    DS18B20_Mode_Out_PP(dq);
    DS18B20_DQ_0;
    /*  主机至少产生480us的低电平复位信号  复位脉冲*/
    DS18B20_DELAY_US(750);

    /* 主机在产生复位信号后，需将总线拉高 */
    DS18B20_DQ_1;

    /*从机接收到主机的复位信号后，会在15~60us后给主机发一个存在脉冲*/
    DS18B20_DELAY_US(30);
}
/**
 * @brief  检测从机给主机返回的存在脉冲
 * @note   存在阻塞延时最多340us;
 * @param  dq: 传感器引脚
 * @retval 0:成功; -1:没有存在脉冲; -2:存在脉冲超时
 */
static int8_t DS18B20_Presence(ds18b20_dq_t *dq)
{
    uint8_t pulse_time = 0;

    /*  主机设置为输入
        当总线被释放后，5kΩ的上拉电阻将总线拉到高电平。
        当 DS18B20 检测到 IO 引脚上的上升沿后，等待 15-60us，然后发出一个由 60-240us 低电平信号构成的存在脉冲。
    */
    DS18B20_Mode_IPU(dq);

    /* 等待存在脉冲的到来，存在脉冲为一个60~240us的低电平信号 
        * 如果存在脉冲没有来则做超时处理，从机接收到主机的复位信号后，会在15~60us后给主机发一个存在脉冲
        */
    //等待从机拉低电平
    while(DS18B20_DQ_IN() && pulse_time < 10) {
        pulse_time++;
        DS18B20_DELAY_US(10);
    }
    /* 经过100us后，存在脉冲都还没有到来*/
    if( pulse_time >= 10) {
        return -1;//失败
    } else{
        pulse_time = 0;
    }

    /* 存在脉冲到来，且存在的时间不能超过240us */
    while( !DS18B20_DQ_IN() && pulse_time < 24 ) {
        pulse_time++;
        DS18B20_DELAY_US(10);
    }	
    if( pulse_time >= 24) {
        return -2;//失败
    } else {
        return 0;
    }
}
/**
  * @brief  DS18B20 初始化函数
  * @note:  存在阻塞延时1240us
  * @param  dq: 传感器引脚
  * @retval 0:成功; -1:没有存在脉冲; -2:存在脉冲超时
*/
int8_t DS18B20_Init(ds18b20_dq_t *dq)
{
    int8_t ret = 0;
    NODE_CRITICAL_SECTION_BEGIN();
    DS18B20_Mode_Out_PP(dq);
    DS18B20_DQ_1;//强制拉高,防止拉低无效
    DS18B20_Rst(dq);
    ret = DS18B20_Presence(dq);
    NODE_CRITICAL_SECTION_END();

    return ret;
}
/**
 * @brief  从DS18B20读取一个bit
 * @note   存在阻塞延时60us
 *         所有读时序必须最少 60us，包括两个读周期间至少 1us 的恢复时间。
 * @param  *dq: 传感器引脚
 * @retval 返回读取到的数据
 */
static uint8_t DS18B20_ReadBit(ds18b20_dq_t *dq)
{
    uint8_t dat;

    /* 读0和读1的时间至少要大于60us */	
    DS18B20_Mode_Out_PP(dq);
    /* 读时间的起始：必须由主机产生 >1us <15us 的低电平信号 */
    DS18B20_DELAY_US(1);
    /* 设置成输入，释放总线，由上拉电阻将总线拉高 */
    DS18B20_Mode_IPU(dq);
    DS18B20_DELAY_US(10);
    if( DS18B20_DQ_IN() == GPIO_PIN_SET )
        dat = 1;
    else
        dat = 0;
    /* 这个延时参数请参考时序图 */
    DS18B20_DELAY_US(50);

    return dat;
}
/**
 * @brief  从DS18B20读一个字节(8位)，低位先行
 * @note   存在阻塞延时480us 
 * @param  *dq: 传感器引脚
 * @retval 返回读取到的数据
 */
static uint8_t DS18B20_ReadByte(ds18b20_dq_t *dq)
{
    uint8_t j, dat = 0;

    for(uint8_t i = 0; i < 8; i++) {
        j = DS18B20_ReadBit(dq);
        dat = (dat) | (j << i);
    }

    return dat;
}
/**
 * @brief  写一个字节到DS18B20，低位先行
 * @note   存在阻塞延时63us
 * @param  *dq: 传感器引脚
 * @param  dat: 要写入的数据
 * @retval None
 */
static void DS18B20_WriteByte(ds18b20_dq_t *dq, uint8_t dat)
{
    uint8_t testb;
    DS18B20_Mode_Out_PP(dq);

    for(uint8_t i = 0; i < 8; i++) {

        testb = dat & 0x01;
        dat = dat >> 1;
        /* 写0和写1的时间至少要大于60us */
        if (testb) {
            //总线控制器要写产生一个写时序，必须把数据线拉到低电平然后释放，且需在 15us 内释放总线。
            DS18B20_DQ_0;
            /* 1us < 这个延时 < 15us */
            DS18B20_DELAY_US(3);

            DS18B20_DQ_1;
            //总线控制器初始化写时序后，DS18B20 在一个 15us 到 60us 的窗口内对信号线进行采用。如果线上是高电平，就是写 1。反之，如果线上是低电平，就是写 0
            DS18B20_DELAY_US(60);
        } else {
            DS18B20_DQ_0;
            /* 60us < Tx 0 < 120us */
            //总线控制器要生成写 0 时序，必须把数据线拉到低电平且继续保持至少 60us。
            DS18B20_DELAY_US(61);

            DS18B20_DQ_1;
            /* 1us < Trec(恢复时间) < 无穷大*/
            DS18B20_DELAY_US(2);
        }
    }
}
/**
 * @brief  跳过匹配 DS18B20 ROM
 * @note    存在阻塞延时1183us
 * @param   dq: 传感器引脚
 * @retval 无
 */
static bool DS18B20_SkipRom(ds18b20_dq_t *dq)
{
    DS18B20_Rst(dq);
    DS18B20_DELAY_US(400);
    DS18B20_WriteByte(dq, DS18B20_CMD_SKIP_ROM);
    return true;
}
/**
 * @brief 计算温度值
 * 
 * 根据传感器返回的高字节和低字节数据计算温度值。
 * 
 * @param tmh 高字节数据
 * @param tml 低字节数据
 * @return 计算得到的温度值
 */
float caculate_temp(uint8_t tmh, uint8_t tml)
{
    uint8_t th = 0;
    uint8_t tl = 0;
    float temp = 0;

    tl = tml & 0x0F;//取低字节后四位

    th = (tmh << 4) + (tml >> 4);//取高字节后三位和低字节前四位

    temp = (int)th;//整数部分

    if (tmh > 0x08) {
        th = ~th + 1;//取反加一
        temp = -th;//负数
    }

    temp += tl * 0.0625;//小数部分
    return temp;
}
/**
 * @brief  在跳过匹配 ROM 情况下获取 DS18B20 温湿度度值
 * @note   存在阻塞延时7562us
 * @param  dq: 传感器引脚
 * @param  temperature: 温度值
 * @retval 读取是否成功 1：成功;0：失败
 */
bool DS18B20_GetTemp_SkipRom(ds18b20_dq_t *dq, float *temperature)
{
    uint8_t tpmsb = 0, tplsb = 0, crc = 0;
    uint8_t reg[9] = {0};
    uint8_t crc_data = 0;

{
    NODE_CRITICAL_SECTION_BEGIN();
    DS18B20_SkipRom(dq);
    DS18B20_WriteByte(dq, DS18B20_CMD_CONVERT_T); /* 开始转换 */
    NODE_CRITICAL_SECTION_END();
}
    //DQ信号至少保持500ms高电平，以确保转换完成
    DS18B20_DELAY_MS(750);
{
    NODE_CRITICAL_SECTION_BEGIN();
    DS18B20_SkipRom(dq);
    DS18B20_WriteByte(dq, DS18B20_CMD_READ_SCRPAD); /* 读温度值 */
    for(uint8_t i = 0; i < 9; i++) {
        reg[i] = DS18B20_ReadByte(dq);
    }
    NODE_CRITICAL_SECTION_END();
}
    tplsb   = reg[0];
    tpmsb   = reg[1];
    crc     = reg[8];

    crc_data = crc8_maxim(reg, sizeof(reg) - 1);
    if(crc_data != crc) {
        return false;
    } else {
        *temperature = caculate_temp(tpmsb, tplsb);
        return true;
    }
}