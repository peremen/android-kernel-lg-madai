/* Copyright (c) 2013, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include "msm_sensor.h"
#include <mach/board_lge.h>		//to use lge_get_board_revno()
#define IMX179_SENSOR_NAME "imx179"

#define CONFIG_IMX179_DEBUG
#undef CDBG
#ifdef CONFIG_IMX179_DEBUG
#define CDBG(fmt, args...) pr_err(fmt, ##args)
#else
#define CDBG(fmt, args...) do { } while (0)
#endif

DEFINE_MSM_MUTEX(imx179_mut);

static struct msm_sensor_ctrl_t imx179_s_ctrl;

#if defined(CONFIG_MACH_MSM8226_G2MDS_OPEN_CIS) || defined(CONFIG_MACH_MSM8226_G2MDS_GLOBAL_COM) || defined(CONFIG_MACH_MSM8226_G2MSS_GLOBAL_COM)
static struct msm_sensor_power_setting imx179_power_setting[] = {
	 /* Set GPIO_RESET to low to disable power on reset*/
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_LOW,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_VREG,
		.seq_val = CAM_VDIG,
		.config_val = 0,
		.delay = 0,
	},
	{								//VANA, GPIO 62
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_VANA,
		.config_val = GPIO_OUT_HIGH,
		.delay = 1,
	},
	{								//VIO, GPIO 113
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_VIO,
		.config_val = GPIO_OUT_HIGH,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_STANDBY,
		.config_val = GPIO_OUT_HIGH,
		.delay = 0,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_HIGH,
		.delay = 30,
	},
	{
		.seq_type = SENSOR_CLK,
		.seq_val = SENSOR_CAM_MCLK,
		.config_val = 0,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_I2C_MUX,
		.seq_val = 0,
		.config_val = 0,
		.delay = 0,
	},
};
static struct msm_sensor_power_setting imx179_power_setting_rev_a[] = {
	 /* Set GPIO_RESET to low to disable power on reset*/
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_LOW,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_VREG,
		.seq_val = CAM_VDIG,
		.config_val = 0,
		.delay = 0,
	},
	{								//VANA, GPIO 62
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_VANA,
		.config_val = GPIO_OUT_HIGH,
		.delay = 1,
	},
	{								//VIO, GPIO 113
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_VIO,
		.config_val = GPIO_OUT_HIGH,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_STANDBY,
		.config_val = GPIO_OUT_HIGH,
		.delay = 0,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_HIGH,
		.delay = 30,
	},
	{
		.seq_type = SENSOR_CLK,
		.seq_val = SENSOR_CAM_MCLK,
		.config_val = 0,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_I2C_MUX,
		.seq_val = 0,
		.config_val = 0,
		.delay = 0,
	},
};

