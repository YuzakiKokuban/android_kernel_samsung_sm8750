bbg-objs += baseband_guard.o
bbg-objs += tracing/tracing.o

ccflags-y += -I$(srctree)/security/selinux -I$(srctree)/security/selinux/include -I$(srctree)/block
ccflags-y += -I$(objtree)/security/selinux -include $(srctree)/include/uapi/asm-generic/errno.h
ccflags-y += -DBBG_USE_DEFINE_LSM
ccflags-y += -DBB_HAS_SELINUX_STATE
ccflags-y += -DBB_HAS_IOCTL_COMPAT
ccflags-y += -DBBG_VERSION=\"manual\"
ccflags-y += -DBBG_REPO=\"vc-teahouse/Baseband-guard\"

obj-$(CONFIG_BBG) += bbg.o

# Ensure selinux flask.h is generated before BBG compilation
# (objsec.h includes flask.h which is a generated header)
$(obj)/baseband_guard.o: $(objtree)/security/selinux/flask.h
$(obj)/tracing/tracing.o: $(objtree)/security/selinux/flask.h
