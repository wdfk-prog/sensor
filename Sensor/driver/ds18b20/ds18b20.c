/**
 * @File Name: ds18b20.c
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
/* Includes ------------------------------------------------------------------*/
#include "ds18b20.h"
/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define WAIT_TIMEOUT_COUNT          1500    //接收等待超时计数 [测试调试值]
//ROM 指令
#define DS18B20_CMD_SEARCH_ROM      0XF0    //搜索ROM
#define DS18B20_CMD_READ_ROM        0X33    //读ROM
#define DS18B20_CMD_MATCH_ROM       0X55    //匹配ROM
#define DS18B20_CMD_SKIP_ROM        0XCC    //跳过ROM
#define DS18B20_CMD_ALARM_SEARCH    0XEC    //报警搜索
//功能命令
#define DS18B20_CMD_CONVERT_T       0X44    //温度转换
#define DS18B20_CMD_WRITE_SCRPAD    0X4E    //写入暂存器
#define DS18B20_CMD_READ_SCRPAD     0XBE    //读取暂存器
#define DS18B20_CMD_COPY_SCRPAD     0X48    //复制暂存器
#define DS18B20_CMD_RECALL_E2       0XB8    //EEPROM数据回收
#define DS18B20_CMD_READ_POWER      0XB4    //读取电源

#define WIRE_0                      0x00    //写入0
#define WIRE_1                      0xff    //写入1
/*  读脉冲中，串口发送数据0xff，起始位实现读脉冲时序的启动，LSB为1，
    此时，主机释放并采样总线，起始位与LSB共维持17.4 μs，满足协议对读时序的要求。
*/
#define OW_READ                     0xff    //读取
#define ONEWIRE_NOBODY              0xF0    //无设备
/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private user code ---------------------------------------------------------*/
/**
 * @brief 计算温度值
 * 
 * 根据传感器返回的高字节和低字节数据计算温度值。
 * 
 * @param tmh 高字节数据
 * @param tml 低字节数据
 * @return 计算得到的温度值
 */
static float caculate_temp(uint8_t tmh, uint8_t tml)
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
 * @brief 设置USART参数
 * 
 * @param baud 波特率
 */
static void usart_setup(ds18b20_t *dev,uint32_t baud)
{
    dev->huart->Instance = dev->Instance;
    dev->huart->Init.BaudRate = baud;
    dev->huart->Init.WordLength = UART_WORDLENGTH_8B;
    dev->huart->Init.StopBits = UART_STOPBITS_1;
    dev->huart->Init.Parity = UART_PARITY_NONE;
    dev->huart->Init.Mode = UART_MODE_TX_RX;
    dev->huart->Init.HwFlowCtl = UART_HWCONTROL_NONE;
    dev->huart->Init.OverSampling = UART_OVERSAMPLING_16;
    dev->huart->Init.ClockPrescaler = UART_PRESCALER_DIV1;

    if (HAL_HalfDuplex_Init(dev->huart) != HAL_OK) {
        Error_Handler();
    }
}
/**
 * @brief 将一个字节转换为位数组
 * 
 * @param ow_byte 要转换的字节
 * @param bits 存储位数组的指针
 */
static void byte_to_bits(uint8_t ow_byte, uint8_t *bits) 
{
    uint8_t i;
    for (i = 0; i < 8; i++) {
        if (ow_byte & 0x01) {
            *bits = WIRE_1;
        } else {
            *bits = WIRE_0;
        }
        bits++;
        ow_byte = ow_byte >> 1;
    }
}
/**
 * @brief  接收数据
 * 
 * @note   无阻塞,需外部延时调整
 * @return 接收到的数据内容
 */
static ds18b20_err_t one_wire_read(UART_HandleTypeDef *uart, uint8_t *byte)
{
    uint16_t pause = WAIT_TIMEOUT_COUNT;
    uint8_t rx = 0;
    for (uint8_t i = 0; i < 8; i++) {
        uart->Instance->TDR = OW_READ;  //发送一个字节
        //等待接收完成
        while(__HAL_UART_GET_FLAG(uart, UART_FLAG_RXNE) == false && pause--);
        if(pause == 0) {
            return DS18B20_ERR_TIMEOUT;
        }
        rx = uart->Instance->RDR;

        *byte >>= 1;                 //先接收的数据放在最低位
        if (rx == WIRE_1) {
            *byte |= 0x80;
        }
        pause = WAIT_TIMEOUT_COUNT;
    }
    return DS18B20_ERR_OK;
}
/**
 * @brief 发送一个字节的数据到单总线设备
 * 
 * @param byte 要发送的字节数据
 */
