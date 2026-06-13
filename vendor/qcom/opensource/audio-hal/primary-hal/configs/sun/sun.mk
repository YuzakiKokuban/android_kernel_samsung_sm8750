# Audio product definitions
include vendor/qcom/opensource/audio-hal/primary-hal/configs/sun/audio-modules.mk
PRODUCT_PACKAGES += $(AUDIO_MODULES)
#AUDIO_FEATURE_FLAGS
ifeq ($(TARGET_USES_QMAA_OVERRIDE_AUDIO), false)
ifeq ($(TARGET_USES_QMAA),true)
AUDIO_USE_STUB_HAL := true
endif
endif

ifneq ($(AUDIO_USE_STUB_HAL), true)
TARGET_USES_AOSP_FOR_AUDIO := false

ifneq ($(TARGET_USES_AOSP_FOR_AUDIO), true)
AUDIO_FEATURE_ENABLED_AUDIOSPHERE := true
AUDIO_FEATURE_ENABLED_3D_AUDIO := true
AUDIO_FEATURE_ENABLED_SPATIAL_AUDIO := true
endif

AUDIO_FEATURE_ENABLED_DLKM := true
AUDIO_FEATURE_ENABLED_INSTANCE_ID := true
AUDIO_FEATURE_ENABLED_DYNAMIC_LOG := true
MM_AUDIO_ENABLED_FTM := true
TARGET_USES_QCOM_MM_AUDIO := true
AUDIO_FEATURE_ENABLED_SVA_MULTI_STAGE := true
BUILD_AUDIO_TECHPACK_SOURCE := true
AUDIO_FEATURE_ENABLED_MCS := false

ifeq (1,0)
############################################
#[samsung audio feature - unused
ifneq ($(strip $(TARGET_USES_RRO)), true)
#Audio Specific device overlays
DEVICE_PACKAGE_OVERLAYS += vendor/qcom/opensource/audio-hal/primary-hal/configs/common/overlay
endif
#samsung audio feature - unused]
############################################
endif

