/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

 Log utility

 GENERAL DESCRIPTION
 This header declares a logging utility

 Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 SPDX-License-Identifier: BSD-3-Clause-Clear
 =============================================================================*/
#ifndef __XTRAT_WIFI_LOG_H__
#define __XTRAT_WIFI_LOG_H__

namespace qc_loc_fw
{

enum ERROR_LEVEL
{
  EL_LOG_OFF = 0, EL_ERROR = 1, EL_WARNING = 2, EL_INFO = 3, EL_DEBUG = 4, EL_VERBOSE = 5, EL_LOG_ALL = 100
};

// bits masks for Error Output preference
enum ERROR_OUTPUT
{
  EO_ANDROID = 0x1, EO_STDOUT = 0x2, EO_ALL = 0x3
};

void log_error(const char * const local_log_tag, const char * const format, ...);
void log_warning(const char * const local_log_tag, const char * const format, ...);
void log_info(const char * const local_log_tag, const char * const format, ...);
void log_debug(const char * const local_log_tag, const char * const format, ...);
void log_verbose(const char * const local_log_tag, const char * const format, ...);

int log_set_global_level(const ERROR_LEVEL level);
int log_set_global_tag(const char * const tag);
int log_flush_all_local_level();
int log_flush_local_level_for_tag(const char *const tag);

int log_set_local_level_for_tag(const char *const tag, const ERROR_LEVEL level);
int log_set_local_level_for_tag(const char *const tag, const ERROR_LEVEL level,
                                const ERROR_OUTPUT output);
bool is_log_verbose_enabled(const char * const local_log_tag);

} // namespace qc_loc_fw

#endif //#ifndef __XTRAT_WIFI_LOG_H__