static struct msm_sensor_power_setting imx179_power_setting_rev_b[] = {
	 /* Set GPIO_RESET to low to disable power on reset*/
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_LOW,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_VREG,
		.seq_val = CAM_VDIG,
		.config_val = 0,
		.delay = 0,
	},
	{								//VANA, GPIO 62
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_VANA,
		.config_val = GPIO_OUT_HIGH,
		.delay = 1,
	},
	{								//VIO, GPIO 113
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_VIO,
		.config_val = GPIO_OUT_HIGH,
		.delay = 2,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_STANDBY,
		.config_val = GPIO_OUT_HIGH,
		.delay = 0,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_HIGH,
		.delay = 30,
	},
	{
		.seq_type = SENSOR_CLK,
		.seq_val = SENSOR_CAM_MCLK,
		.config_val = 0,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_I2C_MUX,
		.seq_val = 0,
		.config_val = 0,
		.delay = 0,
	},
};
static struct msm_sensor_power_setting imx179_power_setting_rev_c[] = {
	 /* Set GPIO_RESET to low to disable power on reset*/
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_LOW,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_VREG,
		.seq_val = CAM_VDIG,
		.config_val = 0,
		.delay = 0,
	},
	/*CAM_VIO is added for Rev.C*/
	{     
		.seq_type = SENSOR_VREG,
		.seq_val = CAM_VIO,
		.config_val = 0,
		.delay = 0,
	},
	{								//VANA, GPIO 62
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_VANA,
		.config_val = GPIO_OUT_HIGH,
		.delay = 1,
	},
	{								//VIO, GPIO 113
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_VIO,
		.config_val = GPIO_OUT_HIGH,
		.delay = 2,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_STANDBY,
		.config_val = GPIO_OUT_HIGH,
		.delay = 0,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_HIGH,
		.delay = 30,
	},
	{
		.seq_type = SENSOR_CLK,
		.seq_val = SENSOR_CAM_MCLK,
		.config_val = 0,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_I2C_MUX,
		.seq_val = 0,
		.config_val = 0,
		.delay = 0,
	},
};
static struct msm_sensor_power_setting imx179_power_setting_rev_d[] = {
	 /* Set GPIO_RESET to low to disable power on reset*/
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_LOW,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_OIS_RESET,
		.config_val = GPIO_OUT_LOW,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_VREG,
		.seq_val = CAM_VDIG,
		.config_val = 0,
		.delay = 0,
	},
	{								//VANA, GPIO 62
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_VANA,
		.config_val = GPIO_OUT_HIGH,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_OIS_LDO_EN,
		.config_val = GPIO_OUT_HIGH,
		.delay = 1,
	},
	{								//VIO, GPIO 113
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_VIO,
		.config_val = GPIO_OUT_HIGH,
		.delay = 2,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_STANDBY,
		.config_val = GPIO_OUT_HIGH,
		.delay = 0,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_HIGH,
		.delay = 30,
	},
	{
		.seq_type = SENSOR_CLK,
		.seq_val = SENSOR_CAM_MCLK,
		.config_val = 0,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_OIS_RESET,
		.config_val = GPIO_OUT_HIGH,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_I2C_MUX,
		.seq_val = 0,
		.config_val = 0,
		.delay = 0,
	},
};
#elif defined(CONFIG_MACH_MSM8926_MADAI)
static struct msm_sensor_power_setting imx179_power_setting_madai[] = {
	 /* Set GPIO_RESET to low to disable power on reset*/
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_LOW,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_OIS_RESET,
		.config_val = GPIO_OUT_LOW,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_VREG,
		.seq_val = CAM_VDIG,
		.config_val = 0,
		.delay = 0,
	},
	{								//VANA, GPIO 62
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_VANA,
		.config_val = GPIO_OUT_HIGH,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_OIS_LDO_EN,
		.config_val = GPIO_OUT_HIGH,
		.delay = 1,
	},
	{								//VIO, GPIO 113
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_VIO,
		.config_val = GPIO_OUT_HIGH,
		.delay = 1,
	},	
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_STANDBY,
		.config_val = GPIO_OUT_HIGH,
		.delay = 0,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_HIGH,
		.delay = 30,
	},
	{
		.seq_type = SENSOR_CLK,	//MCLK order is changed. 20140103. younjung.park
		.seq_val = SENSOR_CAM_MCLK,
		.config_val = 0,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_OIS_RESET,
		.config_val = GPIO_OUT_HIGH,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_I2C_MUX,
		.seq_val = 0,
		.config_val = 0,
		.delay = 0,
	},
};

#else
static struct msm_sensor_power_setting imx179_power_setting[] = {
	 /* Set GPIO_RESET to low to disable power on reset*/
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_LOW,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_VREG,
		.seq_val = CAM_VDIG,
		.config_val = 0,
		.delay = 0,
	},
	{								//VANA, GPIO 62
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_VANA,
		.config_val = GPIO_OUT_HIGH,
		.delay = 1,
	},
	{								//VIO, GPIO 113
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_VIO,
		.config_val = GPIO_OUT_HIGH,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_STANDBY,
		.config_val = GPIO_OUT_HIGH,
		.delay = 0,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_HIGH,
		.delay = 30,
	},
	{
		.seq_type = SENSOR_CLK,
		.seq_val = SENSOR_CAM_MCLK,
		.config_val = 0,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_I2C_MUX,
		.seq_val = 0,
		.config_val = 0,
		.delay = 0,
	},
};

static struct msm_sensor_power_setting imx179_power_setting_rev_a[] = {
	 /* Set GPIO_RESET to low to disable power on reset*/
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_LOW,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_VREG,
		.seq_val = CAM_VDIG,
		.config_val = 0,
		.delay = 0,
	},
	{								//VANA, GPIO 62
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_VANA,
		.config_val = GPIO_OUT_HIGH,
		.delay = 1,
	},
	{								//VIO, GPIO 113
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_VIO,
		.config_val = GPIO_OUT_HIGH,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_STANDBY,
		.config_val = GPIO_OUT_HIGH,
		.delay = 0,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_HIGH,
		.delay = 30,
	},
	{
		.seq_type = SENSOR_CLK,
		.seq_val = SENSOR_CAM_MCLK,
		.config_val = 0,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_I2C_MUX,
		.seq_val = 0,
		.config_val = 0,
		.delay = 0,
	},
};