PRODUCT_PACKAGES += fai__2.7.5_0.0__3.0.0_0.0__3.1.1.0_0.0__3.2.0_0.0__eai_2.7_enpu_v3.pmd
PRODUCT_PACKAGES += fai__4.8.2_0.0__3.0.0_0.0__3.1.1.0_0.0__3.2.0_0.0__eai_2.7_enpu_v3.pmd
PRODUCT_PACKAGES += fai__2.6.3_0.0__3.0.0_0.0__3.1.1.0_0.0__3.2.0_0.0__eai_2.7_enpu_v3.pmd
PRODUCT_PACKAGES += fai__2.0.0_0.1__3.0.0_0.0__3.1.0_0.0__3.2.0_0.0__eai_2.7_enpu3.pmd
PRODUCT_PACKAGES += fai__2.6.5_0.0__3.0.0_0.0__3.1.0_0.0__3.2.0_0.0__eai_2.10_enpuv3.pmd
PRODUCT_PACKAGES += fai__2.7.8_0.0__3.0.0_0.0__3.1.0_0.0__3.2.0_0.0__eai_2.10_enpuv3.pmd
PRODUCT_PACKAGES += fai__4.8.3_0.0__3.0.0_0.0__3.1.0_0.0__3.2.0_0.0__eai_2.10_enpuv3.pmd
PRODUCT_PACKAGES += fai__3.0.0_0.0__eai_2.10_enpuv3.pmd
PRODUCT_PACKAGES += fai__2.6.5_0.0__3.0.0_0.0__3.1.1_0.0__3.2.0_0.0__eai_2.10_enpuv3.pmd
PRODUCT_PACKAGES += fai__2.7.8_0.0__3.0.0_0.0__3.1.1_0.0__3.2.0_0.0__eai_2.10_enpuv3.pmd
PRODUCT_PACKAGES += fai__4.8.4_0.0__3.0.0_0.0__3.1.1_0.0__3.2.0_0.0__eai_2.10_enpuv3.pmd
PRODUCT_PACKAGES += fai__2.6.3_0.0__3.0.0_0.0__3.1.1_0.0__3.2.0_0.1__eai_2.10_enpuv3.pmd
PRODUCT_PACKAGES += fai__2.7.5_0.0__3.0.0_0.0__3.1.1_0.0__3.2.0_0.1__eai_2.10_enpuv3.pmd
PRODUCT_PACKAGES += fai__2.7.8_0.0__3.0.0_0.0__3.1.1_0.0__3.2.0_0.1__eai_2.10_enpuv3.pmd
PRODUCT_PACKAGES += fai__4.8.4_0.0__3.0.0_0.0__3.1.1_0.0__3.2.0_0.1__eai_2.10_enpuv3.pmd
PRODUCT_PACKAGES += fai__2.6.3_0.0__3.0.0_0.0__3.1.1_0.0__3.2.0_0.1__eai_3.0_enpuv4.pmd
PRODUCT_PACKAGES += fai__2.7.5_0.0__3.0.0_0.0__3.1.1_0.0__3.2.0_0.1__eai_3.0_enpuv4.pmd
PRODUCT_PACKAGES += fai__2.7.8_0.0__3.0.0_0.0__3.1.1_0.0__3.2.0_0.1__eai_3.0_enpuv4.pmd
PRODUCT_PACKAGES += fai__4.8.4_0.0__3.0.0_0.0__3.1.1_0.0__3.2.0_0.1__eai_3.0_enpuv4.pmd
PRODUCT_PACKAGES += fai__2.7.2_0.0__3.0.0_0.0__3.1.1_0.0__3.2.0_0.1__eai_3.4_enpuv4.pmd
PRODUCT_PACKAGES += fai__2.7.6_0.0__3.0.0_0.0__3.1.1_0.0__3.2.0_0.1__eai_3.4_enpuv4.pmd
PRODUCT_PACKAGES += fai__2.7.7_0.0__3.0.0_0.0__3.1.1_0.0__3.2.0_0.1__eai_3.4_enpuv4.pmd
PRODUCT_PACKAGES += fai__2.9.0_1.0__3.0.0_0.0__3.1.1_0.0__3.2.0_0.1__eai_3.4_enpuv4.pmd
PRODUCT_PACKAGES += fai__2.9.2_0.0__3.0.0_0.0__3.1.1_0.0__3.2.0_0.1__eai_3.4_enpuv4.pmd
PRODUCT_PACKAGES += fai__4.8.4_0.0__3.0.0_0.0__3.1.1_0.0__3.2.0_0.1__eai_3.4_enpuv4.pmd
PRODUCT_PACKAGES += fai__8.0.2_0.0__3.0.0_0.0__3.1.1_0.0__3.2.0_0.1__eai_3.4_enpuv4.pmd
PRODUCT_PACKAGES += fai__2.9.2_1.0__3.0.0_0.0__3.1.1_0.0__3.2.0_0.0__eai_3.4_enpuv4.pmd
PRODUCT_PACKAGES += fai__3.0.0_0.0__eai_3.4_enpuv4.pmd
PRODUCT_PACKAGES += fai__2.7.2_0.0__3.0.0_0.0__3.1.2_0.0__3.2.0_0.1__eai_4.2_enpuv5.pmd
PRODUCT_PACKAGES += fai__2.9.0_1.0__3.0.0_0.0__3.1.2_0.0__3.2.0_0.1__eai_4.2_enpuv5.pmd
PRODUCT_PACKAGES += fai__2.9.2_1.0__3.0.0_0.0__3.1.2_0.0__3.2.0_0.1__eai_4.2_enpuv5.pmd
PRODUCT_PACKAGES += fai__4.8.4_0.0__3.0.0_0.0__3.1.2_0.0__3.2.0_0.1__eai_4.2_enpuv5.pmd
PRODUCT_PACKAGES += fai__8.0.2_0.0__3.0.0_0.0__3.1.2_0.0__3.2.0_0.1__eai_4.2_enpuv5.pmd
PRODUCT_PACKAGES += fai__3.0.0_0.0__eai_4.2_enpuv5.pmd
PRODUCT_PACKAGES += fai__2.7.2_0.0__3.0.0_0.0__3.1.2_0.0__3.2.0_0.1__eai_4.6_enpuv5.pmd
PRODUCT_PACKAGES += fai__2.9.0_1.0__3.0.0_0.0__3.1.2_0.0__3.2.0_0.1__eai_4.6_enpuv5.pmd
PRODUCT_PACKAGES += fai__2.9.2_1.0__3.0.0_0.0__3.1.2_0.0__3.2.0_0.1__eai_4.6_enpuv5.pmd
PRODUCT_PACKAGES += fai__4.8.4_0.0__3.0.0_0.0__3.1.2_0.0__3.2.0_0.1__eai_4.6_enpuv5.pmd
PRODUCT_PACKAGES += fai__8.0.2_0.0__3.0.0_0.0__3.1.2_0.0__3.2.0_0.1__eai_4.6_enpuv5.pmd
PRODUCT_PACKAGES += fai__8.0.3_0.0__3.0.0_0.0__3.1.2_0.0__3.2.0_0.1__eai_4.6_enpuv5.pmd
PRODUCT_PACKAGES += fai__3.0.0_0.0__eai_4.6_enpuv5.pmd

