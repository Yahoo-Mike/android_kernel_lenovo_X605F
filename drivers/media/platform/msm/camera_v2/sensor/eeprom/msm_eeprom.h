/* Copyright (c) 2011-2015, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef MSM_EEPROM_H
#define MSM_EEPROM_H

#include <linux/i2c.h>
#include <linux/gpio.h>
#include <soc/qcom/camera2.h>
#include <media/v4l2-subdev.h>
#include <media/msmb_camera.h>
#include "msm_camera_i2c.h"
#include "msm_camera_spi.h"
#include "msm_camera_io_util.h"
#include "msm_camera_dt_util.h"

struct msm_eeprom_ctrl_t;

#define DEFINE_MSM_MUTEX(mutexname) \
	static struct mutex mutexname = __MUTEX_INITIALIZER(mutexname)

#define PROPERTY_MAXSIZE 32

struct msm_eeprom_ctrl_t {
	struct platform_device *pdev;
	struct mutex *eeprom_mutex;

	struct v4l2_subdev sdev;
	struct v4l2_subdev_ops *eeprom_v4l2_subdev_ops;
	enum msm_camera_device_type_t eeprom_device_type;
	struct msm_sd_subdev msm_sd;
	enum cci_i2c_master_t cci_master;
	enum i2c_freq_mode_t i2c_freq_mode;

	struct msm_camera_i2c_client i2c_client;
	struct msm_eeprom_board_info *eboard_info;
	uint32_t subdev_id;
	int32_t userspace_probe;
	struct msm_eeprom_memory_block_t cal_data;
	uint8_t is_supported;
};

//##***wangzhancai@wind-mobi.com  --20180316 start ***
struct msm_camera_i2c_reg_array hi556_readotp_init_regval[] = {
    {0x0e00, 0x0102}, //tg_pmem_sckpw/sdly
    {0x0e02, 0x0102}, //tg_dmem_sckpw/sdly
    {0x0e0c, 0x0100}, //tg_pmem_rom_dly

    {0x27fe, 0xe000}, // firmware start address-ROM
    {0x0b0e, 0x8600}, // BGR enable
    {0x0d04, 0x0100}, // STRB(OTP Busy) output enable
    {0x0d02, 0x0707}, // STRB(OTP Busy) output drivability
    {0x0f30, 0x6e25}, // Analog PLL setting
    {0x0f32, 0x7067}, // Analog CLKGEN setting
    {0x0f02, 0x0106}, // PLL enable
    {0x0a04, 0x0000}, // mipi disable
    {0x0e0a, 0x0001}, // TG PMEM CEN anable
    {0x004a, 0x0100}, // TG MCU enable
    {0x003e, 0x1000}, // ROM OTP Continuous W/R mode enable
    {0x0a00, 0x0100}, // Stream On
};
struct msm_camera_i2c_reg_setting hi556_otp_read_init_setting = {
	.reg_setting = hi556_readotp_init_regval,
	.size = ARRAY_SIZE(hi556_readotp_init_regval),
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
	.data_type = MSM_CAMERA_I2C_WORD_DATA,
	.delay = 2,
};
struct msm_camera_i2c_reg_array hi556_readotp_init_regval_start[] = {
	{0x0a02, 0x01, 0x0000},
	{0x0a00, 0x00, 0x000a},
	{0x0f02, 0x00, 0x0000},
	{0x011a, 0x01, 0x0000},
	{0x011b, 0x09, 0x0000},
	{0x0d04, 0x01, 0x0000},
	{0x0d00, 0x07, 0x0000},
	{0x003e, 0x10, 0x0000},
	{0x070f, 0x05, 0x0000},
	{0x0a00, 0x01, 0x0000},
	{0x10a, 0x04, 0},//high
	{0x10b, 0x01, 0},//low
	{0x102, 0x01, 0},
};
struct msm_camera_i2c_reg_setting hi556_otp_read_init_setting_start = {
	.reg_setting = hi556_readotp_init_regval_start,
	.size = ARRAY_SIZE(hi556_readotp_init_regval_start),
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
	.data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.delay = 2,
};
struct msm_camera_i2c_reg_array hi556_readotp_init_regval_end[] = {
	{0x0a00, 0x00, 0x000a},
	{0x003f, 0x00, 0x0000},
	{0x0a00, 0x01, 0x0000},
};
struct msm_camera_i2c_reg_setting hi556_otp_read_init_setting_end = {
	.reg_setting = hi556_readotp_init_regval_end,
	.size = ARRAY_SIZE(hi556_readotp_init_regval_end),
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
	.data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.delay = 2,
};
//##***wangzhancai@wind-mobi.com  --20180316 end ***
#endif
