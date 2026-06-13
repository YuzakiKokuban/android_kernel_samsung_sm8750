
#include <linux/module.h>

extern void parent_func(void);

void child_func(void) {
    parent_func();
}

MODULE_AUTHOR("Google, Inc.");
MODULE_DESCRIPTION("Android Test Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("2.0");