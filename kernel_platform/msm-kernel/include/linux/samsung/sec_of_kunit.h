#ifndef __SEC_OF_KUNIT_H__
#define __SEC_OF_KUNIT_H__

#include <linux/miscdevice.h>
#include <linux/of.h>

#include <linux/samsung/builder_pattern.h>

struct sec_of_kunit_data {
	struct miscdevice misc;
	struct device_node *root;
	struct device_node *of_node;
	struct builder *bd;
};

/* NOTE: Inspired from 'drivers/of/unittest.c'. */

#define SEC_OF_KUNIT_DTB_INFO_EXTERN(name) \
	extern uint8_t __dtb_##name##_begin[]; \
	extern uint8_t __dtb_##name##_end[]

#define SEC_OF_KUNIT_DTB_INFO(overlay_name) \
struct sec_of_dtb_info overlay_name ## _info = { \
	.dtb_begin = __dtb_##overlay_name##_begin, \
	.dtb_end = __dtb_##overlay_name##_end, \
	.name = #overlay_name, \
}

struct sec_of_dtb_info {
	uint8_t *dtb_begin;
	uint8_t *dtb_end;
	char *name;
};

#if IS_ENABLED(CONFIG_KUNIT)
extern int sec_of_kunit_data_init(struct sec_of_kunit_data *testdata, const char *name, struct builder *bd, const char *compatible, struct sec_of_dtb_info *info);
extern void sec_of_kunit_data_exit(struct sec_of_kunit_data *testdata);
extern struct device_node *sec_of_kunit_dtb_to_fdt(struct sec_of_dtb_info *info);
#else
static int sec_of_kunit_data_init(struct sec_of_kunit_data *testdata, const char *name, struct builder *bd, const char *compatible, struct sec_of_dtb_info *info) { return -EINVAL; }
static void sec_of_kunit_data_exit(struct sec_of_kunit_data *testdata) {}
static inline struct device_node *sec_of_kunit_dtb_to_fdt(struct sec_of_dtb_info *info) { return ERR_PTR(-EINVAL); }
#endif

#endif /* __SEC_OF_KUNIT_H__ */
