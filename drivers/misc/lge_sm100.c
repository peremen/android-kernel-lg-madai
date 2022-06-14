/*
 * android vibrator driver (msm7x27, Motor IC)
 *
 * Copyright (C) 2009 LGE, Inc.
 *
 * Author: Jinkyu Choi <jinkyu@lge.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <linux/platform_device.h>
#include <mach/gpio.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/stat.h>
#include "../staging/android/timed_output.h"
#include <linux/lge_sm100.h>

#include <mach/msm_iomap.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/regulator/machine.h>

/* gpio and clock control for vibrator */
#define REG_WRITEL(value, reg)		writel(value, reg)
#define REG_READL(reg)			readl(reg)

static void __iomem *virt_bases_v;

#define MMSS_CC_GP1_CMD_RCGR(x) (void __iomem *)(virt_bases_v + (x))

#define MMSS_CC_PWM_SET		0xFD8C3450
#define GCC_GP1_PWM_SET		0xFC401900
#define MMSS_CC_PWM_SIZE	SZ_1K
#define DEVICE_NAME		"lge_sm100"

#define MMSS_CC_N_DEFAULT	54	/* for 175Hz motor */
#define MMSS_CC_D_MAX		MMSS_CC_N_DEFAULT
#define MMSS_CC_D_HALF		(MMSS_CC_N_DEFAULT >> 1)
static DEFINE_MUTEX(vib_lock);

struct timed_vibrator_data {
	struct timed_output_dev dev;
	struct hrtimer timer;
	spinlock_t lock;
	int max_timeout;
	atomic_t ms_time; /* vibrator duration */
	atomic_t vib_status;			/* on/off */
	atomic_t vibe_gain;				/* default max gain */
	atomic_t vibe_pwm;
	atomic_t gp1_clk_flag;
	int amp;
	int vibe_n_value;
	int haptic_en_gpio;
	int motor_pwm_gpio;
	int vibe_warmup_delay; /* in ms */
	int vpwr_on;
	struct regulator *vreg_l16;
	struct work_struct work_vibrator_off;
	struct work_struct work_vibrator_on;
};

static struct clk *cam_gp1_clk;

static int vibrator_ic_enable_set(int enable, struct timed_vibrator_data *vib_data)
{
	int gpio;
	int rc = 0;

	INFO_MSG("[vib]ic_enable=%d\n", enable);

	if (enable)
	{
		rc=gpio_direction_output(vib_data->haptic_en_gpio, 1);
	    INFO_MSG("[vib]gpio_direction_output : %d\n ",rc);
	}
	else
		gpio_direction_output(vib_data->haptic_en_gpio, 0);

	gpio = gpio_get_value(vib_data->haptic_en_gpio);
	INFO_MSG("[vib]Haptic_EN_GPIO Value : %d\n ", gpio);

	return 0;
}

static int vibrator_power_set(int enable, struct timed_vibrator_data *vib_data)
{
	int rc;

	INFO_MSG("[vib]pwr_enable=%d\n", enable);

	mutex_lock(&vib_lock);
	if (enable) {
		rc = regulator_enable(vib_data->vreg_l16);
		if (rc < 0)
			pr_err("%s: regulator_enable failed\n", __func__);
	} else {
		if (regulator_is_enabled(vib_data->vreg_l16) > 0) {
			rc = regulator_disable(vib_data->vreg_l16);
			if (rc < 0)
				pr_err("%s: regulator_disable failed\n", __func__);
		}
	}
	mutex_unlock(&vib_lock);

	return 0;
}

static int vibrator_pwm_set(int enable, int amp, int n_value)
{
	/* TODO: set clk for amp */
	uint d_val;

	INFO_MSG("[vib]amp=%d, n_value=%d\n", amp, n_value);

	d_val = ((MMSS_CC_N_DEFAULT * amp) >> 7);

	if (virt_bases_v == NULL)
		virt_bases_v = ioremap(MMSS_CC_PWM_SET, MMSS_CC_PWM_SIZE);

	if (enable) {
		REG_WRITEL(
			((~(d_val << 1)) & 0xffU),	/* D[7:0] */
			MMSS_CC_GP1_CMD_RCGR(0x10));
		REG_WRITEL(
			(1 << 1U) +	/* ROOT_EN[1] */
			(1),		/* UPDATE[0] */
			MMSS_CC_GP1_CMD_RCGR(0));
	} else {
		REG_WRITEL(
			(0 << 1U) +	/* ROOT_EN[1] */
			(0),		/* UPDATE[0] */
			MMSS_CC_GP1_CMD_RCGR(0));
	}

	return 0;
}

