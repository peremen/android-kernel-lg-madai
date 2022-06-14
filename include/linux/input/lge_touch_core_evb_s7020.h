/* include/linux/lge_touch_core.h
 *
 * Copyright (C) 2011 LGE.
 *
 * Author: yehan.ahn@lge.com, hyesung.shin@lge.com
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef LGE_TOUCH_CORE_H
#define LGE_TOUCH_CORE_H

//#define ABS_M(p,v) ( ((v)>0) ? (v):-(v))
//#define ABS_M(p,v) ( ((p)==0) ? (v+719):(v+1279))//0 -> x position, 1 -> y position

//                            
#if defined(CONFIG_TOUCHSCREEN_SYNAPTICS_S7020_EVB)
#define CONFIG_MACH_MSM8926_MADAI_M//Main Feature
#define CONFIG_MACH_MSM8926_MADAI_S//Sub Feature
#endif
#define MAX_FINGER	10

#if defined(CONFIG_MACH_MSM8926_MADAI_M)
#define MAX_BUTTON	3
#else
#define MAX_BUTTON	4
#endif
struct touch_device_caps
{
	u8		button_support;
	u16		y_button_boundary;
	u32		button_margin;		// percentage %
	u8		number_of_button;
	u32		button_name[MAX_BUTTON];
	u8		is_width_supported;
	u8	 	is_pressure_supported;
	u8		is_id_supported;
	u32		max_width;
	u32		max_pressure;
	u32		max_id;
	u32		x_max;
	u32		y_max;
	u32		lcd_x;
	u32		lcd_y;
};

struct touch_operation_role
{
	u8		operation_mode;	// interrupt = 1 , polling = 0;
	u8		key_type;		// none = 0, hard_touch_key = 1, virtual_key = 2
	u8		report_mode;
	u8		delta_pos_threshold;
	u8		orientation;	// 0' = 0, 90' = 1, 180' = 2, 270' = 3
	u32		report_period;	// ns
	u32		booting_delay;	// ms
	u32		reset_delay;	// ms
	u8		suspend_pwr;
	u8		resume_pwr;
	int		jitter_filter_enable;	// enable = 1, disable = 0
	int		jitter_curr_ratio;
	int		accuracy_filter_enable;	// enable = 1, disable = 0
	unsigned long	irqflags;
	int		ghost_detection_enable;
	int		ta_debouncing_count;
};

struct touch_power_module
{
	u8		use_regulator;
	char	vdd[30];
	int		vdd_voltage;
	char	vio[30];
	int		vio_voltage;
	int		(*power)	(struct i2c_client *client, int on);
};

struct touch_platform_data
{
	u32	int_pin;
	u32	reset_pin;
	char	maker[30];
	char	fw_version[11];
	struct touch_device_caps*		caps;
	u8 num_caps;
	struct touch_operation_role*	role;
	u8 num_role;
	struct touch_power_module*		pwr;
	u8 num_pwr;
};

struct t_data
{
	u16	id;
	u16	x_position;
	u16	y_position;
	u16	width_major;
	u16	width_minor;
	u16	width_orientation;
	u16	pressure;
	u8	status;
};

struct b_data
{
	u16	key_code;
	u16	state;
};

struct touch_data
{
	u8		total_num;
	u8		prev_total_num;
	u8		state;
	u8		palm_touch;
	struct t_data	curr_data[MAX_FINGER];
	struct t_data	prev_data[MAX_FINGER];
	struct b_data	curr_button;
	struct b_data	prev_button;
};

struct touch_fw_info
{
	u8		fw_rev;
	u8		fw_image_rev;
	u8		manufacturer_id;
	u8		product_id[11];
	u8		fw_image_product_id[11];
	unsigned char	*fw_start;
	unsigned long	fw_size;
	u8		fw_img_config_id[5];
	u8		fw_config_id[4];
};

struct fw_upgrade_info
{
	char		fw_path[256];
	u8			fw_force_upgrade;
	volatile u8	is_downloading;
};

struct rect
{
	u16	left;
	u16	right;
	u16	top;
	u16	bottom;
};

struct section_info
{
	struct rect	panel;
	struct rect button[MAX_BUTTON];
	struct rect button_cancel[MAX_BUTTON];
	u16 b_inner_width;
	u16 b_width;
	u16 b_margin;
	u16 b_height;
	u16 b_num;
	u16 b_name[MAX_BUTTON];
};

struct ghost_finger_ctrl {
	volatile u8	 stage;
#if defined(CONFIG_MACH_MSM8960_L1) || defined(CONFIG_MACH_MSM8926_MADAI_M)
	volatile u8     incoming_call;
#endif
	volatile u8	probe;
	int count;
	int min_count;
	int max_count;
	int ghost_check_count;
	int baseline_state;
};

struct jitter_history_data{
	u16	x;
	u16	y;
	u16	pressure;
	int	delta_x;
	int	delta_y;
	int	mod_x;
	int 	mod_y;
};

struct jitter_filter_info {
	int	id_mask;
	int	adjust_margin;
	struct jitter_history_data	his_data[10];
	int	ignore_pressure_gap;
	int	delta_max;
};

struct accuracy_history_data {
	u16	x;
	u16	y;
	u16	pressure;
	int	count;
	int	mod_x;
	int	mod_y;
	int 	axis_x;
	int	axis_y;
	int	prev_total_num;
};

struct accuracy_filter_info {
	int	ignore_pressure_gap;
	int	touch_max_count;
	int	delta_max;
	int	max_pressure;
	int	direction_count;
	int	time_to_max_pressure;
	u16	finish_filter;
	struct accuracy_history_data	his_data;
};

struct lge_touch_data
{
	void*			h_touch;
	atomic_t		next_work;
	atomic_t		device_init;
	u8				work_sync_err_cnt;
	u8				ic_init_err_cnt;
	volatile int	curr_pwr_state;
	struct i2c_client 			*client;
	struct input_dev 			*input_dev;
	struct hrtimer 				timer;
	struct work_struct  		work;
	struct delayed_work			work_init;
	struct delayed_work			work_touch_lock;
	struct work_struct  		work_fw_upgrade;
	struct mutex			    irq_work_mutex;
	#if defined(CONFIG_FB)
	struct notifier_block fb_notif;
    #elif defined(CONFIG_HAS_EARLYSUSPEND)
	struct early_suspend		early_suspend;
    #endif
	struct touch_platform_data 	*pdata;
	struct touch_data			ts_data;
	struct touch_fw_info		fw_info;
	struct fw_upgrade_info		fw_upgrade;
	struct section_info			st_info;
	struct kobject 				lge_touch_kobj;
	struct ghost_finger_ctrl	gf_ctrl;
	struct jitter_filter_info	jitter_filter;
	struct accuracy_filter_info	accuracy_filter;
};


struct touch_device_driver {
	int		(*probe)		(struct lge_touch_data *lge_touch_ts);
	void	(*remove)		(struct i2c_client *client);
	int		(*init)			(struct i2c_client *client, struct touch_fw_info* info);
	int		(*data)			(struct i2c_client *client, struct t_data* data, struct b_data* button, u8* total_num, u8* palm_touch);
	int		(*power)		(struct i2c_client *client, int power_ctrl);
	int		(*ic_ctrl)		(struct i2c_client *client, u8 code, u16 value);
	int 	(*fw_upgrade)	(struct i2c_client *client, const char* fw_path);
};

enum{
	POLLING_MODE = 0,
	INTERRUPT_MODE,
	HYBRIDE_MODE
};

enum{
	POWER_OFF = 0,
	POWER_ON,
	POWER_SLEEP,
	POWER_WAKE
};

enum{
	KEY_NONE = 0,
	TOUCH_HARD_KEY,
	TOUCH_SOFT_KEY,
	VIRTUAL_KEY,
};

enum{
	CONTINUOUS_REPORT_MODE = 0,
	REDUCED_REPORT_MODE,
};

enum{
	RESET_NONE = 0,
	SOFT_RESET,
	PIN_RESET,
	VDD_RESET,
};

enum{
	DOWNLOAD_COMPLETE = 0,
	UNDER_DOWNLOADING,
};

enum{
	OP_NULL = 0,
	OP_RELEASE,
	OP_SINGLE,
	OP_MULTI,
	OP_LOCK,
};

enum{
	KEY_NULL=0,
	KEY_PANEL,
	KEY_BOUNDARY
};

enum{
	DO_NOT_ANYTHING = 0,
	ABS_PRESS,
	ABS_RELEASE,
	BUTTON_PRESS,
	BUTTON_RELEASE,
	BUTTON_CANCEL,
	TOUCH_BUTTON_LOCK,
	TOUCH_ABS_LOCK
};

enum{
	BUTTON_RELEASED	= 0,
	BUTTON_PRESSED	= 1,
	BUTTON_RELEASE_ALL = 2,
	BUTTON_CANCLED	= 0xff,
};

enum{
	FINGER_RELEASED	= 0,
	FINGER_PRESSED	= 1,
};

enum{
	KEYGUARD_RESERVED,
	KEYGUARD_ENABLE,
};

#if defined(CONFIG_MACH_MSM8960_L1) || defined(CONFIG_MACH_MSM8926_MADAI_M)
enum{
	INCOMIMG_CALL_RESERVED,
	INCOMIMG_CALL_TOUCH,
};
#endif

enum{
	GHOST_STAGE_CLEAR=0,
	GHOST_STAGE_1=1,
	GHOST_STAGE_2=2,
	GHOST_STAGE_3=4,
	GHOST_STAGE_4=8,
};

enum{
	BASELINE_OPEN = 0,
	BASELINE_FIX,
	BASELINE_REBASE,
};

enum{
	IC_CTRL_CODE_NONE = 0,
	IC_CTRL_BASELINE,
	IC_CTRL_READ,
	IC_CTRL_WRITE,
	IC_CTRL_RESET_CMD,
};

enum{
	DEBUG_NONE				= 0,
	DEBUG_BASE_INFO			= (1U << 0),	// 1
	DEBUG_TRACE				= (1U << 1),	// 2
	DEBUG_GET_DATA			= (1U << 2),	// 4
	DEBUG_ABS				= (1U << 3),	// 8
	DEBUG_BUTTON			= (1U << 4),	// 16
	DEBUG_FW_UPGRADE		= (1U << 5), 	// 32
	DEBUG_GHOST				= (1U << 6),	// 64
	DEBUG_IRQ_HANDLE		= (1U << 7),	// 128
	DEBUG_POWER				= (1U << 8),	// 256
	DEBUG_JITTER			= (1U << 9),	// 512
	DEBUG_ACCURACY			= (1U << 10),	// 1024
};
#if defined(CONFIG_MACH_MSM8926_MADAI_M)
enum{
	CONFIG_ID_NONE=0,
	CONFIG_ID_FORMAL_VER,
	CONFIG_ID_TEST_VER,
};
#endif

#ifdef LGE_TOUCH_TIME_DEBUG
enum{
	TIME_ISR_START = 0,
	TIME_INT_INTERVAL,
	TIME_THREAD_ISR_START,
	TIME_WORKQUEUE_START,
	TIME_WORKQUEUE_END,
	TIME_FW_UPGRADE_START,
	TIME_FW_UPGRADE_END,
	TIME_PROFILE_MAX,
};

enum{
	DEBUG_TIME_PROFILE_NONE			= 0,
	DEBUG_TIME_INT_INTERVAL			= (1U << 0),	// 1
	DEBUG_TIME_INT_IRQ_DELAY		= (1U << 1),	// 2
	DEBUG_TIME_INT_THREAD_IRQ_DELAY	= (1U << 2),	// 4
	DEBUG_TIME_DATA_HANDLE			= (1U << 3),	// 8
	DEBUG_TIME_FW_UPGRADE			= (1U << 4),	// 16
	DEBUG_TIME_PROFILE_ALL			= (1U << 5),	// 32
};
#endif

enum{
	TIME_EX_PROBE,
	TIME_EX_RESUME_ON,
	TIME_EX_FIRST_INT_TIME,
	TIME_EX_PREV_PRESS_TIME,
	TIME_EX_CURR_PRESS_TIME,
	TIME_EX_BUTTON_PRESS_START_TIME,
	TIME_EX_BUTTON_PRESS_END_TIME,
	TIME_EX_FIRST_GHOST_DETECT_TIME,
	TIME_EX_SECOND_GHOST_DETECT_TIME,
	TIME_EX_PROFILE_MAX
};

#define LGE_TOUCH_NAME		"lge_touch"
#define LGE_TOUCH_SYNAPTICS_NAME		"lge_touch"

#define LGE_TOUCH_SYNATICS_I2C_SLAVE_ADDR	0x20
#define LGE_TOUCH_ATMEL_I2C_SLAVE_ADDR		0x4A

/* Debug Mask setting */
#define TOUCH_DEBUG_PRINT   (1)
#define TOUCH_ERROR_PRINT   (1)
#define TOUCH_INFO_PRINT   	(1)

