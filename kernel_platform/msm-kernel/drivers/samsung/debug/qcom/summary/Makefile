obj-$(CONFIG_SEC_QC_SUMMARY)		+= sec_qc_summary.o
sec_qc_summary-objs			:= sec_qc_summary_main.o \
					   sec_qc_summary_task.o \
					   sec_qc_summary_kconst.o \
					   sec_qc_summary_ksyms.o \
					   sec_qc_summary_sched_log.o \
					   sec_qc_summary_aplpm.o \
					   sec_qc_summary_dump_sink.o \
					   sec_qc_summary_debug_kinfo.o \
					   sec_qc_summary_kallsyms.o \
					   sec_qc_summary_mock.o

sec_qc_summary-$(CONFIG_ARM64)		+= sec_qc_summary_arm64_ap_context.o \
					   sec_qc_summary_var_mon.o \
					   sec_qc_summary_coreinfo.o

__SEC_QC_SUMMARY_BUILD_ROOT		:= $(shell echo $(srctree) | sed -e 's|/android/kernel_platform/.\+$$||' -e 's|^.\+/||')
CFLAGS_sec_qc_summary_var_mon.o		= -D__SEC_QC_SUMMARY_BUILD_ROOT=$(__SEC_QC_SUMMARY_BUILD_ROOT)
CFLAGS_sec_qc_summary_main.o		= -I$(srctree)/drivers/soc/qcom/

ccflags-y				+= -I$(srctree)/drivers/android

# FIXME: too tough including path
CFLAGS_sec_qc_summary_coreinfo.o	= -I$(srctree)