static struct msm_sensor_power_setting imx179_power_setting_rev_b[] = {
	 /* Set GPIO_RESET to low to disable power on reset*/
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_LOW,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_VREG,
		.seq_val = CAM_VDIG,
		.config_val = 0,
		.delay = 0,
	},
	/*CAM_VAF is added for Rev.B*/
	{
		.seq_type = SENSOR_VREG,        //added VAF on rev.b
		.seq_val = CAM_VAF,
		.config_val = 0,
		.delay = 0,
	},
	{								//VANA, GPIO 62
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_VANA,
		.config_val = GPIO_OUT_HIGH,
		.delay = 1,
	},
	{								//VIO, GPIO 113
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_VIO,
		.config_val = GPIO_OUT_HIGH,
		.delay = 2,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_STANDBY,
		.config_val = GPIO_OUT_HIGH,
		.delay = 0,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_HIGH,
		.delay = 30,
	},
	{
		.seq_type = SENSOR_CLK,
		.seq_val = SENSOR_CAM_MCLK,
		.config_val = 0,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_I2C_MUX,
		.seq_val = 0,
		.config_val = 0,
		.delay = 0,
	},
};
static struct msm_sensor_power_setting imx179_power_setting_rev_c[] = {
	 /* Set GPIO_RESET to low to disable power on reset*/
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_LOW,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_VREG,
		.seq_val = CAM_VDIG,
		.config_val = 0,
		.delay = 0,
	},
	/*CAM_VAF is added for Rev.B*/
	{
		.seq_type = SENSOR_VREG,        //added VAF on rev.b
		.seq_val = CAM_VAF,
		.config_val = 0,
		.delay = 0,
	},
	{								//VANA, GPIO 62
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_VANA,
		.config_val = GPIO_OUT_HIGH,
		.delay = 1,
	},
	{								//VIO, GPIO 113
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_VIO,
		.config_val = GPIO_OUT_HIGH,
		.delay = 2,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_STANDBY,
		.config_val = GPIO_OUT_HIGH,
		.delay = 0,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_HIGH,
		.delay = 30,
	},
	{
		.seq_type = SENSOR_CLK,
		.seq_val = SENSOR_CAM_MCLK,
		.config_val = 0,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_I2C_MUX,
		.seq_val = 0,
		.config_val = 0,
		.delay = 0,
	},
};
static struct msm_sensor_power_setting imx179_power_setting_rev_d[] = {
	 /* Set GPIO_RESET to low to disable power on reset*/
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_LOW,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_VREG,
		.seq_val = CAM_VDIG,
		.config_val = 0,
		.delay = 0,
	},
	/*CAM_VAF is added for Rev.B*/
	{
		.seq_type = SENSOR_VREG,        //added VAF on rev.b
		.seq_val = CAM_VAF,
		.config_val = 0,
		.delay = 0,
	},
	{								//VANA, GPIO 62
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_VANA,
		.config_val = GPIO_OUT_HIGH,
		.delay = 1,
	},
	{								//VIO, GPIO 113
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_VIO,
		.config_val = GPIO_OUT_HIGH,
		.delay = 2,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_STANDBY,
		.config_val = GPIO_OUT_HIGH,
		.delay = 0,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_HIGH,
		.delay = 30,
	},
	{
		.seq_type = SENSOR_CLK,
		.seq_val = SENSOR_CAM_MCLK,
		.config_val = 0,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_I2C_MUX,
		.seq_val = 0,
		.config_val = 0,
		.delay = 0,
	},
};


#endif

static struct v4l2_subdev_info imx179_subdev_info[] = {
	{
		.code = V4L2_MBUS_FMT_SBGGR10_1X10,
		.colorspace = V4L2_COLORSPACE_JPEG,
		.fmt = 1,
		.order = 0,
	},
};

static const struct i2c_device_id imx179_i2c_id[] = {
	{IMX179_SENSOR_NAME, (kernel_ulong_t)&imx179_s_ctrl},
	{ }
};

static int32_t msm_imx179_i2c_probe(struct i2c_client *client,
       const struct i2c_device_id *id)
{
       return msm_sensor_i2c_probe(client, id, &imx179_s_ctrl);
}

