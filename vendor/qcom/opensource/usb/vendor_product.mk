#
# Default property overrides for various function configurations
# These can be further overridden at runtime in init*.rc files as needed
#
PRODUCT_PROPERTY_OVERRIDES += vendor.usb.rndis.func.name=gsi
PRODUCT_PROPERTY_OVERRIDES += vendor.usb.rmnet.func.name=gsi
PRODUCT_PROPERTY_OVERRIDES += vendor.usb.rmnet.inst.name=rmnet
PRODUCT_PROPERTY_OVERRIDES += vendor.usb.dpl.inst.name=dpl

ifneq ($(filter blair monaco,$(TARGET_BOARD_PLATFORM)),)
  PRODUCT_PROPERTY_OVERRIDES += vendor.usb.controller=4e00000.dwc3
else
  PRODUCT_PROPERTY_OVERRIDES += vendor.usb.controller=a600000.dwc3
endif

# QDSS uses SW path on these targets
ifneq ($(filter lahaina taro blair kalama pineapple sun parrot monaco,$(TARGET_BOARD_PLATFORM)),)
  PRODUCT_PROPERTY_OVERRIDES += vendor.usb.qdss.inst.name=qdss_sw
else
  PRODUCT_PROPERTY_OVERRIDES += vendor.usb.qdss.inst.name=qdss
endif

ifeq ($(TARGET_HAS_DIAG_ROUTER),true)
  PRODUCT_PROPERTY_OVERRIDES += vendor.usb.diag.func.name=ffs
else
  PRODUCT_PROPERTY_OVERRIDES += vendor.usb.diag.func.name=diag
endif

ifneq ($(TARGET_KERNEL_VERSION),$(filter $(TARGET_KERNEL_VERSION),4.9 4.14 4.19))
  PRODUCT_PROPERTY_OVERRIDES += vendor.usb.use_ffs_mtp=1
  PRODUCT_PROPERTY_OVERRIDES += sys.usb.mtp.batchcancel=1
else
  PRODUCT_PROPERTY_OVERRIDES += vendor.usb.use_ffs_mtp=0
endif

ifneq ($(TARGET_KERNEL_VERSION),$(filter $(TARGET_KERNEL_VERSION),4.9 4.14))
  PRODUCT_PACKAGES += android.hardware.usb-service.qti
endif

PRODUCT_VENDOR_PROPERTIES += ro.usb.uvc.enabled=true

USB_USES_QMAA = $(TARGET_USES_QMAA)
ifeq ($(TARGET_USES_QMAA_OVERRIDE_USB),true)
       USB_USES_QMAA = false
endif

# USB init scripts
ifeq ($(USB_USES_QMAA),true)
  PRODUCT_PACKAGES += init.qti.usb.qmaa.rc
else
  PRODUCT_PACKAGES += init.qcom.usb.rc init.qcom.usb.sh

  #
  # USB Gadget HAL is enabled on newer targets and takes the place
  # of the init-based configfs rules for setting USB compositions
  #
#  ifneq ($(filter taro kalama pineapple sun parrot monaco,$(TARGET_BOARD_PLATFORM)),)
#    PRODUCT_PROPERTY_OVERRIDES += vendor.usb.use_gadget_hal=1
#    PRODUCT_PACKAGES += android.hardware.usb.gadget-service.qti
#    PRODUCT_PACKAGES += usb_compositions.conf
#  else
    PRODUCT_PROPERTY_OVERRIDES += vendor.usb.use_gadget_hal=0
#  endif

endif

# additional debugging on userdebug/eng builds
ifneq (,$(filter userdebug eng, $(TARGET_BUILD_VARIANT)))
  PRODUCT_PACKAGES += init.qti.usb.debug.sh
  PRODUCT_PACKAGES += init.qti.usb.debug.rc
endif
