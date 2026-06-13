
ifeq ($(CONFIG_ARCH_WAIPIO), y)
dtbo-y += waipio-vidc.dtbo
endif

ifeq ($(CONFIG_ARCH_KALAMA), y)
dtbo-y += kalama-vidc.dtbo
dtbo-y += kalama-vidc-v2.dtbo
endif

ifeq ($(CONFIG_ARCH_PINEAPPLE), y)
dtbo-y += pineapple-vidc.dtbo
dtbo-y += pineapple-vidc-v2.dtbo
endif

ifeq ($(CONFIG_ARCH_SUN), y)
dtbo-y += sun-vidc.dtbo
dtbo-y += sun-vidc-v2.dtbo
endif

always-y    := $(dtb-y) $(dtbo-y)
subdir-y    := $(dts-dirs)
clean-files    := *.dtb *.dtbo
