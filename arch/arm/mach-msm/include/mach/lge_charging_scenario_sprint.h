/*
 * LGE charging scenario Header file.
 *
 * Copyright (C) 2013 LG Electronics
 * mansu.lee <mansu.lee@lge.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __LGE_CHARGING_SCENARIO_H_
#define __LGE_CHARGING_SCENARIO_H_

#include <linux/kernel.h>

#ifndef CONFIG_LGE_PM_THERMAL
#define CONFIG_LGE_PM_THERMAL
#endif

#define DC_IUSB_VOLTUV   4000000
#ifdef CONFIG_MACH_MSM8926_X5_VZW
#define DC_IUSB_CURRENT  400
#else
#define DC_IUSB_CURRENT  450
#endif
#define DC_CURRENT_DEF   -1

/* Battery temperature states */
#ifdef CONFIG_MACH_MSM8926_MADAI
enum lge_battemp_states {
	CHG_BATTEMP_BL_M7,
	CHG_BATTEMP_M6_M5,
	CHG_BATTEMP_M4_43,
	CHG_BATTEMP_44_45,
	CHG_BATTEMP_46_OT,
	CHG_BATTEMP_AB_OT,

};
#else
enum lge_battemp_states {
	CHG_BATTEMP_BL_M11,
	CHG_BATTEMP_M10_M5,
	CHG_BATTEMP_M4_41,
	CHG_BATTEMP_42_45,
	CHG_BATTEMP_46_OT,
	CHG_BATTEMP_AB_OT,

};
#endif 
/*                     */
enum lge_charging_states {
	CHG_BATT_NORMAL_STATE,
	CHG_BATT_DECCUR_STATE,
	CHG_BATT_WARNIG_STATE,
	CHG_BATT_STPCHG_STATE,
};

/*                            */
enum lge_states_changes {
	STS_CHE_NONE,
	STS_CHE_NORMAL_TO_DECCUR,
	STS_CHE_NORMAL_TO_STPCHG,
	STS_CHE_DECCUR_TO_NORAML,
	STS_CHE_DECCUR_TO_STPCHG,
	STS_CHE_STPCHG_TO_NORMAL,
};

/* BTM status */
enum lge_btm_states {
	BTM_HEALTH_GOOD,
	BTM_HEALTH_OVERHEAT,
	BTM_HEALTH_COLD,
};

struct charging_info {
	int     batt_volt;
	int     batt_temp;
	int     is_charger;
#ifdef CONFIG_LGE_PM_THERMAL
	int     chg_current_ma;
	int     chg_current_te;
#endif
	bool	is_charger_changed;
};

struct charging_rsp {
	enum lge_charging_states    state;
	enum lge_states_changes     change_lvl;
	bool                        force_update;
	bool                        disable_chg;
	int                         dc_current;
	enum lge_btm_states         btm_state;
	int                         pseudo_chg_ui;
};

struct batt_temp_table {
	int                         min;
	int                         max;
	enum lge_battemp_states     battemp_state;
};

extern void
lge_monitor_batt_temp(struct charging_info req, struct charging_rsp *res);
#endif
/*                            */

