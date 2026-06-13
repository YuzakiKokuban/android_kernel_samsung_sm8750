#ifndef __LEGACY__SEC_PARAM_H__
#define __LEGACY__SEC_PARAM_H__

#include <linux/samsung/bsp/sec_param.h>

static inline bool sec_get_param(size_t index, void *value)
{
	return sec_param_get(index, value);
}

static inline bool sec_set_param(size_t index, void *value)
{
	return sec_param_set(index, value);
}

#endif
