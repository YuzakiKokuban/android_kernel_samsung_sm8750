obj-$(CONFIG_SEC_LOG_BUF)		+= sec_log_buf.o
sec_log_buf-objs			:= sec_log_buf_main.o \
					   sec_log_buf_logger.o \
					   sec_log_buf_builtin.o \
					   sec_log_buf_console.o \
					   sec_log_buf_last_kmsg.o \
					   sec_log_buf_ap_klog.o

sec_log_buf-$(CONFIG_DEBUG_FS)		+= sec_log_buf_debugfs.o

sec_log_buf-$(CONFIG_SEC_LOG_BUF_USING_TP_CONSOLE)	+= sec_log_buf_tp_console.o
sec_log_buf-$(CONFIG_SEC_LOG_BUF_USING_KPROBE)		+= sec_log_buf_kprobe.o
sec_log_buf-$(CONFIG_SEC_LOG_BUF_USING_VH_LOGBUF)	+= sec_log_buf_vh_logbuf.o

ifeq ($(CONFIG_SEC_LOG_BUF_USING_VH_LOGBUF),y)
CFLAGS_sec_log_buf_vh_logbuf.o		= -I$(srctree)/kernel/printk
endif
