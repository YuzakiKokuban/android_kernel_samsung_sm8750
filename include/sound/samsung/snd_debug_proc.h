
/* SPDX-License-Identifier: GPL-2.0-only */
/*
 *  snd_debug_proc.h - header for SAMSUNG Audio debugging.
 */

#include <linux/mutex.h>

#define AUD_LOG_BUF_SIZE	SZ_64K

struct snd_debug_proc {
	char log_buf[AUD_LOG_BUF_SIZE];
	bool is_enabled;
	unsigned int buf_pos;
	unsigned int buf_full;
	struct mutex lock;
	void (*save_log)(char *buf, int len);
};

#if IS_ENABLED(CONFIG_SND_SOC_SAMSUNG_AUDIO)
void sdp_info_print(const char *fmt, ...);

void sdp_boot_print(const char *fmt, ...);
#else
inline void sdp_info_print(const char *fmt, ...)
{
}

inline void sdp_boot_print(const char *fmt, ...)
{
}
#endif

