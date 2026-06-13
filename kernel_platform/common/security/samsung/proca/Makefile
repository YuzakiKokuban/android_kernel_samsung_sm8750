SHELL := /bin/bash

KERNEL := $(VERSION)_$(PATCHLEVEL)
ifeq ($(CONFIG_MACH_MT6853),y)
    KERNEL := 4_14
else ifeq ($(CONFIG_MACH_MT6833),y)
    KERNEL := 4_14
else ifeq ($(CONFIG_MACH_MT6768),y)
    KERNEL := 4_14
endif

# For kernel version 4.14 or lower
ifeq ($(KERNEL),4_14)
$(obj)/proca_certificate-asn1.o: $(obj)/proca_certificate-asn1.c \
                               $(obj)/proca_certificate-asn1.h

$(obj)/proca_certificate.o: $(obj)/proca_certificate-asn1.h

obj-$(CONFIG_PROCA) += proca_identity.o proca_fcntl.o \
                               proca_task_descr.o proca_certificate-asn1.o \
                               proca_certificate.o proca_table.o \
                               proca_config.o

$(obj)/proca_certificate_db-asn1.o: $(obj)/proca_certificate_db-asn1.c \
                               $(obj)/proca_certificate_db-asn1.h

$(obj)/s_os/proca_certificate_db.o: $(obj)/proca_certificate_db-asn1.h

obj-$(CONFIG_PROCA_CERTIFICATES_DB) += proca_certificate_db-asn1.o
else
$(obj)/proca_certificate.asn1.o: $(obj)/proca_certificate.asn1.c \
                               $(obj)/proca_certificate.asn1.h

$(obj)/proca_certificate.o: $(obj)/proca_certificate.asn1.h

obj-$(CONFIG_PROCA) += proca_identity.o proca_fcntl.o \
                               proca_task_descr.o proca_certificate.asn1.o \
                               proca_certificate.o proca_table.o \
                               proca_config.o

$(obj)/proca_certificate_db.asn1.o: $(obj)/proca_certificate_db.asn1.c \
                               $(obj)/proca_certificate_db.asn1.h

$(obj)/s_os/proca_certificate_db.o: $(obj)/proca_certificate_db.asn1.h

obj-$(CONFIG_PROCA_CERTIFICATES_DB) += proca_certificate_db.asn1.o
endif

ccflags-y += -I$(srctree)/security/samsung/proca/
arflags-y += -I$(srctree)/security/samsung/proca/
asflags-y += -I$(srctree)/security/samsung/proca/
ccflags-y += -I$(srctree)/security/samsung/five/

ifdef CONFIG_PROCA_S_OS

obj-$(CONFIG_PROCA_CERTIFICATES_DB) += s_os/proca_certificate_db.o \
                            s_os/proca_keyring.o \
                            proca_cert_builtin.o \
                            s_os/proca_storage_db.o

obj-$(CONFIG_PROCA_CERT_DEVICE) += s_os/proca_certificate_dev.o

obj-$(CONFIG_PROCA_CERTIFICATES_XATTR) += s_os/proca_storage_xattr.o

obj-$(CONFIG_PROCA) += s_os/proca_lsm.o \
                      s_os/proca_vfs.o \
                      s_os/proca_audit.o

ccflags-y += -Isecurity/samsung/proca/s_os
else
obj-$(CONFIG_PROCA) += proca_lsm.o
endif # CONFIG_PROCA_S_OS

# clean-files is given relative to the current directory,
# so don't use (obj)/ prefix here.
# Look at file ../../scripts/Makefile.clean for details.
ifeq ($(KERNEL),4_14)
clean-files += proca_certificate-asn1.c proca_certificate-asn1.h
ifdef CONFIG_PROCA_CERTIFICATES_DB
clean-files += proca_certificate_db-asn1.c proca_certificate_db-asn1.h
endif
else
clean-files += proca_certificate.asn1.c proca_certificate.asn1.h
ifdef CONFIG_PROCA_CERTIFICATES_DB
clean-files += proca_certificate_db.asn1.c proca_certificate_db.asn1.h
endif
endif

subdir-ccflags-y += -Wformat

# kunit tests options:
ENABLED_TARGETS_LIST = ''
ifeq ($(CONFIG_SEC_KUNIT), y)
  GCOV_PROFILE := y
  ifeq ($(CONFIG_UML), y)
    ccflags-$(CONFIG_PROCA) += -DPROCA_KUNIT_ENABLED
    subdir-$(CONFIG_PROCA) += kunit_test
    obj-$(CONFIG_PROCA) += kunit_test/
  else
    ifneq ($(filter $(TARGET_DEVICE),$(ENABLED_TARGETS_LIST)),)
      $(warning TARGET_DEVICE is $(TARGET_DEVICE). building on-device kunit tests)
      obj-m += kunit_test/proca_certificate_test.o
      obj-m += kunit_test/proca_identity_test.o
      obj-m += kunit_test/proca_table_test.o
    else
      $(warning security/samsung/proca: TARGET_DEVICE: $(TARGET_DEVICE). skip on-device kunit test)
    endif
  endif
endif
