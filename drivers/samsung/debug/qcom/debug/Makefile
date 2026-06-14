obj-$(CONFIG_SEC_QC_DEBUG)		+= sec_qc_debug.o
sec_qc_debug-objs			:= sec_qc_debug_main.o \
					   sec_qc_debug_lpm_log.o \
					   sec_qc_debug_reboot.o \
					   sec_qc_force_err.o \
					   sec_qc_boot_stat.o \
					   sec_qc_cp_dump_encrypt.o \
					   sec_qc_arch_vendor_hooks.o

sec_qc_debug-$(CONFIG_ARM64)		+= sec_qc_arm64_vendor_hooks.o
