ccflags-y += -I$(srctree)
ccflags-y += -I$(srctree)/security/samsung/five

KBUILD_CFLAGS := $(subst -Werror=strict-prototypes,,$(KBUILD_CFLAGS))

obj-y += five_lv_test.o
obj-y += five_crypto_test.o
obj-y += five_cache_test.o
obj-y += five_file.o
obj-y += five_dmverity_test.o
obj-y += five_keyring_test.o
obj-y += task_integrity_test.o
obj-y += five_cert_test.o
obj-y += five_audit_test.o
obj-y += five_state_test.o
ifeq ($(shell test -e $(srctree)/../../sm8650 && echo 1 || echo 0), 0)
  obj-y += five_pa_test.o
else
  ccflags-y += -DFIVE_KUNIT_SM8650U
endif
