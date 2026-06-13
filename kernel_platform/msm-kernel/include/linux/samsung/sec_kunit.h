#ifndef __SEC_KUNIT_H__
#define __SEC_KUNIT_H__

#include <linux/fs.h>
#include <linux/miscdevice.h>

#if IS_ENABLED(CONFIG_KUNIT)
#define __ss_static
#define __ss_inline
#define __ss_always_inline

static inline int sec_kunit_init_miscdevice(struct miscdevice *misc, const char *name)
{
	static struct file_operations dummy_fops = {
		.owner = THIS_MODULE,
	};

	misc->minor = MISC_DYNAMIC_MINOR;
	misc->name = name;
	misc->fops = &dummy_fops;

	return misc_register(misc);
}

static inline void sec_kunit_exit_miscdevice(struct miscdevice *misc)
{
	misc_deregister(misc);
}
#else
#define __ss_static		static
#define __ss_inline		inline
#define __ss_always_inline	__always_inline

static inline int sec_kunit_init_miscdevice(struct miscdevice *misc, const char *name) { return -EINVAL; }
static inline void sec_kunit_exit_miscdevice(struct miscdevice *misc) {}
#endif

#endif /* __SEC_KUNIT_H__ */