static ds18b20_err_t one_wire_send_byte(UART_HandleTypeDef *uart,uint8_t byte)
{
    uint16_t pause = WAIT_TIMEOUT_COUNT;
    uint8_t data[8] = {0};
    byte_to_bits(byte, data);
    for (uint8_t i = 0; i < 8; i++) {
        uart->Instance->TDR = data[i];
        //等待发送完成
        while(__HAL_UART_GET_FLAG(uart, UART_FLAG_TC) == false && pause--);
        if(pause == 0) {
            return DS18B20_ERR_TIMEOUT;
        }
        data[i] = uart->Instance->RDR;    //读取接收到的数据时清零RXNE标志
        pause = 1000;
    }
    return DS18B20_ERR_OK;
}
/**
 * @brief 复位
 * 
 * @return 1: 总线上存在设备 0: 总线上不存在设备
 */
ds18b20_err_t ds18b20_reset(ds18b20_t *dev)
{
    uint16_t pause = WAIT_TIMEOUT_COUNT;
    usart_setup(dev, 9600);
    /*  发送复位脉冲 
        设置波特率为9600b/s，每1bit的宽度为104.2 μs，
        发送数据0xf0，则起始位和低4 bit构成复位信号，高4 bit为1，即释放总线。
        如总线上无应答信号，则串口接收的数据等于0xf0，否则说明总线上有应答信号。
    */
    dev->Instance->TDR = ONEWIRE_NOBODY; //发送后TDR自动清零
    //等待接收完成UART_FLAG_RXNE
    while(__HAL_UART_GET_FLAG(dev->huart, UART_FLAG_RXNE) == false && pause--);
    //读取接收到的数据时清零RXNE标志
    uint8_t rx = dev->Instance->RDR;
    if(pause == 0) {
        return DS18B20_ERR_TIMEOUT;
    }
    /*  设置波特率为115 200 b/s，即每1 bit的宽度为8.7 μs，完整的发送过程持续87 μs，
        满足读写脉冲至少60 μs的协议要求。起始位、停止位分别用来启动、终止读写脉冲。
    */
    usart_setup(dev, 115200);
    if(rx != ONEWIRE_NOBODY) {
        return DS18B20_ERR_OK;
    } else {
        return DS18B20_ERR_NO_DEV1;
    }
}
/**
 * @brief 通过跳过ROM地址获取温度值
 * @param temperature 存储温度值的指针
 * @return 获取温度值成功返回true，否则返回false
 */
ds18b20_err_t ds18b20_get_temp_skiprom(ds18b20_t *dev, float *temperature)
{
    uint8_t reg[9] = {0};
    ds18b20_err_t ret = DS18B20_ERR_OK;

    if(ds18b20_reset(dev) != DS18B20_ERR_OK) {
        ret = DS18B20_ERR_NO_DEV1;
        goto exit;
    }
    if(one_wire_send_byte(dev->huart, DS18B20_CMD_SKIP_ROM) != DS18B20_ERR_OK) {
        ret = DS18B20_ERR_WRITE;
        goto exit;
    }
    //开始转换
    if(one_wire_send_byte(dev->huart, DS18B20_CMD_CONVERT_T) != DS18B20_ERR_OK) {
        ret = DS18B20_ERR_WRITE;
        goto exit;
    }
    //等待转换, 最大750ms
    HAL_Delay(800);
    //复位
    if(ds18b20_reset(dev) != DS18B20_ERR_OK) {
        ret = DS18B20_ERR_NO_DEV2;
        goto exit;
    }
    if(one_wire_send_byte(dev->huart, DS18B20_CMD_SKIP_ROM) != DS18B20_ERR_OK) {
        ret = DS18B20_ERR_WRITE;
        goto exit;
    }
    //读取温度
    if(one_wire_send_byte(dev->huart, DS18B20_CMD_READ_SCRPAD) != DS18B20_ERR_OK) {
        ret = DS18B20_ERR_WRITE;
        goto exit;
    }
    //读取9字节内存
    for (uint8_t i = 0; i < 9; i++) {
        if(one_wire_read(dev->huart, &reg[i]) != DS18B20_ERR_OK) {
            ret = DS18B20_ERR_READ;
            goto exit;
        }
    }

    uint8_t temp_l  = reg[0];
    uint8_t temp_h  = reg[1];
    uint8_t crc     = reg[8];
    uint8_t crc_data = crc8_maxim(reg, sizeof(reg) - 1);

    if(crc_data != crc) {
        ret = DS18B20_ERR_CRC;
    } else {
        *temperature = caculate_temp(temp_h, temp_l);
        ret = DS18B20_ERR_OK;
    }

exit:
    HAL_UART_DeInit(dev->huart);
    return ret;
}