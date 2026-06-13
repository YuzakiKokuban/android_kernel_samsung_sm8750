ifneq ($(CONFIG_ARCH_QTI_VM), y)

ifeq ($(CONFIG_ARCH_SUN), y)
dtbo-y += sun-eva.dtbo
dtbo-y += sun-eva-v2.dtbo
endif

ifeq ($(CONFIG_ARCH_TUNA), y)
dtbo-y += tuna-eva.dtbo
endif

ifeq ($(CONFIG_ARCH_PINEAPPLE), y)
dtbo-y += pineapple-eva.dtbo
endif

ifeq ($(CONFIG_ARCH_KALAMA), y)
dtbo-y += trustedvm-kalama-eva-mtp.dtbo \
	trustedvm-kalama-eva-qrd.dtbo
endif

endif

always-y	:= $(dtb-y) $(dtbo-y)
subdir-y	:= $(dts-dirs)
clean-files	:= *.dtb *.dtbo
