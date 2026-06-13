ifeq ($(CONFIG_ARCH_SUN),y)
dtbo-y += nxp/sun-nfc.dtbo \
          nxp/sun-nfc-cdp.dtbo \
          nxp/sun-nfc-mtp.dtbo \

dtbo-y += st/sun-nfc.dtbo \
          st/sun-nfc-mtp.dtbo \
          st/sun-nfc-cdp.dtbo \
          st/sun-nfc-qrd-sku1.dtbo \
          st/sun-nfc-qrd-sku1-v8.dtbo \
          st/sun-nfc-qrd-sku2-v8.dtbo \
          st/sun-nfc-atp.dtbo \
          st/sun-mtp-kiwi-v8.dtbo \
          st/sun-nfc-rcm.dtbo
endif

ifeq ($(CONFIG_ARCH_PARROT),y)
dtbo-y += nxp/parrot-nfc.dtbo \
          nxp/parrot-nfc-qrd.dtbo \
          nxp/parrot-nfc-idp.dtbo \
          nxp/parrot-nfc-atp.dtbo \
          nxp/parrot-nfc-idp-wcn6755.dtbo
endif

ifeq ($(CONFIG_ARCH_RAVELIN),y)
dtbo-y += nxp/ravelin-nfc.dtbo \
          nxp/ravelin-nfc-qrd.dtbo \
          nxp/ravelin-nfc-idp.dtbo \
          nxp/ravelin-nfc-atp.dtbo
endif

always-y        := $(dtb-y) $(dtbo-y)
subdir-y        := $(dts-dirs)
clean-files     := *.dtb *.dtbo
