-include $(TOPDIR)/rules.mk

CFLAGS := -Wall -g -MMD
LDFLAGS :=
LIBS :=
LOCAL_CFLAGS :=
EXTERNAL_CFLAGS :=

ifeq ($(CONFIG_TARGET_BOARD),"ipq806x")
is_ipq806x = 1
endif

ifeq ($(is_ipq806x), 1)
ifneq ($(strip $(TOOLPREFIX)),)
export  CROSS:=$(TOOLPREFIX)
endif

ifndef INSTALL_ROOT
INSTALL_ROOT= $(FSTMANDIR)/install
endif

export CC = $(CROSS)gcc
CFLAGS += -L$(INSTALL_ROOT)/lib
endif

external_srcs :=

ifeq ($(is_ipq806x), 1)
CONFIG_MUX_L2DA:=1
endif

PKG_CONFIG := pkg-config

EXTERNAL_SRC_DIR := external

progs := fstman

ifndef CONFIG_MUX_L2DA
FST_MUX_SRCS=fst_mux_bonding.c fst_tc.c
else
FST_MUX_SRCS=fst_mux_l2da.c
endif

local_srcs := $(FST_MUX_SRCS) \
	fst_manager.c

LOCAL_CFLAGS += -I$(EXTERNAL_SRC_DIR)/ -I$(EXTERNAL_SRC_DIR)/inih
EXTERNAL_CFLAGS += $(addprefix -I,$(sort $(dir $(wildcard $(EXTERNAL_SRC_DIR)/*/))))

LOCAL_CFLAGS += $(shell $(PKG_CONFIG) --cflags libnl-3.0)
ifeq ($(is_ipq806x), 1)
LIBS += -lnl-3
else
LIBS += $(shell $(PKG_CONFIG) --libs libnl-3.0)
endif


ifndef CONFIG_DBUS

external_srcs += \
	$(EXTERNAL_SRC_DIR)/wpa_ctrl.c \
	$(EXTERNAL_SRC_DIR)/eloop.c \
	$(EXTERNAL_SRC_DIR)/common.c \
	$(EXTERNAL_SRC_DIR)/os_unix.c \
	$(EXTERNAL_SRC_DIR)/wpa_debug.c \
	$(EXTERNAL_SRC_DIR)/fst_ctrl_aux.c \
	$(EXTERNAL_SRC_DIR)/inih/ini.c

EXTERNAL_CFLAGS += -DCONFIG_CTRL_IFACE -DCONFIG_CTRL_IFACE_UNIX -DCONFIG_FST
EXTERNAL_CFLAGS += -DCONFIG_DEBUG_FILE

local_srcs += fst_ctrl.c fst_cfgmgr.c fst_ini_conf.c main.c fst_rateupg.c fst_tpoll.c fst_capconfigstore.c

else

LOCAL_CFLAGS += \
	$(shell $(PKG_CONFIG) --cflags glib-2.0) \
	$(shell $(PKG_CONFIG) --cflags gio-2.0) \
	-DCONFIG_DBUS
LIBS += \
	$(shell $(PKG_CONFIG) --libs glib-2.0) \
	$(shell $(PKG_CONFIG) --libs gio-2.0)

local_srcs += fst_ctrl_dbus.c main_dbus.c

external_srcs += \
	$(EXTERNAL_SRC_DIR)/os_unix.c \
	$(EXTERNAL_SRC_DIR)/fst_ctrl_aux.c

endif

ifeq ($(is_ipq806x), 1)
# What we build by default:
ALL = $(progs)
endif

local_objs :=$(local_srcs:.c=.o)
external_objs := $(external_srcs:.c=.o)
all_objs :=$(local_objs) $(external_objs)

ifeq ($(is_ipq806x), 1)
all prod prof: $(progs) install
else
all prod prof: $(progs)
endif

fstman: $(all_objs)

prod prof: CFLAGS += -O2
prof: CFLAGS += -pg

prod: strip

$(all_objs): %.o: %.c

$(local_objs):
	$(CC) $(CFLAGS) $(LOCAL_CFLAGS) -o $@ -c $<

$(external_objs):
	$(CC) $(CFLAGS) $(EXTERNAL_CFLAGS) -o $@ -c $<

$(progs): %:
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

strip:
	strip $(progs)

# Doing installation (see comments at top of this file)
install:
ifeq ($(is_ipq806x), 1)
	mkdir -p $(INSTALL_ROOT)/usr/sbin/
	cp -a -f $(ALL) $(INSTALL_ROOT)/usr/sbin/
	@echo Installed outputs from `pwd`
endif


clean:
ifeq ($(is_ipq806x), 1)
	rm -rf install/usr/sbin/fstman
endif
	$(RM) $(all_objs) $(progs) $(all_objs:%.o=%.d)

echo:
	@echo $(progs) $(local_srcs) $(all_objs) $(all_objs:%.o=%.d)

-include $(all_objs:%.o=%.d)
