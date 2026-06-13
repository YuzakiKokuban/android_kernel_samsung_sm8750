ifeq ($(CONFIG_ARCH_PINEAPPLE), y)
ifeq ($(CONFIG_ARCH_QTI_VM), y)
dtbo-y += pineapple/pineapple-dsp-trustedvm.dtbo
else
dtbo-y += pineapple/pineapple-dsp.dtbo
endif
endif

ifeq ($(CONFIG_ARCH_BLAIR), y)
dtbo-y += blair/blair-dsp.dtbo
endif

ifeq ($(CONFIG_ARCH_SUN), y)
ifeq ($(CONFIG_ARCH_QTI_VM), y)
dtbo-y += sun/sun-dsp-trustedvm.dtbo
else
dtbo-y += sun/sun-dsp.dtbo
endif
endif

ifeq ($(CONFIG_ARCH_MONACO), y)
dtbo-y += monaco/monaco-dsp.dtbo
endif

ifeq ($(CONFIG_ARCH_PARROT), y)
dtbo-y += parrot/parrot-dsp.dtbo
endif

ifeq ($(CONFIG_ARCH_RAVELIN), y)
dtbo-y += ravelin/ravelin-dsp.dtbo
endif
always-y	:= $(dtb-y) $(dtbo-y)
subdir-y	:= $(dts-dirs)
clean-files	:= *.dtb *.dtbo
