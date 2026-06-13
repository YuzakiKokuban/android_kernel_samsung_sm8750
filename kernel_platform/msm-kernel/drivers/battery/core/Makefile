obj-$(CONFIG_SB_CORE)	+= sb-core.o
sb-core-$(CONFIG_SB_CORE)	+= sb_core.o
sb-core-$(CONFIG_SB_PQUEUE)	+= sb_pqueue.o
sb-core-$(CONFIG_SB_NOTIFY)	+= sb_notify.o
sb-core-$(CONFIG_SB_SYSFS)	+= sb_sysfs.o
sb-core-$(CONFIG_SB_VOTE)	+= sb_vote.o

ifeq ($(CONFIG_SEC_KUNIT), y)
 GCOV_PROFILE_sb_pqueue.o := $(CONFIG_SEC_KUNIT)
 GCOV_PROFILE_sb_notify.o := $(CONFIG_SEC_KUNIT)
 GCOV_PROFILE_sb_sysfs.o := $(CONFIG_SEC_KUNIT)
 GCOV_PROFILE_sb_vote.o := $(CONFIG_SEC_KUNIT)
endif

ccflags-y := -Wformat
