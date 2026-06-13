# SPDX-License-Identifier: GPL-2.0
#
# Makefile for nfc devices
#
ifneq ($(SEC_BUILD_CONF_USE_ESE_TZ), false)
	ccflags-y += -DENABLE_ESE_SPI_SECURED
endif

# for combined driver
ifeq ($(CONFIG_NFC_NXP_COMBINED),y)
ifeq ($(CONFIG_SAMSUNG_NFC),m)
obj-$(CONFIG_SAMSUNG_NFC)		+= nfc_nxp_sec.o
nfc_nxp_sec-$(CONFIG_NFC_SN2XX_ESE_SUPPORT)	+= p73.o ese_reset.o
nfc_nxp_sec-$(CONFIG_SEC_NFC_LOGGER)		+= ../nfc_logger/nfc_logger.o
nfc_nxp_sec-$(CONFIG_NFC_SN2XX)		+= common.o common_ese.o i2c_drv.o
else
obj-$(CONFIG_NFC_SN2XX)	+= nfc.o
nfc-objs			+= common.o common_ese.o i2c_drv.o
obj-$(CONFIG_NFC_SN2XX_ESE_SUPPORT)	+= p73.o ese_reset.o
obj-$(CONFIG_SEC_NFC_LOGGER)	+= ../nfc_logger/nfc_logger.o
endif # CONFIG_SAMSUNG_NFC

else # !CONFIG_NFC_NXP_COMBINED
# for sn2xx only
ifeq ($(CONFIG_SAMSUNG_NFC),m)
obj-$(CONFIG_SAMSUNG_NFC)		+= nfc_sec.o
nfc_sec-$(CONFIG_NFC_SN2XX_ESE_SUPPORT)	+= p73.o ese_reset.o
nfc_sec-$(CONFIG_SEC_NFC_LOGGER)		+= nfc_logger/nfc_logger.o
nfc_sec-$(CONFIG_NFC_SN2XX)		+= common.o common_ese.o i2c_drv.o
else
obj-$(CONFIG_NFC_SN2XX)	+= nfc.o
nfc-objs			+= common.o common_ese.o i2c_drv.o
obj-$(CONFIG_NFC_SN2XX_ESE_SUPPORT)	+= p73.o ese_reset.o
obj-$(CONFIG_SEC_NFC_LOGGER)	+= nfc_logger/nfc_logger.o
endif # CONFIG_SAMSUNG_NFC


ifeq ($(CONFIG_SEC_STAR), m)
obj-$(CONFIG_SEC_STAR) = sec-star/sec_star.o

sec_star-y += \
                        sec-star/sec_star.o \
                        sec-star/protocol/ese_data.o \
                        sec-star/protocol/ese_iso7816_t1.o \
                        sec-star/hal/ese_i2c.o \
                        sec-star/hal/ese_spi.o \
                        sec-star/hal/ese_hal.o

sec_star-$(CONFIG_STAR_K250A) += sec-star/sec_k250a.o
else
obj-$(CONFIG_SEC_STAR) += sec-star/sec_star.o \
                        sec-star/protocol/ese_data.o \
                        sec-star/protocol/ese_memory.o \
                        sec-star/protocol/ese_iso7816_t1.o \
                        sec-star/hal/ese_i2c.o \
                        sec-star/hal/ese_spi.o \
                        sec-star/hal/ese_hal.o

obj-$(CONFIG_STAR_K250A) += sec-star/sec_k250a.o
endif # CONFIG_SEC_STAR
endif # CONFIG_NFC_NXP_COMBINED

ccflags-y 			+= -DRECOVERY_ENABLE -UDEBUG