# Audio configuration xml's related to Lanai
QCV_FAMILY_SKUS := sun
DEVICE_SKU := sun
UV_WRAPPER2 := true

CONFIG_PAL_SRC_DIR := vendor/qcom/opensource/pal/configs/sun
CONFIG_HAL_SRC_DIR := vendor/qcom/opensource/audio-hal/primary-hal/configs/sun
CONFIG_SKU_OUT_DIR := $(TARGET_COPY_OUT_VENDOR)/etc/audio/sku_$(DEVICE_SKU)

PRODUCT_COPY_FILES += \
    $(CONFIG_HAL_SRC_DIR)/audio_effects.conf:$(CONFIG_SKU_OUT_DIR)/audio_effects.conf \
    $(CONFIG_HAL_SRC_DIR)/audio_effects.xml:$(CONFIG_SKU_OUT_DIR)/audio_effects.xml \
    $(CONFIG_HAL_SRC_DIR)/audio_effects_config.xml:$(CONFIG_SKU_OUT_DIR)/audio_effects_config.xml \
    $(CONFIG_HAL_SRC_DIR)/microphone_characteristics.xml:$(TARGET_COPY_OUT_VENDOR)/etc/microphone_characteristics.xml \
    $(CONFIG_HAL_SRC_DIR)/audconf/$(PROJECT_NAME)/mixer_paths.xml:$(CONFIG_SKU_OUT_DIR)/mixer_paths.xml \
    $(CONFIG_HAL_SRC_DIR)/audconf/$(PROJECT_NAME)/resourcemanager.xml:$(CONFIG_SKU_OUT_DIR)/resourcemanager.xml \
    $(CONFIG_PAL_SRC_DIR)/card-defs.xml:$(TARGET_COPY_OUT_VENDOR)/etc/card-defs.xml \
    $(CONFIG_PAL_SRC_DIR)/resourcemanager_sun_qrd.xml:$(CONFIG_SKU_OUT_DIR)/resourcemanager_sun_qrd.xml \
    $(CONFIG_PAL_SRC_DIR)/resourcemanager_sun_mtp.xml:$(CONFIG_SKU_OUT_DIR)/resourcemanager_sun_mtp.xml \
    $(CONFIG_PAL_SRC_DIR)/resourcemanager_sun_cdp.xml:$(CONFIG_SKU_OUT_DIR)/resourcemanager_sun_cdp.xml \
    $(CONFIG_PAL_SRC_DIR)/resourcemanager_sun_qrd_sku2.xml:$(CONFIG_SKU_OUT_DIR)/resourcemanager_sun_qrd_sku2.xml \
    $(CONFIG_HAL_SRC_DIR)/audconf/$(PROJECT_NAME)/usecaseKvManager.xml:$(TARGET_COPY_OUT_VENDOR)/etc/usecaseKvManager.xml \
    $(CONFIG_PAL_SRC_DIR)/Hapticsconfig.xml:$(TARGET_COPY_OUT_VENDOR)/etc/Hapticsconfig.xml \
    vendor/qcom/opensource/audio-hal/primary-hal/configs/common/media_codecs_vendor_audio.xml:$(TARGET_COPY_OUT_VENDOR)/etc/media_codecs_vendor_audio.xml \
    frameworks/native/data/etc/android.hardware.audio.pro.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.audio.pro.xml \
    frameworks/native/data/etc/android.hardware.audio.low_latency.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.audio.low_latency.xml

# Copy AudioEffects config
PRODUCT_COPY_FILES += \
    hardware/interfaces/audio/aidl/default/audio_effects_config.xml:$(CONFIG_SKU_OUT_DIR)/audio_effects_config_stub.xml

