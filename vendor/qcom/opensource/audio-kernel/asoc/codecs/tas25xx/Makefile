ccflags-y += -I$(srctree)/$(src)/inc/
ccflags-y += -I$(srctree)/sound/soc/codecs/uda20/driver/tas25xx_driver/inc/
ccflags-y += -I$(srctree)/sound/soc/mediatek/common/
ccflags-y += -I$(src)

obj-$(CONFIG_SND_SOC_TAS25XX) += snd-soc-tas25xx.o

snd-soc-tas25xx-$(CONFIG_SND_SOC_TAS25XX) := src/tas25xx-codec.o \
	src/tas25xx-regmap.o \
	src/tas25xx.o \
	src/tas25xx-logic.o \
	src/tas25xx-regbin-parser.o

ifneq ($(CONFIG_TAS25XX_ALGO),)
snd-soc-tas25xx-objs += algo/src/tas25xx-algo-intf.o \
	algo/src/tas25xx-calib.o \
	algo/src/tas25xx-algo-common.o
endif

ifneq ($(CONFIG_PLATFORM_MTK),)
snd-soc-tas25xx-objs += algo/platform/mtk/tas25xx-mtk-wrapper.o
endif

ifneq ($(CONFIG_TAS25XX_MISC),)
snd-soc-tas25xx-objs += src/tas25xx-misc.o
endif

ifneq ($(CONFIG_PLATFORM_QCOM),)
snd-soc-tas25xx-objs += algo/platform/qcom/tas25xx-algo-qdsp-intf.o
endif

ifneq ($(CONFIG_TISA_KBIN_INTF),)
snd-soc-tas25xx-objs += algo/src/tas25xx-algo-kbin-intf.o \
	algo/src/tas25xx-algo-bin-utils.o
endif

ifneq ($(CONFIG_TAS25XX_CALIB_VAL_BIG),)
snd-soc-tas25xx-objs += algo/src/tas25xx-calib-validation.o
endif

ifneq ($(CONFIG_TISA_SYSFS_INTF),)
snd-soc-tas25xx-objs += algo/src/tas25xx-sysfs-debugfs-utils.o \
	algo/src/smartpa-debug-common.o
endif

ifneq ($(CONFIG_TISA_BIN_INTF),)
snd-soc-tas25xx-objs += algo/src/tas25xx-algo-bin-intf.o
endif

ifneq ($(CONFIG_PLATFORM_EXYNOS),)
snd-soc-tas25xx-objs += algo/platform/exynos/tas25xx-algo-exynos-dsp-intf.o
snd-soc-tas25xx-objs += algo/platform/exynos/skinprot-sysfs-cb.o
endif
