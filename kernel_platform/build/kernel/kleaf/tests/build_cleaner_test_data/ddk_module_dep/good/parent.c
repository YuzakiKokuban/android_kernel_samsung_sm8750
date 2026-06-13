
#include <linux/module.h>

void parent_func(void) {}

EXPORT_SYMBOL(parent_func);

MODULE_AUTHOR("Google, Inc.");
MODULE_DESCRIPTION("Android Test Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("2.0");