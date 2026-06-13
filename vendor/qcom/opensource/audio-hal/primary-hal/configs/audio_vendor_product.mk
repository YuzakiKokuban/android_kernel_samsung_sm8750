#Audio product definitions 
include vendor/qcom/opensource/audio-hal/primary-hal/configs/audio-generic-modules.mk
PRODUCT_PACKAGES += $(AUDIO_GENERIC_MODULES)

PRODUCT_PACKAGES_DEBUG += $(MM_AUDIO_DBG)

#----------------------------------------------------------------------
# audio specific
#----------------------------------------------------------------------
TARGET_USES_AOSP := false
TARGET_USES_AOSP_FOR_AUDIO := false

ifeq ($(TARGET_USES_QMAA_OVERRIDE_AUDIO), false)
ifeq ($(TARGET_USES_QMAA),true)
AUDIO_USE_STUB_HAL := true
TARGET_USES_AOSP_FOR_AUDIO := true
endif
endif
ifeq ($(AUDIO_USE_STUB_HAL), true)
-include $(TOPDIR)vendor/qcom/opensource/audio-hal/primary-hal/configs/common/default.mk
else
-include $(TOPDIR)vendor/qcom/opensource/audio-hal/primary-hal/configs/$(TARGET_BOARD_PLATFORM)/$(TARGET_BOARD_PLATFORM).mk
endif

$(warning audio check QC_HWASAN: $(QC_HWASAN) sanitize_target $(SANITIZE_TARGET))
$(call add_soong_config_namespace,vendor_audio_hwasan_config)
ifneq ($(filter audio, $(QC_HWASAN)),)
$(warning audio hwasan enabled at module level)
AUDIO_FEATURE_USE_HWASAN_ARTIFACTS := true
PRODUCT_HWASAN_INCLUDE_PATHS += \
    vendor/qcom/opensource/audio-hal \
    vendor/qcom/opensource/pal \
    vendor/qcom/opensource/agm
endif

# Pro Audio feature
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.audio.pro.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.audio.pro.xml

SOONG_CONFIG_qtiaudio_var00 := false
SOONG_CONFIG_qtiaudio_var11 := false
SOONG_CONFIG_qtiaudio_var22 := false
SOONG_CONFIG_qtiaudio_hwasan := false

ifneq ($(BUILD_AUDIO_TECHPACK_SOURCE), true)
    SOONG_CONFIG_qtiaudio_var00 := true
    SOONG_CONFIG_qtiaudio_var11 := true
    SOONG_CONFIG_qtiaudio_var22 := true
endif
ifeq (,$(wildcard $(QCPATH)/mm-audio-noship))
    SOONG_CONFIG_qtiaudio_var11 := true
endif
ifeq (,$(wildcard $(QCPATH)/mm-audio))
    SOONG_CONFIG_qtiaudio_var22 := true
endif

ifneq ($(filter hwaddress,$(SANITIZE_TARGET)),)
$(warning audio hwasan enabled at target level)
AUDIO_FEATURE_USE_HWASAN_ARTIFACTS := true
SOONG_CONFIG_qtiaudio_hwasan := true
endif

# this feature flag is only set when hwasan is enabled (local or global)
ifeq ($(AUDIO_FEATURE_USE_HWASAN_ARTIFACTS), true)
$(warning audio use hwasan artifacts)
$(call add_soong_config_var_value,vendor_audio_hwasan_config,use_hwasan,true)
else
$(call add_soong_config_var_value,vendor_audio_hwasan_config,use_hwasan,false)
endif
