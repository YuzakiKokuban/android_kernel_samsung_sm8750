obj-$(CONFIG_SEC_DEBUG)			+= sec_debug.o
sec_debug-objs				:= sec_debug_main.o \
					   sec_ap_serial.o \
					   sec_user_fault.o \
					   sec_debug_node.o \
					   sec_debug_show_stat.o \
					   sec_panic_with_reason.o

sec_debug-$(CONFIG_SEC_FORCE_ERR)	+= sec_force_err.o