#ifdef ANDROID_VIBRATOR_USE_WORKQUEUE
static inline void vibrator_work_on(struct work_struct *work)
{
	INFO_MSG("[vib]vibrator_work_on use workqueue");
	queue_work(vibrator_workqueue, work);
}

static inline void vibrator_work_off(struct work_struct *work)
{
	INFO_MSG("[vib]vibrator_work_off use workqueue");
	if (!work_pending(work))
		queue_work(vibrator_workqueue, work);
}
#else
static inline void vibrator_work_on(struct work_struct *work)
{
	INFO_MSG("[vib]vibrator_work_on use schework");
	schedule_work(work);
}

static inline void vibrator_work_off(struct work_struct *work)
{
	INFO_MSG("[vib]vibrator_work_off use schework");
	if (!work_pending(work))
		schedule_work(work);
}
#endif

static int android_vibrator_force_set(struct timed_vibrator_data *vib, int nForce, int n_value)
{
	/* Check the Force value with Max and Min force value */
	int vib_duration_ms = 0;

	INFO_MSG("[vib]nForce : %d\n", nForce);
#if 1
	if (nForce > 127)
		nForce = 127;
	if (nForce < 0)
		nForce = 0;
#endif

	if (vib->vibe_warmup_delay > 0) {
		if (atomic_read(&vib->vib_status))
			msleep(vib->vibe_warmup_delay);
	}

	/* TODO: control the gain of vibrator */
	if (nForce == 0) {
		INFO_MSG("[vib]call vibrator_ic_enable_set ,nForce : %d\n", nForce);
		vibrator_ic_enable_set(0, vib);
		INFO_MSG("[vib]call vibrator_pwm_set, nForce : %d\n", nForce);
		vibrator_pwm_set(0, 0, n_value);
		/* should be checked for vibrator response time */
		if (vib->vpwr_on != 1)
			vibrator_power_set(0, vib);

		atomic_set(&vib->vib_status, false);
	} else {
		if (work_pending(&vib->work_vibrator_off))
			cancel_work_sync(&vib->work_vibrator_off);
		hrtimer_cancel(&vib->timer);

		vib_duration_ms = atomic_read(&vib->ms_time);
		/* should be checked for vibrator response time */
		if (vib->vpwr_on != 1)
			vibrator_power_set(1, vib);

		INFO_MSG("[vib]call vibrator_pwm_set, nForce : %d\n", nForce);
		vibrator_pwm_set(1, nForce, n_value);
		INFO_MSG("[vib]call vibrator_ic_enable_set ,nForce : %d\n", nForce);
		vibrator_ic_enable_set(1, vib);

		atomic_set(&vib->vib_status, true);

		hrtimer_start(&vib->timer,
				ns_to_ktime((u64)vib_duration_ms * NSEC_PER_MSEC),
				HRTIMER_MODE_REL);
	}

	return 0;
}

static void android_vibrator_on(struct work_struct *work)
{
	struct timed_vibrator_data *vib =
		container_of(work, struct timed_vibrator_data,
				work_vibrator_on);
	int gain = atomic_read(&vib->vibe_gain);
	int pwm = atomic_read(&vib->vibe_pwm);
	/* suspend /resume logging test */
	INFO_MSG("[vib]call android_vibrator_on ,gain = %d , pwm = %d\n ", gain, pwm);
	pr_debug("%s: gain = %d pwm = %d\n", __func__,
			gain, pwm);

	android_vibrator_force_set(vib, gain, pwm);
}

static void android_vibrator_off(struct work_struct *work)
{
	struct timed_vibrator_data *vib =
		container_of(work, struct timed_vibrator_data,
				work_vibrator_off);

	INFO_MSG("[vib]call android_vibrator_off ");
	android_vibrator_force_set(vib, 0, vib->vibe_n_value);
}

static enum hrtimer_restart vibrator_timer_func(struct hrtimer *timer)
{
	struct timed_vibrator_data *vib =
		container_of(timer, struct timed_vibrator_data, timer);

	INFO_MSG("[vib]call vibrator_timer_func");
	vibrator_work_off(&vib->work_vibrator_off);
	return HRTIMER_NORESTART;
}

static int vibrator_get_time(struct timed_output_dev *dev)
{
	struct timed_vibrator_data *vib =
		container_of(dev, struct timed_vibrator_data, dev);

	INFO_MSG("[vib]call vibrator_get_time");
	if (hrtimer_active(&vib->timer)) {
		ktime_t r = hrtimer_get_remaining(&vib->timer);
		return ktime_to_ms(r);
	}
		return 0;
}

