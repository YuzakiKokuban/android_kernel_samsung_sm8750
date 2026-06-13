#BOARD_USES_GENERIC_AUDIO := true
#
#AUDIO_FEATURE_FLAGS
BOARD_USES_ALSA_AUDIO := true
TARGET_USES_AOSP_FOR_AUDIO := false
AUDIO_FEATURE_QSSI_COMPLIANCE := true

ifneq ($(TARGET_USES_AOSP_FOR_AUDIO), true)
ifeq ($(TARGET_FWK_SUPPORTS_FULL_VALUEADDS),true)
USE_CUSTOM_AUDIO_POLICY := 1
else
USE_CUSTOM_AUDIO_POLICY := 0
endif
AUDIO_FEATURE_ENABLED_COMPRESS_VOIP := false
AUDIO_FEATURE_ENABLED_EXTN_FORMATS := true
AUDIO_FEATURE_ENABLED_EXTN_FLAC_DECODER := true
AUDIO_FEATURE_ENABLED_EXTN_RESAMPLER := true
AUDIO_FEATURE_ENABLED_FM_POWER_OPT := true
AUDIO_FEATURE_ENABLED_HDMI_SPK := true
AUDIO_FEATURE_ENABLED_PCM_OFFLOAD := true
AUDIO_FEATURE_ENABLED_PCM_OFFLOAD_24 := true
AUDIO_FEATURE_ENABLED_FLAC_OFFLOAD := true
AUDIO_FEATURE_ENABLED_VORBIS_OFFLOAD := true
AUDIO_FEATURE_ENABLED_WMA_OFFLOAD := true
AUDIO_FEATURE_ENABLED_ALAC_OFFLOAD := true
AUDIO_FEATURE_ENABLED_APE_OFFLOAD := true
AUDIO_FEATURE_ENABLED_AAC_ADTS_OFFLOAD := true
AUDIO_FEATURE_ENABLED_PROXY_DEVICE := true
AUDIO_FEATURE_ENABLED_AUDIOSPHERE := true
AUDIO_FEATURE_ENABLED_3D_AUDIO := true
AUDIO_FEATURE_ENABLED_AHAL_EXT := false
DOLBY_ENABLE := false
endif

USE_XML_AUDIO_POLICY_CONF := 1
BOARD_SUPPORTS_SOUND_TRIGGER := true
#AUDIO_FEATURE_ENABLED_KEEP_ALIVE := true
AUDIO_FEATURE_ENABLED_DS2_DOLBY_DAP := false
TARGET_USES_QCOM_MM_AUDIO := true
AUDIO_FEATURE_ENABLED_SVA_MULTI_STAGE := true
##AUDIO_FEATURE_FLAGS

ifneq ($(strip $(TARGET_USES_RRO)), true)
#Audio Specific device overlays
DEVICE_PACKAGE_OVERLAYS += hardware/qcom/audio/configs/common/overlay
endif

ifneq ($(GENERIC_ODM_IMAGE),true)
# Reduce client buffer size for fast audio output tracks
PRODUCT_PRODUCT_PROPERTIES += \
af.fast_track_multiplier=1

#Enable offload audio video playback by default
PRODUCT_PRODUCT_PROPERTIES += \
audio.offload.video=true

#Enable music through deep buffer
PRODUCT_PRODUCT_PROPERTIES += \
audio.deep_buffer.media=true

#audio becoming noisy intent broadcast delay
PRODUCT_PRODUCT_PROPERTIES += \
audio.sys.noisy.broadcast.delay=500

#audio device switch mute latency factor for draining unmuted data
PRODUCT_PRODUCT_PROPERTIES += \
audio.sys.mute.latency.factor=2

#audio device switch mute latency to absorb routing activities
PRODUCT_PRODUCT_PROPERTIES += \
audio.sys.routing.latency=0

#offload minimum duration set to 30sec
PRODUCT_PRODUCT_PROPERTIES += \
audio.offload.min.duration.secs=30

#offload pausetime out duration to 3 secs to inline with other outputs
PRODUCT_PRODUCT_PROPERTIES += \
audio.sys.offload.pstimeout.secs=3

#Set AudioFlinger client heap size
PRODUCT_PRODUCT_PROPERTIES += \
ro.af.client_heap_size_kbyte=7168

#enable deep buffer
PRODUCT_PRODUCT_PROPERTIES += \
media.stagefright.audio.deep=false

endif
#guard for non generic_odm_image

# Enable AAudio MMAP/NOIRQ data path.
# 2 is AAUDIO_POLICY_AUTO so it will try MMAP then fallback to Legacy path.
PRODUCT_PRODUCT_PROPERTIES += aaudio.mmap_policy=2
# Allow EXCLUSIVE then fall back to SHARED.
PRODUCT_PRODUCT_PROPERTIES += aaudio.mmap_exclusive_policy=2
PRODUCT_PRODUCT_PROPERTIES += aaudio.hw_burst_min_usec=2000

# for HIDL related packages
PRODUCT_PACKAGES += \
    android.hardware.audio@4.0 \
    android.hardware.audio.common@4.0 \
    android.hardware.audio.common@4.0-util \
    android.hardware.audio.effect@4.0

# enable audio hidl hal 5.0
PRODUCT_PACKAGES += \
    android.hardware.audio@5.0 \
    android.hardware.audio.common@5.0 \
    android.hardware.audio.common@5.0-util \
    android.hardware.audio.effect@5.0 \

#enable audio hidl hal 6.0
PRODUCT_PACKAGES += \
    android.hardware.audio@6.0 \
    android.hardware.audio.common@6.0 \
    android.hardware.audio.common@6.0-util \
    android.hardware.audio.effect@6.0

PRODUCT_PACKAGES_ENG += \
    VoicePrintTest \
    VoicePrintDemo

PRODUCT_PACKAGES_DEBUG += \
    AudioSettings

PRODUCT_PACKAGES += libnbaio
