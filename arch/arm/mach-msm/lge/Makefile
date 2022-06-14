obj-$(CONFIG_MACH_LGE) += devices_lge.o

obj-$(CONFIG_MACH_MSM8926_MADAI) += board-8926-madai.o board-8926-madai-gpiomux.o

obj-$(CONFIG_LGE_BOOTLOADER_LOG) += lge_bootloader_log.o
obj-$(CONFIG_LGE_HANDLE_PANIC) += lge_handle_panic.o
obj-$(CONFIG_LGE_BOOTLOADER_TIME_CHECKER) += lge_bootloader_time_checker.o
obj-$(CONFIG_LGE_PM_CHARGING_TEMP_SCENARIO) += lge_charging_scenario.o
obj-$(CONFIG_MACH_LGE) += lge_kcal_ctrl.o
#[WiFi]_S[sangheon.shim@lge.com] BRCM WLAN
obj-$(CONFIG_WIFI_CONTROL_FUNC) += wifi_bcm_lge.o
obj-$(CONFIG_LGE_QFPROM_INTERFACE) += lge_qfprom_access.o
obj-$(CONFIG_LGE_ENABLE_MMC_STRENGTH_CONTROL) += lge_mmc_strength_control.o

#[Power] Battery Data 
ifdef CONFIG_LGE_PM_BATTERY_PROFILE_DATA
	ifdef CONFIG_LGE_PM_BATTERY_ID_CHECKER
		ifdef CONFIG_LGE_PM_BATTERY_CAPACITY_2440mAh
			obj-$(CONFIG_MACH_LGE) += bms-batterydata-LGE_BL_59UH_2440mAh_LG_Chem_Final.o bms-batterydata-LGE_BL_59UH_2440mAh_TOCAD_MADAI_Final.o
		else
			obj-$(CONFIG_MACH_LGE) += bms-batterydata-LGE_BL_54SH_2540mAh_LG_Chem_Final.o
		endif
	else
		obj-$(CONFIG_MACH_LGE) += bms-batterydata-LGE_BL_54SH_2540mAh_LG_Chem_Final.o
	endif
endif

CFLAGS_devices_lge.o += -Idrivers/staging/android

obj-$(CONFIG_PRE_SELF_DIAGNOSIS) += lge_pre_selfd.o