static void vibrator_enable(struct timed_output_dev *dev, int value)
{
	struct timed_vibrator_data *vib =
		container_of(dev, struct timed_vibrator_data, dev);
	unsigned long flags;

	INFO_MSG("[vib]call vibrator_enable");

	spin_lock_irqsave(&vib->lock, flags);
	if (value > 0) {
		INFO_MSG("[vib]call vibrator_enable value = %d \n", value);
		if (atomic_read(&vib->gp1_clk_flag) == 0) {
			clk_prepare_enable(cam_gp1_clk);
			atomic_set(&vib->gp1_clk_flag, 1);
		}

		if (value > vib->max_timeout)
			value = vib->max_timeout;

		atomic_set(&vib->ms_time, value);
		INFO_MSG("[vib] vibrator_enable call vibrator_work_on ");
		vibrator_work_on(&vib->work_vibrator_on);
	} else {
		INFO_MSG("[vib]call vibrator_enable value = %d \n", value);
		if (atomic_read(&vib->gp1_clk_flag) == 1) {
			clk_disable_unprepare(cam_gp1_clk);
			atomic_set(&vib->gp1_clk_flag, 0);
		}
		INFO_MSG("[vib] vibrator_enable call vibrator_work_off ");
		vibrator_work_off(&vib->work_vibrator_off);
	}
	spin_unlock_irqrestore(&vib->lock, flags);
}

static int vibrator_gpio_init(struct timed_vibrator_data *vib_data)
{
	int rc;

	INFO_MSG("[vib]***vibrator_init\n");

	/* GPIO setting for Motor EN in pmic8921 */
	rc = gpio_request(vib_data->haptic_en_gpio, "lin_motor_en");
	INFO_MSG("[vib]vibrator_gpio_init , rc = %d \n",rc);
	if (rc) {
		INFO_MSG("[vib]GPIO_LIN_MOTOR_EN %d request failed\n", vib_data->haptic_en_gpio);
		return 0;
	}

	/* gpio init */
	rc = gpio_request(vib_data->motor_pwm_gpio, "lin_motor_pwm");
	INFO_MSG("[vib]vibrator_gpio_init , rc = %d \n",rc);
	if (unlikely(rc < 0)) {
		INFO_MSG("[vib]not able to get gpio %d\n", vib_data->motor_pwm_gpio);
		return 0;
	}

	return 0;
}

#ifdef CONFIG_OF
static void vibrator_parse_dt(struct device *dev, struct timed_vibrator_data *vib_data)
{
	struct device_node *np = dev->of_node;

	of_property_read_u32(np, "syncoam,vpwr-on", &vib_data->vpwr_on);
	INFO_MSG("[sm100] vib->vpwr_on : %d!!\n", vib_data->vpwr_on);

	vib_data->haptic_en_gpio = of_get_named_gpio_flags(np, "syncoam,haptic-pwr-gpio", 0, NULL);
	vib_data->motor_pwm_gpio = of_get_named_gpio_flags(np, "syncoam,motor-pwm-gpio", 0, NULL);

	of_property_read_u32(np, "syncoam,motor-amp", &vib_data->amp);
	of_property_read_u32(np, "syncoam,n-value", &vib_data->vibe_n_value);

	INFO_MSG("[sm100] gpio en : %d, pwm : %d, amp : %d, n_value : %d\n",
		   vib_data->haptic_en_gpio, vib_data->motor_pwm_gpio,
		   vib_data->amp, vib_data->vibe_n_value);
}
#endif

static ssize_t vibrator_amp_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct timed_output_dev *dev_ = (struct timed_output_dev *)dev_get_drvdata(dev);
	struct timed_vibrator_data *vib = container_of(dev_, struct timed_vibrator_data, dev);

	int gain = atomic_read(&(vib->vibe_gain));
	INFO_MSG("[vib]vibrator_amp_show , gain = %d \n",gain);
	return sprintf(buf, "%d\n", gain);
}

static ssize_t vibrator_amp_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	struct timed_output_dev *dev_ = (struct timed_output_dev *)dev_get_drvdata(dev);
	struct timed_vibrator_data *vib = container_of(dev_, struct timed_vibrator_data, dev);

	int gain;
	sscanf(buf, "%d", &gain);
	INFO_MSG("[vib]vibrator_amp_store , gain = %d \n",gain);
#if 0
	if (gain > 128 || gain < -128) {
		INFO_MSG(KERN_ERR "%s invalid value: should be -128 ~ +128\n", __func__);
		return -EINVAL;
	}