# include usb mixer file
ifneq ($(filter chn_open% usa% kor%, $(PROJECT_REGION)),)
PRODUCT_COPY_FILES += \
    vendor/qcom/opensource/audio-hal/primary-hal/configs/sun/audconf/$(PROJECT_NAME)/$(TARGET_PRODUCT)/mixer_usb_default.xml:$(TARGET_COPY_OUT_VENDOR)/etc/mixer_usb_default.xml
else
PRODUCT_COPY_FILES += \
    vendor/qcom/opensource/audio-hal/primary-hal/configs/sun/audconf/$(PROJECT_NAME)/mixer_usb_default.xml:$(TARGET_COPY_OUT_VENDOR)/etc/mixer_usb_default.xml
endif

ifneq ($(filter chn_open% usa%, $(PROJECT_REGION)),)
PRODUCT_COPY_FILES += \
    vendor/qcom/opensource/audio-hal/primary-hal/configs/sun/audconf/$(PROJECT_NAME)/$(TARGET_PRODUCT)/mixer_usb_gray.xml:$(TARGET_COPY_OUT_VENDOR)/etc/mixer_usb_gray.xml
else
PRODUCT_COPY_FILES += \
    vendor/qcom/opensource/audio-hal/primary-hal/configs/sun/audconf/$(PROJECT_NAME)/mixer_usb_gray.xml:$(TARGET_COPY_OUT_VENDOR)/etc/mixer_usb_gray.xml
endif

# include audio rc file
PRODUCT_COPY_FILES += \
    vendor/qcom/opensource/audio-hal/primary-hal/configs/sun/init.audio.samsung.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/init.audio.samsung.rc

ifeq (1,0)
####################c########################
#[samsung audio feature - unused
#XML Audio configuration files
ifneq ($(TARGET_USES_AOSP_FOR_AUDIO), true)
PRODUCT_COPY_FILES += \
    $(CONFIG_HAL_SRC_DIR)/audio_policy_configuration.xml:$(CONFIG_SKU_OUT_DIR)/audio_policy_configuration.xml

#Audio configuration xml's common to sun family
PRODUCT_COPY_FILES += \
$(foreach DEVICE_SKU, $(QCV_FAMILY_SKUS), \
    $(CONFIG_HAL_SRC_DIR)/audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio/sku_$(DEVICE_SKU)_qssi/audio_policy_configuration.xml)

PRODUCT_COPY_FILES += \
    $(CONFIG_HAL_SRC_DIR)/audio_module_config_primary.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio/audio_module_config_primary.xml
endif
#qcom original audio feature]
############################################
else
############################################
#[samsung audio feature - used
#Audio configuration xml's common to Pineapple family
PRODUCT_COPY_FILES += \
    $(CONFIG_HAL_SRC_DIR)/audio_policy_configuration.xml:$(CONFIG_SKU_OUT_DIR)/audio_policy_configuration.xml

#Audio configuration xml's common to sun family
PRODUCT_COPY_FILES += \
$(foreach DEVICE_SKU, $(QCV_FAMILY_SKUS), \
    $(CONFIG_HAL_SRC_DIR)/audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio/sku_$(DEVICE_SKU)_qssi/audio_policy_configuration.xml)

#add for sec vendor on gsi
PRODUCT_COPY_FILES += \
    $(CONFIG_HAL_SRC_DIR)/audio_module_config_primary_sec_on_gsi.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio/audio_module_config_primary_sec_on_gsi.xml

ifeq ($(SEC_FACTORY_BUILD),true)
PRODUCT_COPY_FILES += \
    $(CONFIG_HAL_SRC_DIR)/audio_module_config_primary_factory.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio/audio_module_config_primary.xml
else
PRODUCT_COPY_FILES += \
    $(CONFIG_HAL_SRC_DIR)/audio_module_config_primary_sec.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio/audio_module_config_primary.xml
    
#add audio_effects_config.xml
PRODUCT_COPY_FILES += \
    $(CONFIG_HAL_SRC_DIR)/audio_effects_config_sec.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio_effects_config_sec.xml
endif
#samsung audio feature]
############################################
endif

# XML config file for memory logger
PRODUCT_COPY_FILES += $(TOPDIR)vendor/qcom/opensource/audio-hal/primary-hal/configs/$(DEVICE_SKU)/mem_logger_config.xml:$(TARGET_COPY_OUT_VENDOR)/etc/mem_logger_config.xml