#if defined(TOUCH_INFO_PRINT)
#define TOUCH_INFO_MSG(fmt, args...) \
		printk(KERN_INFO "[Touch] " fmt, ##args);
#else
#define TOUCH_INFO_MSG(fmt, args...)     {};
#endif

#if defined(TOUCH_ERROR_PRINT)
#define TOUCH_ERR_MSG(fmt, args...) \
		printk(KERN_ERR "[Touch E] [%s %d] " \
				fmt, __FUNCTION__, __LINE__, ##args);
#else
#define TOUCH_ERR_MSG(fmt, args...)     {};
#endif

#if defined(TOUCH_DEBUG_PRINT)
#define TOUCH_DEBUG_MSG(fmt, args...) \
		printk(KERN_INFO "[Touch D] [%s %d] " \
				fmt, __FUNCTION__, __LINE__, ##args);
#else
#define TOUCH_DEBUG_MSG(fmt, args...)     {};
#endif

int  touch_evb_driver_register(struct touch_device_driver* driver,struct of_device_id *match_table);
void touch_evb_driver_unregister(void);

void set_touch_evb_handle(struct i2c_client *client, void* h_touch);
void* get_touch_evb_handle(struct i2c_client *client);
int touch_evb_i2c_read(struct i2c_client *client, u8 reg, int len, u8 *buf);
int touch_evb_i2c_write(struct i2c_client *client, u8 reg, int len, u8 *buf);
int touch_evb_i2c_write_byte(struct i2c_client *client, u8 reg, u8 data);

extern u32 touch_evb_debug_mask;
extern u32 touch_evb_time_debug_mask;

#endif
