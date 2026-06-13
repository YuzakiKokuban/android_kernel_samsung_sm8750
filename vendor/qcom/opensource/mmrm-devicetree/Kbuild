ifeq ($(CONFIG_ARCH_SUN), y)
dtbo-y += sun/sun-mmrm.dtbo
dtbo-y += sun/sun-mmrm-test.dtbo
endif

ifeq ($(CONFIG_ARCH_PINEAPPLE), y)
dtbo-y += pineapple/pineapple-mmrm.dtbo
dtbo-y += pineapple/pineapple-mmrm-test.dtbo
dtbo-y += pineapple/pineapple-mmrm-test-v2.dtbo
endif

ifeq ($(CONFIG_ARCH_KALAMA), y)
	ifneq ($(CONFIG_ARCH_QTI_VM), y)
		dtbo-y += kalama/kalama-mmrm.dtbo
		dtbo-y += kalama/kalama-mmrm-test.dtbo
		dtbo-y += kalama/kalama-mmrm-v2.dtbo
		dtbo-y += kalama/kalama-mmrm-test-v2.dtbo
		ifeq ($(CONFIG_MSM_MMRM_VM),y)
		    dtbo-y += kalama/kalama-mmrm-vm-be.dtbo
		endif
	else
		ifeq ($(CONFIG_MSM_MMRM_VM),y)
		    dtbo-y += kalama/kalama-mmrm-vm-fe.dtbo
		    dtbo-y += kalama/kalama-mmrm-vm-fe-test.dtbo
		endif
	endif
endif

ifeq ($(CONFIG_ARCH_WAIPIO), y)
dtbo-y += waipio/waipio-mmrm.dtbo
dtbo-y += waipio/waipio-mmrm-test.dtbo
dtbo-y += waipio/waipio-v2-mmrm.dtbo
dtbo-y += waipio/waipio-v2-mmrm-test.dtbo
endif

always-y	:= $(dtb-y) $(dtbo-y)
subdir-y	:= $(dts-dirs)
clean-files	:= *.dtb *.dtbo
