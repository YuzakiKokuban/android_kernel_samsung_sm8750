#ifndef	__SEC_HALL_DUMPKEY_H__
#define __SEC_HALL_DUMPKEY_H__
struct sec_hall_dumpkey_param {
	unsigned int keycode;
	int down;
};

struct hall_dump_callbacks {
	void (*inform_dump)(struct device *dev);
};
#endif /* __SEC_HALL_DUMPKEY_H__ */
