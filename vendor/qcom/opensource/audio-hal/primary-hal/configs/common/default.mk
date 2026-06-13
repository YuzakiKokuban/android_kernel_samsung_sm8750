# This configuration is used when audio runs in stub/QMAA mode using AIDL Hals.

$(warning use audio in stub mode)

# AudioPolicyConfigs
APM_CONFIG_SRC_PATH := frameworks/av/services/audiopolicy/config
APM_CONFIG_DST_PATH := $(TARGET_COPY_OUT_VENDOR)/etc/audio

PRODUCT_COPY_FILES += \
    vendor/qcom/opensource/audio-hal/primary-hal/configs/common/audio_policy_configuration_stub.xml:$(APM_CONFIG_DST_PATH)/audio_policy_configuration.xml

PRODUCT_COPY_FILES += \
    $(APM_CONFIG_SRC_PATH)/primary_audio_policy_configuration_7_0.xml:$(APM_CONFIG_DST_PATH)/primary_audio_policy_configuration.xml \
    $(APM_CONFIG_SRC_PATH)/bluetooth_audio_policy_configuration_7_0.xml:$(APM_CONFIG_DST_PATH)/bluetooth_audio_policy_configuration.xml \
    $(APM_CONFIG_SRC_PATH)/r_submix_audio_policy_configuration.xml:$(APM_CONFIG_DST_PATH)/r_submix_audio_policy_configuration.xml \
    $(APM_CONFIG_SRC_PATH)/usb_audio_policy_configuration.xml:$(APM_CONFIG_DST_PATH)/usb_audio_policy_configuration.xml \
    $(APM_CONFIG_SRC_PATH)/stub_audio_policy_configuration.xml:$(APM_CONFIG_DST_PATH)/stub_audio_policy_configuration.xml \
    $(APM_CONFIG_SRC_PATH)/default_volume_tables.xml:$(APM_CONFIG_DST_PATH)/default_volume_tables.xml \
    $(APM_CONFIG_SRC_PATH)/audio_policy_volumes.xml:$(APM_CONFIG_DST_PATH)/audio_policy_volumes.xml

# Copy AudioEffects config
PRODUCT_COPY_FILES += \
    hardware/interfaces/audio/aidl/default/audio_effects_config.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio_effects_config.xml

# Add AIDL packages for stub mode
PRODUCT_PACKAGES += \
    audiohalservice.qti \
    libaudiocorehal.default \
    libaudioeffecthal.qti

BUILD_AUDIO_TECHPACK_SOURCE := true