ifeq (1,0)
############################################
#[samsung audio feature - unused
PRODUCT_COPY_FILES += \
    $(TOPDIR)vendor/qcom/opensource/audio-hal/primary-hal/configs/common/audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio_policy_configuration.xml \
    $(TOPDIR)frameworks/av/services/audiopolicy/config/a2dp_audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/a2dp_audio_policy_configuration.xml \
    $(TOPDIR)frameworks/av/services/audiopolicy/config/audio_policy_volumes.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio_policy_volumes.xml \
    $(TOPDIR)frameworks/av/services/audiopolicy/config/default_volume_tables.xml:$(TARGET_COPY_OUT_VENDOR)/etc/default_volume_tables.xml \
    $(TOPDIR)frameworks/av/services/audiopolicy/config/r_submix_audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/r_submix_audio_policy_configuration.xml \
    $(TOPDIR)frameworks/av/services/audiopolicy/config/usb_audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/usb_audio_policy_configuration.xml \
    $(TOPDIR)frameworks/av/services/audiopolicy/config/stub_audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/stub_audio_policy_configuration.xml \
    $(TOPDIR)vendor/qcom/opensource/audio-hal/primary-hal/configs/common/bluetooth_qti_audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/bluetooth_qti_audio_policy_configuration.xml \
    $(TOPDIR)vendor/qcom/opensource/audio-hal/primary-hal/configs/common/bluetooth_qti_hearing_aid_audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/bluetooth_qti_hearing_aid_audio_policy_configuration.xml
#qcom original audio feature]
############################################
else
############################################
#[samsung audio feature - used
PRODUCT_COPY_FILES += \
    $(TOPDIR)frameworks/av/services/audiopolicy/config/a2dp_audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/a2dp_audio_policy_configuration.xml \
    $(TOPDIR)vendor/qcom/opensource/audio-hal/primary-hal/configs/common/bluetooth_qti_audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/bluetooth_qti_audio_policy_configuration.xml \
    $(TOPDIR)vendor/qcom/opensource/audio-hal/primary-hal/configs/common/bluetooth_qti_hearing_aid_audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/bluetooth_qti_hearing_aid_audio_policy_configuration.xml
#samsung audio feature]
############################################
endif

PRODUCT_COPY_FILES += \
    $(TOPDIR)vendor/qcom/opensource/audio-hal/primary-hal/configs/common/codec2/media_codecs_c2_audio.xml:vendor/etc/media_codecs_c2_audio.xml \
    $(TOPDIR)vendor/qcom/opensource/audio-hal/primary-hal/configs/common/codec2/service/1.0/c2audio.vendor.base-arm.policy:vendor/etc/seccomp_policy/c2audio.vendor.base-arm.policy \
    $(TOPDIR)vendor/qcom/opensource/audio-hal/primary-hal/configs/common/codec2/service/1.0/c2audio.vendor.base-arm64.policy:vendor/etc/seccomp_policy/c2audio.vendor.base-arm64.policy \
    $(TOPDIR)vendor/qcom/opensource/audio-hal/primary-hal/configs/common/codec2/service/1.0/c2audio.vendor.ext-arm.policy:vendor/etc/seccomp_policy/c2audio.vendor.ext-arm.policy \
    $(TOPDIR)vendor/qcom/opensource/audio-hal/primary-hal/configs/common/codec2/service/1.0/c2audio.vendor.ext-arm64.policy:vendor/etc/seccomp_policy/c2audio.vendor.ext-arm64.policy
PRODUCT_COPY_FILES += \
    $(CONFIG_HAL_SRC_DIR)/vendor_audio_interfaces.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio/vendor_audio_interfaces.xml

ifneq (,$(filter userdebug eng, $(TARGET_BUILD_VARIANT)))
PRODUCT_COPY_FILES += \
    $(TOPDIR)vendor/qcom/opensource/audio-hal/primary-hal/configs/common/init.qti.audio.debug.sh:$(TARGET_COPY_OUT_VENDOR)/bin/init.qti.audio.debug.sh
endif
# Reduce AF standby time for playback threads (except offload)
PRODUCT_PROPERTY_OVERRIDES += \
   ro.audio.flinger_standbytime_ms=2000

# timecheck timeout value for audio in ms
PRODUCT_PROPERTY_OVERRIDES += \
    vendor.audio_hal.timecheck_timeoutMS=8000

# Low latency audio buffer size in frames
PRODUCT_PROPERTY_OVERRIDES += \
    vendor.audio_hal.period_size=192

# period multiplier for low latency capture tracks
PRODUCT_PROPERTY_OVERRIDES += \
    vendor.audio.ull_record_period_multiplier=2
