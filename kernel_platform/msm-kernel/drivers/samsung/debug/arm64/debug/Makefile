obj-$(CONFIG_SEC_ARM64_DEBUG)		+= sec_arm64_debug.o
sec_arm64_debug-objs			:= sec_arm64_debug_main.o \
					   sec_arm64_force_err.o

CFLAGS_REMOVE_sec_arm64_force_err.o	+= -mgeneral-regs-only