#endif
	atomic_set(&vib->vibe_gain, gain);

	return size;
}

static ssize_t vibrator_pwm_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct timed_output_dev *dev_ = (struct timed_output_dev *)dev_get_drvdata(dev);
	struct timed_vibrator_data *vib = container_of(dev_, struct timed_vibrator_data, dev);

	int gain = atomic_read(&(vib->vibe_pwm));
	INFO_MSG("[vib]vibrator_pwm_show , gain = %d \n",gain);
	return sprintf(buf, "%d\n", gain);
}

static ssize_t vibrator_pwm_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	struct timed_output_dev *dev_ = (struct timed_output_dev *)dev_get_drvdata(dev);
	struct timed_vibrator_data *vib = container_of(dev_, struct timed_vibrator_data, dev);

	int gain;
	sscanf(buf, "%d", &gain);
	INFO_MSG("[vib]vibrator_pwm_store , gain = %d \n",gain);
	
	atomic_set(&vib->vibe_pwm, gain);

	return size;
}

static struct device_attribute sm100_device_attrs[] = {
	__ATTR(amp, S_IRUGO | S_IWUSR, vibrator_amp_show, vibrator_amp_store),
	__ATTR(n_val, S_IRUGO | S_IWUSR, vibrator_pwm_show, vibrator_pwm_store),
};

struct timed_vibrator_data android_vibrator_data = {
	.dev.name = "vibrator",
	.dev.enable = vibrator_enable,
	.dev.get_time = vibrator_get_time,
	.max_timeout = 30000, /* max time for vibrator enable 30 sec. */
};