#Enable audio track offload by default
PRODUCT_PROPERTY_OVERRIDES += \
vendor.audio.offload.track.enable=true
#Disable Multiple offload sesison
PRODUCT_PROPERTY_OVERRIDES += \
vendor.audio.offload.multiple.enabled=false
#flac sw decoder 24 bit decode capability
PRODUCT_PROPERTY_OVERRIDES += \
vendor.audio.flac.sw.decoder.24bit=true

#split a2dp DSP supported encoder list
PRODUCT_PROPERTY_OVERRIDES += \
persist.vendor.bt.a2dp_offload_cap=sbc-aptx-aptxtws-aptxhd-aac-ldac

# A2DP offload support
PRODUCT_PROPERTY_OVERRIDES += \
ro.bluetooth.a2dp_offload.supported=true

# Disable A2DP offload
PRODUCT_PROPERTY_OVERRIDES += \
persist.bluetooth.a2dp_offload.disabled=false

# A2DP offload DSP supported encoder list
PRODUCT_PROPERTY_OVERRIDES += \
persist.bluetooth.a2dp_offload.cap=sbc-aac-aptx-aptxhd-ldac

#enable software decoders for ALAC and APE
PRODUCT_PROPERTY_OVERRIDES += \
vendor.audio.use.sw.alac.decoder=true
PRODUCT_PROPERTY_OVERRIDES += \
vendor.audio.use.sw.ape.decoder=true

#enable software decoder for MPEG-H
PRODUCT_PROPERTY_OVERRIDES += \
vendor.audio.use.sw.mpegh.decoder=true

#disable hw aac encoder by default in AR
PRODUCT_PROPERTY_OVERRIDES += \
vendor.audio.hw.aac.encoder=false

ifeq (1,0)
############################################
#[samsung audio feature - unused
# - refer /audio_config/common/default.mk
#ADM Buffering size in ms
PRODUCT_PROPERTY_OVERRIDES += \
vendor.audio.adm.buffering.ms=2
#samsung audio feature - unused]
############################################
endif

#enable headset calibration
PRODUCT_PROPERTY_OVERRIDES += \
vendor.audio.volume.headset.gain.depcal=true

#enable c2 based encoders/decoders as default NT decoders/encoders
PRODUCT_PROPERTY_OVERRIDES += \
vendor.audio.c2.preferred=true

#Enable dmaBuf heap usage by C2 components
PRODUCT_PROPERTY_OVERRIDES += \
debug.c2.use_dmabufheaps=1

#Enable C2 suspend
PRODUCT_PROPERTY_OVERRIDES += \
vendor.qc2audio.suspend.enabled=true

#Enable qc2 audio sw flac frame decode
PRODUCT_PROPERTY_OVERRIDES += \
vendor.qc2audio.per_frame.flac.dec.enabled=true


ifneq ($(GENERIC_ODM_IMAGE),true)
$(warning "Enabling codec2.0 SW only for non-generic odm build variant")
#Rank OMX SW codecs lower than OMX HW codecs
PRODUCT_PROPERTY_OVERRIDES += debug.stagefright.omx_default_rank=0
endif
endif
#enable keytone FR
PRODUCT_PROPERTY_OVERRIDES += \
vendor.audio.hal.output.suspend.supported=true
#enable AAC frame ctl for A2DP sinks
PRODUCT_PROPERTY_OVERRIDES += \
persist.vendor.bt.aac_frm_ctl.enabled=true

#enable VBR frame ctl
PRODUCT_PROPERTY_OVERRIDES += \
persist.vendor.bt.aac_vbr_frm_ctl.enabled=true
#add dynamic feature flags here
PRODUCT_PROPERTY_OVERRIDES += \
vendor.audio.feature.a2dp_offload.enable=true \
vendor.audio.feature.battery_listener.enable=true \
vendor.audio.feature.hfp.enable=true \
vendor.audio.feature.kpi_optimize.enable=true \
vendor.audio.feature.dmabuf.cma.memory.enable=false

AUDIO_FEATURE_ENABLED_GKI := true
BUILD_AUDIO_TECHPACK_SOURCE := true

#enable osal_panic
#PRODUCT_PROPERTY_OVERRIDES += \
#persist.vendor.audio.induce_crash=true

include vendor/qcom/opensource/audio-hal/primary-hal/configs/sun/audio-properties.mk

