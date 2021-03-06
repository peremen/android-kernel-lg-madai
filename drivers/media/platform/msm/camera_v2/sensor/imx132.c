/* Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
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
#define IMX132_SENSOR_NAME "imx132"
DEFINE_MSM_MUTEX(imx132_mut);

//                                      
#define CONFIG_IMX132_DEBUG
//                                      

#undef CDBG
#ifdef CONFIG_IMX132_DEBUG
#define CDBG(fmt, args...) pr_err(fmt, ##args)
#else
#define CDBG(fmt, args...) do { } while (0)
#endif

static struct msm_sensor_ctrl_t imx132_s_ctrl;

static struct msm_sensor_power_setting imx132_power_setting[] = {
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
		.delay = 1,
	},
	{								//VIO, GPIO 113
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_VIO,
		.config_val = GPIO_OUT_HIGH,
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
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_HIGH,
		.delay = 10,
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
static struct msm_sensor_power_setting imx132_power_setting_revA[] = {
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
		.delay = 1,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_VIO,
		.config_val = GPIO_OUT_HIGH,
		.delay = 0,
	},
	{
		.seq_type = SENSOR_VREG,
		.seq_val = CAM_VANA,
		.config_val = 0,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_HIGH,
		.delay = 10,
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

static struct v4l2_subdev_info imx132_subdev_info[] = {
	{
		.code		= V4L2_MBUS_FMT_SBGGR10_1X10,
		.colorspace	= V4L2_COLORSPACE_JPEG,
		.fmt		= 1,
		.order		= 0,
	},
};

static const struct i2c_device_id imx132_i2c_id[] = {
	{IMX132_SENSOR_NAME, (kernel_ulong_t)&imx132_s_ctrl},
	{ }
};

static int32_t msm_imx132_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	return msm_sensor_i2c_probe(client, id, &imx132_s_ctrl);
}
static struct i2c_driver imx132_i2c_driver = {
	.id_table = imx132_i2c_id,
	.probe  = msm_imx132_i2c_probe,
	.driver = {
		.name = IMX132_SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client imx132_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

static const struct of_device_id imx132_dt_match[] = {
	{.compatible = "qcom,imx132", .data = &imx132_s_ctrl},
	{}
};

MODULE_DEVICE_TABLE(of, imx132_dt_match);

static struct platform_driver imx132_platform_driver = {
	.driver = {
		.name = "qcom,imx132",
		.owner = THIS_MODULE,
		.of_match_table = imx132_dt_match,
	},
};

static int32_t imx132_platform_probe(struct platform_device *pdev)
{
	int32_t rc = 0;
	const struct of_device_id *match;
	CDBG("%s E\n", __func__);
	match = of_match_device(imx132_dt_match, &pdev->dev);
	
/*                                                    */
	if(!match)
	{
		pr_err(" %s failed ",__func__);
	      return -ENODEV;
	}
/*                                                    */
	      
	rc = msm_sensor_platform_probe(pdev, match->data);
	CDBG("%s: X, rc = %d\n", __func__, rc);
	return rc;
}

static int __init imx132_init_module(void)
{
	int32_t rc = 0;
	hw_rev_type rev_type = 0;
	CDBG("%s E\n", __func__);
      rev_type = lge_get_board_revno();
	switch(rev_type) {
		case HW_REV_0:
			pr_err("%s: Sensor power is set as Rev.0\n", __func__);
			imx132_s_ctrl.power_setting_array.power_setting = imx132_power_setting;
			imx132_s_ctrl.power_setting_array.size = ARRAY_SIZE(imx132_power_setting);
			break;
		default:
			pr_err("%s: Sensor power is set as Default mode and Rev. %d\n", __func__,rev_type);
			imx132_s_ctrl.power_setting_array.power_setting = imx132_power_setting_revA;
			imx132_s_ctrl.power_setting_array.size = ARRAY_SIZE(imx132_power_setting_revA);
			break;
	}
	rc = platform_driver_probe(&imx132_platform_driver,
		imx132_platform_probe);
	if (!rc) {
		CDBG("%s: X, rc = %d\n", __func__, rc);
		return rc;
	}
	return i2c_add_driver(&imx132_i2c_driver);
}

static void __exit imx132_exit_module(void)
{
	if (imx132_s_ctrl.pdev) {
		msm_sensor_free_sensor_data(&imx132_s_ctrl);
		platform_driver_unregister(&imx132_platform_driver);
	} else
		i2c_del_driver(&imx132_i2c_driver);
	return;
}

static struct msm_sensor_ctrl_t imx132_s_ctrl = {
	.sensor_i2c_client = &imx132_sensor_i2c_client,
	//.power_setting_array.power_setting = imx132_power_setting,
	//.power_setting_array.size = ARRAY_SIZE(imx132_power_setting),
	.msm_sensor_mutex = &imx132_mut,
	.sensor_v4l2_subdev_info = imx132_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(imx132_subdev_info),
};

module_init(imx132_init_module);
module_exit(imx132_exit_module);
MODULE_DESCRIPTION("imx132");
MODULE_LICENSE("GPL v2");
