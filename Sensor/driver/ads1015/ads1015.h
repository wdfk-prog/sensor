#ifndef __ADS1015_H__
#define __ADS1015_H__
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "i2c_sys.h"

typedef enum ads1015_reg_addr_s {
	Reg_Conversion = 0,
	Reg_Config,
	Reg_LoThresh,
	Reg_HiThresh
} ads1015_reg_addr_t;

typedef enum ads1015_mode_s {
	Continuous_Mode = 0,
	SingleShot_Mode
} ads1015_mode_t;

typedef enum ads1015_fsr_s {
	FSR_6144 = 0,
	FSR_4096,
	FSR_2048,
	FSR_1024,
	FSR_0512,
	FSR_0256,
} ads1015_fsr_t;

typedef enum ads1015_mux_s {
	DIFF_0_1 = 0,
	DIFF_0_3,
	DIFF_1_3,
	DIFF_2_3,
	SINGLE_0,
	SINGLE_1,
	SINGLE_2,
	SINGLE_3,
} ads1015_mux_t;

typedef enum ads1015_dr_s {
	SPS_128 = 0,
	SPS_250,
	SPS_490,
	SPS_920,
	SPS_1600,
	SPS_2400,
	SPS_3300,
} ads1015_dr_t;

typedef union uConfigReg {
	uint16_t value;
	struct sBits
	{
		uint8_t CompQue         : 2;
		uint8_t CompLat         : 1;
		uint8_t CompPol         : 1;
		uint8_t CompMode        : 1;
		uint8_t Dr              : 3;
		uint8_t Mode            : 1;
		uint8_t Pga             : 3;
		uint8_t Mux             : 3;
		uint8_t Os              : 1;
	}Bits;	
} ConfigReg_t;

typedef struct ads1015_data_s {
	bool succ;
	uint16_t value;
} ads1015_data_t;

void ads1015_init(I2c_t *obj);
uint8_t ads1015_config(ads1015_mux_t num, ads1015_mode_t mode, ads1015_fsr_t fsr, ads1015_dr_t dr);
uint8_t ads1015_collect(uint8_t samples, ads1015_data_t *data);
uint8_t ads1015_extend_collect(ads1015_mux_t num, ads1015_mode_t mode, ads1015_fsr_t fsr, uint8_t samples, ads1015_data_t *data);
void ads1015_test(void);
#endif

