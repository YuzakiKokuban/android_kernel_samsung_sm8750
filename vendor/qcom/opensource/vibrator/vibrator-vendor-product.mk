TARGET_DISABLE_VIBRATOR := false
ifeq ($(TARGET_USES_QMAA),true)
ifneq ($(TARGET_USES_QMAA_OVERRIDE_VIBRATOR),true)

TARGET_DISABLE_VIBRATOR := true

endif #TARGET_USES_QMAA_OVERRIDE_VIBRATOR
endif #TARGET_USES_QMAA

ifneq ($(TARGET_DISABLE_VIBRATOR),true)
QTI_VIBRATOR_HAL_SERVICE := \
      vendor.qti.hardware.vibrator.service

PRODUCT_PACKAGES += $(QTI_VIBRATOR_HAL_SERVICE)

PRODUCT_COPY_FILES += \
      vendor/qcom/opensource/vibrator/excluded-input-devices.xml:vendor/etc/excluded-input-devices.xml

PRODUCT_COPY_FILES += \
      vendor/qcom/opensource/vibrator/aidl/HapticsPolicy.xml:vendor/etc/HapticsPolicy.xml

endif