static int android_vibrator_probe(struct platform_device *pdev)
{
	int i, ret = 0;
	struct timed_vibrator_data *vib;

	INFO_MSG("[vib]sm100_probe!\n");

	platform_set_drvdata(pdev, &android_vibrator_data);
	
	vib = (struct timed_vibrator_data *)platform_get_drvdata(pdev);

	
	if (pdev->dev.of_node) {
		INFO_MSG("[sm100] probe : pdev->dev.of_node\n");
		vibrator_parse_dt(&pdev->dev, vib);
	}
 
	if (vib->vpwr_on != 1) {
		if (!(vib->vreg_l16)) {
			vib->vreg_l16 = regulator_get(&pdev->dev, "vdd_ana");
			
			if (IS_ERR(vib->vreg_l16)) {
				pr_err("%s: regulator get of pm8941_l21 failed (%ld)\n",
						__func__, PTR_ERR(vib->vreg_l16));
				vib->vreg_l16 = NULL;
			}
		}
	}

	pdev->dev.init_name = vib->dev.name;
	INFO_MSG("[sm100] dev->init_name : %s, dev->kobj : %s\n",
				pdev->dev.init_name, pdev->dev.kobj.name);

	if (vibrator_gpio_init(vib) < 0) {
		INFO_MSG("Android Vreg, GPIO set failed\n");
		return -ENODEV;
	}

	vibrator_pwm_set(1, 117, 166);
	INFO_MSG("[vib] vibrator_pwm_set(1, 117, 166)\n");
	
	cam_gp1_clk = clk_get(&pdev->dev, "cam_gp1_clk");
	clk_set_rate(cam_gp1_clk, 22222);
	INFO_MSG("[vib] clk_set_rate(cam_gp1_clk, 22222)\n");
	
	atomic_set(&vib->vibe_gain, vib->amp); /* max value is 128 */
	atomic_set(&vib->vibe_pwm, vib->vibe_n_value);
	atomic_set(&vib->vib_status, false);
	atomic_set(&vib->gp1_clk_flag, 0);
	INFO_MSG("[vib] atomic_set done \n");
	
	INIT_WORK(&vib->work_vibrator_off, android_vibrator_off);
	INIT_WORK(&vib->work_vibrator_on, android_vibrator_on);
	hrtimer_init(&vib->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	vib->timer.function = vibrator_timer_func;
	spin_lock_init(&vib->lock);
	
	INFO_MSG("[vib] before timed_output_dev_register ,ret = %d \n",ret);
	ret = timed_output_dev_register(&vib->dev);
	INFO_MSG("[vib] after timed_output_dev_register ,ret = %d \n",ret);
	if (ret < 0) {
		INFO_MSG("[vib] after timed_output_dev_register1 ,ret <0 ");
		timed_output_dev_unregister(&vib->dev);
		return -ENODEV;
	}

	for (i = 0; i < ARRAY_SIZE(sm100_device_attrs); i++) {
		ret = device_create_file(vib->dev.dev, &sm100_device_attrs[i]);
		INFO_MSG("[vib] after device_create_file ,ret =%d ",ret);
		if (ret < 0) {
			INFO_MSG("[vib] after timed_output_dev_register2 ,ret <0 ");
			timed_output_dev_unregister(&vib->dev);
			device_remove_file(vib->dev.dev, &sm100_device_attrs[i]);
			return -ENODEV;
		}
	}

	INFO_MSG("[vib]Android Vibrator Initialization was done\n");

	return 0;
}

static int android_vibrator_remove(struct platform_device *pdev)
{
	struct timed_vibrator_data *vib =
		(struct timed_vibrator_data *)platform_get_drvdata(pdev);

	INFO_MSG("[vib]Android Vibrator Driver Shutdown\n");

	android_vibrator_force_set(vib, 0, vib->vibe_n_value);
	timed_output_dev_unregister(&vib->dev);

	return 0;
}

static int android_vibrator_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct timed_vibrator_data *vib;

	INFO_MSG("[vib]Android Vibrator Driver Suspend\n");

	vib = (struct timed_vibrator_data *)platform_get_drvdata(pdev);
	android_vibrator_force_set(vib, 0, vib->vibe_n_value);

/* it will be applied after calendar-noti test */
	if (atomic_read(&vib->gp1_clk_flag) == 1) {
		clk_disable_unprepare(cam_gp1_clk);
		atomic_set(&vib->gp1_clk_flag, 0);
	}

	gpio_tlmm_config(GPIO_CFG(vib->motor_pwm_gpio, 0, GPIO_CFG_OUTPUT,
		GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(vib->haptic_en_gpio, 0, GPIO_CFG_OUTPUT,
		GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);

	return 0;
}

static int android_vibrator_resume(struct platform_device *pdev)
{
	struct timed_vibrator_data *vib;

	INFO_MSG("[vib]Android Vibrator Driver Resume\n");

	vib = (struct timed_vibrator_data *)platform_get_drvdata(pdev);

	android_vibrator_force_set(vib, 0, vib->vibe_n_value);

	gpio_tlmm_config(GPIO_CFG(vib->motor_pwm_gpio, 6, GPIO_CFG_OUTPUT,
		GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(vib->haptic_en_gpio, 0, GPIO_CFG_OUTPUT,
		GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);

	return 0;
}

static void android_vibrator_shutdown(struct platform_device *pdev)
{
	struct timed_vibrator_data *vib;
	INFO_MSG("[vib]Android Vibrator Driver Shutdown\n");

	vib = (struct timed_vibrator_data *)platform_get_drvdata(pdev);
	android_vibrator_force_set(vib, 0, vib->vibe_n_value);
}

#ifdef CONFIG_OF
static struct of_device_id sm100_match_table[] = {
    { .compatible = "syncoam,sm100",},
    { },
};
#endif

static struct platform_driver android_vibrator_driver = {
	.probe = android_vibrator_probe,
	.remove = android_vibrator_remove,
	.shutdown = android_vibrator_shutdown,
	.suspend = android_vibrator_suspend,
	.resume = android_vibrator_resume,
	.driver = {
		.name = DEVICE_NAME,
#ifdef CONFIG_OF
		.of_match_table = sm100_match_table,
#endif
	},
};

static int __init android_vibrator_init(void)
{
	INFO_MSG("[vib]Android Vibrator Driver Init\n");
#ifdef ANDROID_VIBRATOR_USE_WORKQUEUE
	vibrator_workqueue = create_workqueue("vibrator");
	if (!vibrator_workqueue) {
		pr_err("%s: out of memory\n", __func__);
		return -ENOMEM;
	}
#endif
	return platform_driver_register(&android_vibrator_driver);
}

static void __exit android_vibrator_exit(void)
{
	INFO_MSG("[vib]Android Vibrator Driver Exit\n");
#ifdef ANDROID_VIBRATOR_USE_WORKQUEUE
	if (vibrator_workqueue)
		destroy_workqueue(vibrator_workqueue);
	vibrator_workqueue = NULL;
#endif
	platform_driver_unregister(&android_vibrator_driver);
}

/* to let init lately */
late_initcall_sync(android_vibrator_init);
module_exit(android_vibrator_exit);
/*
MODULE_AUTHOR("LG Electronics Inc.");
MODULE_DESCRIPTION("Android Common Vibrator Driver");
MODULE_LICENSE("GPL");
*/
