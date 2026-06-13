// SPDX-License-Identifier: GPL-2.0
/*
 *  sec_pm_log.h - header for SAMSUNG Power/Thermal logging.
 *
 */

#if IS_ENABLED(CONFIG_SEC_PM_LOG)
void ss_thermal_print(const char *fmt, ...);
void ss_power_print(const char *fmt, ...);
void ss_dcvsh_print(const char *fmt, ...);
#endif
