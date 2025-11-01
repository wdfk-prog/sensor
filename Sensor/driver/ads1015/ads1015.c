#include "ads1015.h"
#include "main.h"
#include "i2c.h"
#include "module_ntag.h"

#define I2C_ADDR          0x48
#define I2C_READ_ADDR     0x91
#define I2C_WRITE_ADDR    0x90

static I2c_t *s_i2c_handle = NULL;
static ads1015_mode_t s_cur_mode = 0;

/**
 * @brief  加锁
 * @note   None
 * @retval None
 */
static int ads1015_lock(void)
{
    return ntag_lock();
}
/**
 * @brief  解锁
 * @note   None
 * @retval None
 */
static int ads1015_unlock(void)
{
    return ntag_unlock();
}

static uint8_t write_register(ads1015_reg_addr_t reg, uint16_t value)
{
	uint8_t data[3] = {0};
	
	data[0] = reg;
	data[1] = (value >> 8) & 0xff;
	data[2] = value & 0xff;
	ads1015_lock();
	uint8_t ret =  I2cTransmit(s_i2c_handle, I2C_ADDR, data, sizeof(data));
	ads1015_unlock();
	return ret;
}

static uint8_t read_register(ads1015_reg_addr_t reg, uint16_t *value)
{
	uint8_t data[2] = {0};
	uint8_t ret;

	ads1015_lock();
	ret = I2cTransmit(s_i2c_handle, I2C_ADDR, (uint8_t *)&reg, 1);
	if(ret) {
		ads1015_unlock();
		return ret;
	}
	
	ret = I2cReceive(s_i2c_handle, I2C_ADDR, data, sizeof(data));
	ads1015_unlock();
	if(ret) {
		return ret;
	}
	
	*value = (data[0] << 8) | data[1];
	
	return ret;
}

static uint8_t ads1015_conversions_trigger(void)
{
	ConfigReg_t reg;
	uint8_t ret;
	
	ret = read_register(Reg_Config, &reg.value);
	if(ret != 0) {
		return ret;
	}
	
	reg.Bits.Os = 1;
	
	ret = write_register(Reg_Config, reg.value);	
	
	return ret;
}

#if 0
static void ads1015_set_mode(ads1015_mode_t mode)
{
	ConfigReg_t reg;
	
	read_register(Reg_Config, &reg.value);
	reg.Bits.Mode = mode;
	
	write_register(Reg_Config, reg.value);
}

static void ads1015_set_fsr(ads1015_fsr_t fsr)
{
	ConfigReg_t reg;
	
	read_register(Reg_Config, &reg.value);
	reg.Bits.Pga = fsr;
	
	write_register(Reg_Config, reg.value);
}

static void ads1015_set_mux(ads1015_mux_t mux)
{
	ConfigReg_t reg;
	
	read_register(Reg_Config, &reg.value);
	reg.Bits.Mux = mux;
	
	write_register(Reg_Config, reg.value);	
}

static void ads1015_set_dr(ads1015_dr_t dr)
{
	ConfigReg_t reg;
	
	read_register(Reg_Config, &reg.value);
	reg.Bits.Dr = dr;
	
	write_register(Reg_Config, reg.value);	
}
#endif

void ads1015_init(I2c_t *obj)
{
	s_i2c_handle = obj;
}

uint8_t ads1015_config(ads1015_mux_t num, ads1015_mode_t mode, ads1015_fsr_t fsr, ads1015_dr_t dr)
{
	ConfigReg_t reg;
	uint8_t ret;
	
	ret = read_register(Reg_Config, &reg.value);
	if(ret != 0) {
		return ret;
	}

	reg.Bits.Mux  = num;
	reg.Bits.Mode = mode;
	s_cur_mode    = mode;
	reg.Bits.Pga  = fsr;
	reg.Bits.Dr   = dr;
	
	ret = write_register(Reg_Config, reg.value);
	
	return ret;
}

uint8_t ads1015_collect(uint8_t samples, ads1015_data_t *data)
{
	uint8_t count = 0;
	uint8_t ret = 0;

	for(count = 0; count < samples; count++) {
		if(s_cur_mode == SingleShot_Mode) {
			HAL_Delay(10);
			ret = ads1015_conversions_trigger();
			if(ret != 0) {
				data[count].succ = false;
				continue;
			}
		}

		HAL_Delay(10);

		ret = read_register(Reg_Conversion, &data[count].value);
		if(ret == 0) {
			data[count].value = (data[count].value >> 4);
			data[count].succ  = true;
		} else {
			data[count].succ  = false;
		}
	}

	return 0;
}

uint8_t ads1015_extend_collect(ads1015_mux_t num, ads1015_mode_t mode, ads1015_fsr_t fsr, uint8_t samples, ads1015_data_t *data)
{
	uint8_t count = 0;
	uint8_t ret;
	
	ret = ads1015_config(num, mode, fsr, SPS_128);
	if(ret != 0) {
		return ret;
	}
	
	for(count = 0; count < samples; count++) {
		if(mode == SingleShot_Mode) {
			HAL_Delay(10);
			ret = ads1015_conversions_trigger();
			if(ret != 0) {
				data[count].succ = false;
				continue;
			}
		}
		
		HAL_Delay(10);			
		if(count == 0 && Continuous_Mode == mode) {
			ret = read_register(Reg_Conversion, &data[count].value);
			HAL_Delay(10);
			ret = read_register(Reg_Conversion, &data[count].value);
		} else {
			ret = read_register(Reg_Conversion, &data[count].value);
		}

		if(ret == 0) {
			data[count].value = (data[count].value >> 4);
			data[count].succ  = true;
		} else {
			data[count].succ  = false;
		}
	}
	
	return 0;
}

void ads1015_test(void)
{
	ConfigReg_t reg;

	read_register(Reg_Config, &reg.value);

	printf("===ads1015 read default begin===\r\n");
	printf("===default config reg : 0x%04x\r\n", reg.value);
	printf("===comp_que = %d, comp_lat = %d, comp_pol = %d, comp_mode = %d\r\n", reg.Bits.CompQue, reg.Bits.CompLat, \
	reg.Bits.CompPol, reg.Bits.CompMode);
	printf("===dr = %d, mode = %d, pga = %d, mux = %d, os = %d\r\n", reg.Bits.Dr, reg.Bits.Mode, reg.Bits.Pga, \
	reg.Bits.Mux, reg.Bits.Os);
	printf("===ads1015 read defaut end===\r\n\r\n");

	printf("===ads1015 write test begin===\r\n");
	reg.Bits.CompQue = 2;
	reg.Bits.Pga     = 3;
	reg.Bits.Mux     = 4;
	write_register(Reg_Config, reg.value);
	printf("===set config reg : 0x%04x\r\n", reg.value);
	reg.value = 0;

	read_register(Reg_Config, &reg.value);
	printf("===current config reg : 0x%04x\r\n", reg.value);
	printf("===comp_que = %d, comp_lat = %d, comp_pol = %d, comp_mode = %d\r\n", reg.Bits.CompQue, reg.Bits.CompLat, \
	reg.Bits.CompPol, reg.Bits.CompMode);
	printf("===dr = %d, mode = %d, pga = %d, mux = %d, os = %d\r\n", reg.Bits.Dr, reg.Bits.Mode, reg.Bits.Pga, \
	reg.Bits.Mux, reg.Bits.Os);
	printf("===ads1015 write test end===\r\n\r\n");
}
