/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _SEA_SIGNAL_CONFIG_H
#define _SEA_SIGNAL_CONFIG_H

#include <security.h>

int sea_signal_init(void);
void sea_signal_exit(void);
bool sea_signal_enabled(void);
void sea_detect_sensitive_domain(u32 ssid, u32 tsid, u32 requested, struct av_decision *avd);
void sea_detect_permissive(u32 ssid, struct av_decision *avd);
extern int is_sea_socket_initialized;

#endif /* _SEA_SIGNAL_CONFIG_H */
