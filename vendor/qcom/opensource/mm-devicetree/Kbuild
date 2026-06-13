ifeq ($(CONFIG_ARCH_SUN), y)
	dtbo-y += hw_fence/sun-hw-fence.dtbo \
		sun-mm-cdp-kiwi-overlay.dtbo \
		sun-mm-cdp-kiwi-v8-overlay.dtbo \
		sun-mm-cdp-nfc-overlay.dtbo \
		sun-mm-cdp-overlay.dtbo \
		sun-mm-cdp-v8-overlay.dtbo \
		sun-mm-mtp-kiwi-overlay.dtbo \
		sun-mm-mtp-kiwi-v8-overlay.dtbo \
		sun-mm-mtp-nfc-overlay.dtbo \
		sun-mm-mtp-overlay.dtbo \
		sun-mm-mtp-v8-overlay.dtbo \
		sun-mm-mtp-qmp1000-overlay.dtbo \
		sun-mm-mtp-qmp1000-v8-overlay.dtbo \
		sun-mm-qrd-sku1-overlay.dtbo \
		sun-mm-qrd-sku1-v8-overlay.dtbo \
		sun-mm-qrd-sku2-v8-overlay.dtbo \
		sun-mm-rumi-overlay.dtbo \
		sun-mm-rcm-overlay.dtbo \
		sun-mm-atp-overlay.dtbo \
		sun-mm-cdp-ganges-nodisplay-overlay.dtbo \
		sun-mm-mtp-3-5mm-overlay.dtbo \
		sun-mm-rcm-kiwi-overlay.dtbo \
		sun-mm-rcm-kiwi-v8-overlay.dtbo \
		sun-mm-rcm-v8-overlay.dtbo \
		sun-mm-cdp-no-display-overlay.dtbo
endif

always-y    := $(dtb-y) $(dtbo-y)
subdir-y    := $(dts-dirs)
clean-files    := *.dtb *.dtbo