static struct i2c_driver imx179_i2c_driver = {
	.id_table = imx179_i2c_id,
	.probe  = msm_imx179_i2c_probe,
	.driver = {
		.name = IMX179_SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client imx179_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

static const struct of_device_id imx179_dt_match[] = {
	{.compatible = "qcom,imx179", .data = &imx179_s_ctrl},
	{}
};

MODULE_DEVICE_TABLE(of, imx179_dt_match);

static struct platform_driver imx179_platform_driver = {
	.driver = {
		.name = "qcom,imx179",
		.owner = THIS_MODULE,
		.of_match_table = imx179_dt_match,
	},
};

static int32_t imx179_platform_probe(struct platform_device *pdev)
{
	int32_t rc = 0;
	const struct of_device_id *match;

	match = of_match_device(imx179_dt_match, &pdev->dev);

	if(!match)
		return -EINVAL;
		
	rc = msm_sensor_platform_probe(pdev, match->data);
	return rc;
}

static int __init imx179_init_module(void)
{
	int32_t rc = 0;
	hw_rev_type rev_type = 0;

	pr_info("%s:%d\n", __func__, __LINE__);

      rev_type = lge_get_board_revno();

#if defined(CONFIG_MACH_MSM8926_MADAI)
	switch(rev_type) {
		case HW_REV_0:
			pr_err("%s: Sensor power is set as Rev.0\n", __func__);
			imx179_s_ctrl.power_setting_array.power_setting = imx179_power_setting_madai;
			imx179_s_ctrl.power_setting_array.size = ARRAY_SIZE(imx179_power_setting_madai);		
			break;
		default:
			pr_err("%s: Sensor power is set as Default mode and Rev. %d\n", __func__,rev_type);
			imx179_s_ctrl.power_setting_array.power_setting = imx179_power_setting_madai;
			imx179_s_ctrl.power_setting_array.size = ARRAY_SIZE(imx179_power_setting_madai);
			break;
	}

#else//                         
	switch(rev_type) {
		case HW_REV_0:
			pr_err("%s: Sensor power is set as Rev.0\n", __func__);
			imx179_s_ctrl.power_setting_array.power_setting = imx179_power_setting;
			imx179_s_ctrl.power_setting_array.size = ARRAY_SIZE(imx179_power_setting);
			break;
		case HW_REV_A:
			pr_err("%s: Sensor power is set as Rev.A\n", __func__);
			imx179_s_ctrl.power_setting_array.power_setting = imx179_power_setting_rev_a;
			imx179_s_ctrl.power_setting_array.size = ARRAY_SIZE(imx179_power_setting_rev_a);
			break;
		
		case HW_REV_B:
		       pr_err("%s: Sensor power is set as Rev.B\n", __func__);
			imx179_s_ctrl.power_setting_array.power_setting = imx179_power_setting_rev_b;
			imx179_s_ctrl.power_setting_array.size = ARRAY_SIZE(imx179_power_setting_rev_b);
			break;
		case HW_REV_C:
		       pr_err("%s: Sensor power is set as Rev.C\n", __func__);
			imx179_s_ctrl.power_setting_array.power_setting = imx179_power_setting_rev_c;
			imx179_s_ctrl.power_setting_array.size = ARRAY_SIZE(imx179_power_setting_rev_c);
			break;
		case HW_REV_D:
		       pr_err("%s: Sensor power is set as Rev.D\n", __func__);
			imx179_s_ctrl.power_setting_array.power_setting = imx179_power_setting_rev_d;
			imx179_s_ctrl.power_setting_array.size = ARRAY_SIZE(imx179_power_setting_rev_d);
			break;
		default:
			pr_err("%s: Sensor power is set as Default mode and Rev. %d\n", __func__,rev_type);
			imx179_s_ctrl.power_setting_array.power_setting = imx179_power_setting_rev_d;
			imx179_s_ctrl.power_setting_array.size = ARRAY_SIZE(imx179_power_setting_rev_d);
			break;
	}
#endif
/*                                                                                                            */

	rc = platform_driver_probe(&imx179_platform_driver,
		imx179_platform_probe);
	if (!rc)
		return rc;
	pr_err("%s:%d rc %d\n", __func__, __LINE__, rc);
	return i2c_add_driver(&imx179_i2c_driver);
}

static void __exit imx179_exit_module(void)
{
	pr_info("%s:%d\n", __func__, __LINE__);

	if (imx179_s_ctrl.pdev) {
		msm_sensor_free_sensor_data(&imx179_s_ctrl);
		platform_driver_unregister(&imx179_platform_driver);
	} else {
		i2c_del_driver(&imx179_i2c_driver);
	}
	return;
}

static struct msm_sensor_ctrl_t imx179_s_ctrl = {
	.sensor_i2c_client = &imx179_sensor_i2c_client,
	//.power_setting_array.power_setting = imx179_power_setting,
	//.power_setting_array.size = ARRAY_SIZE(imx179_power_setting),
	.msm_sensor_mutex = &imx179_mut,
	.sensor_v4l2_subdev_info = imx179_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(imx179_subdev_info),
};

module_init(imx179_init_module);
module_exit(imx179_exit_module);
MODULE_DESCRIPTION("imx179");
MODULE_LICENSE("GPL v2");
