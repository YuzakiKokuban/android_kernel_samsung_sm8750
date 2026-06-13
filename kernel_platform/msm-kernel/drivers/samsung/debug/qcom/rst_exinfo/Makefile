obj-$(CONFIG_SEC_QC_RST_EXINFO)		+= sec_qc_rst_exinfo.o
sec_qc_rst_exinfo-objs			:= sec_qc_rst_exinfo_main.o \
					   sec_qc_arch_rst_exinfo_vh.o \
					   sec_qc_rst_exinfo_fault.o

sec_qc_rst_exinfo-$(CONFIG_ARM64)	+= sec_qc_arm64_rst_exinfo.o \
					   sec_qc_arm64_rst_exinfo_vh.o

GCOV_PROFILE_sec_qc_rst_exinfo.o		:= $(CONFIG_KUNIT)
