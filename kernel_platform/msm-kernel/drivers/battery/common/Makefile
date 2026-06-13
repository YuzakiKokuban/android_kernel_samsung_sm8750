obj-$(CONFIG_DIRECT_CHARGING)		+= sec-direct-charger.o
sec-direct-charger-$(CONFIG_DIRECT_CHARGING)	+= sb_pass_through.o sec_direct_charger.o

ccflags-y := -Wformat

obj-$(CONFIG_SEC_PD)				+= sec_pd.o
obj-$(CONFIG_BATTERY_SAMSUNG)		+= sb_wireless.o

obj-$(CONFIG_BATTERY_SAMSUNG)	+= sec-battery.o
sec-battery-$(CONFIG_ENG_BATTERY_CONCEPT) += sb_checklist_app.o
sec-battery-$(CONFIG_BATTERY_SAMSUNG)		+= sec_charging_modprobe.o sec_battery.o sec_battery_vote.o sec_battery_thermal.o sec_battery_sysfs.o sec_battery_dt.o sec_battery_ttf.o sec_adc.o sec_cisd.o sb_full_soc.o
sec-battery-$(CONFIG_STEP_CHARGING)		+= sec_step_charging.o
sec-battery-$(CONFIG_WIRELESS_AUTH)		+= sec_battery_misc.o
sec-battery-$(CONFIG_WIRELESS_CHARGING)	+= sec_battery_wc.o
sec-battery-$(CONFIG_WIRELESS_TX_MODE)	+= sb_tx.o
sec-battery-$(CONFIG_BATTERY_LOGGING)	+= battery_logger.o sb_batt_dump.o

obj-$(CONFIG_UPDATE_BATTERY_DATA)	+= sec_battery_data.o

ifeq ($(CONFIG_SEC_KUNIT), y)
ifeq ($(CONFIG_BATTERY_SAMSUNG), m)
GCOV_PROFILE_sec_battery.o := y
GCOV_PROFILE_sec_battery_thermal.o := y
GCOV_PROFILE_sec_battery_vote.o := y
GCOV_PROFILE_sec_adc.o := y
GCOV_PROFILE_sec_battery_sysfs.o := y
GCOV_PROFILE_sec_battery_ttf.o := y
GCOV_PROFILE_sec_battery_dt.o := y
GCOV_PROFILE_sec_cisd.o := y
ifneq ($(CONFIG_WIRELESS_CHARGING), n)
GCOV_PROFILE_sec_battery_wc.o := y
endif
ifneq ($(CONFIG_STEP_CHARGING), n)
GCOV_PROFILE_sec_step_charging.o := y
endif
ifneq ($(CONFIG_WIRELESS_AUTH), n)
GCOV_PROFILE_sec_battery_misc.o := y
endif
endif
GCOV_PROFILE_sec_pd.o := y
ifeq ($(CONFIG_UML), y)
endif
endif
ccflags-y := -Wformat
