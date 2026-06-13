# Audio product definitions
include vendor/qcom/opensource/audio-hal/primary-hal/configs/anorak/audio-modules.mk
PRODUCT_PACKAGES += $(AUDIO_MODULES)

#BOARD_USES_GENERIC_AUDIO := true
#
#AUDIO_FEATURE_FLAGS
ifeq ($(TARGET_USES_QMAA_OVERRIDE_AUDIO), false)
ifeq ($(TARGET_USES_QMAA),true)
AUDIO_USE_STUB_HAL := true
endif
endif

ifneq ($(AUDIO_USE_STUB_HAL), true)
BOARD_USES_ALSA_AUDIO := true
TARGET_USES_AOSP_FOR_AUDIO := false

ifneq ($(TARGET_USES_AOSP_FOR_AUDIO), true)
USE_CUSTOM_AUDIO_POLICY := 1
AUDIO_FEATURE_QSSI_COMPLIANCE := true
AUDIO_FEATURE_ENABLED_COMPRESS_CAPTURE := false
AUDIO_FEATURE_ENABLED_COMPRESS_INPUT := true
AUDIO_FEATURE_ENABLED_CONCURRENT_CAPTURE := true
AUDIO_FEATURE_ENABLED_COMPRESS_VOIP := false
AUDIO_FEATURE_ENABLED_DYNAMIC_ECNS := true
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
AUDIO_FEATURE_ENABLED_MPEGH_SW_DECODER := true
AUDIO_FEATURE_ENABLED_PROXY_DEVICE := true
AUDIO_FEATURE_ENABLED_SSR := true
AUDIO_FEATURE_ENABLED_DTS_EAGLE := false
AUDIO_FEATURE_ENABLED_PAL_HIDL := true
#AUDIO_FEATURE_ENABLED_AGM_HIDL := true
AUDIO_FEATURE_ENABLED_LSM_HIDL := true
BOARD_USES_SRS_TRUEMEDIA := false
DTS_CODEC_M_ := false
MM_AUDIO_ENABLED_SAFX := false
AUDIO_FEATURE_ENABLED_HW_ACCELERATED_EFFECTS := false
AUDIO_FEATURE_ENABLED_AUDIOSPHERE := true
AUDIO_FEATURE_ENABLED_USB_TUNNEL := true
AUDIO_FEATURE_ENABLED_A2DP_OFFLOAD := true
AUDIO_FEATURE_ENABLED_3D_AUDIO := true
AUDIO_FEATURE_ENABLED_AHAL_EXT := false
AUDIO_FEATURE_ENABLED_EXTENDED_COMPRESS_FORMAT := true
DOLBY_ENABLE := false
endif

AUDIO_FEATURE_ENABLED_DLKM := true
BOARD_SUPPORTS_SOUND_TRIGGER := true
BOARD_SUPPORTS_GCS := false
AUDIO_FEATURE_ENABLED_INSTANCE_ID := true
AUDIO_USE_DEEP_AS_PRIMARY_OUTPUT := false
AUDIO_FEATURE_ENABLED_VBAT_MONITOR := true
AUDIO_FEATURE_ENABLED_NT_PAUSE_TIMEOUT := true
AUDIO_FEATURE_ENABLED_ANC_HEADSET := true
AUDIO_FEATURE_ENABLED_CUSTOMSTEREO := true
AUDIO_FEATURE_ENABLED_FLUENCE := true
AUDIO_FEATURE_ENABLED_HDMI_EDID := true
AUDIO_FEATURE_ENABLED_HDMI_PASSTHROUGH := true
#AUDIO_FEATURE_ENABLED_KEEP_ALIVE := true
AUDIO_FEATURE_ENABLED_DISPLAY_PORT := true
AUDIO_FEATURE_ENABLED_DS2_DOLBY_DAP := false
AUDIO_FEATURE_ENABLED_HFP := true
AUDIO_FEATURE_ENABLED_INCALL_MUSIC := true
AUDIO_FEATURE_ENABLED_MULTI_VOICE_SESSIONS := true
AUDIO_FEATURE_ENABLED_KPI_OPTIMIZE := true
AUDIO_FEATURE_ENABLED_SPKR_PROTECTION := true
AUDIO_FEATURE_ENABLED_ACDB_LICENSE := false
AUDIO_FEATURE_ENABLED_DEV_ARBI := false
AUDIO_FEATURE_ENABLED_DYNAMIC_LOG := true
MM_AUDIO_ENABLED_FTM := true
TARGET_USES_QCOM_MM_AUDIO := true
AUDIO_FEATURE_ENABLED_SOURCE_TRACKING := true
AUDIO_FEATURE_ENABLED_GEF_SUPPORT := true
BOARD_SUPPORTS_QAHW := false
AUDIO_FEATURE_ENABLED_RAS := true
AUDIO_FEATURE_ENABLED_SND_MONITOR := true
AUDIO_FEATURE_ENABLED_USB_BURST_MODE := true
AUDIO_FEATURE_ENABLED_SVA_MULTI_STAGE := true
AUDIO_FEATURE_ENABLED_BATTERY_LISTENER := true
BUILD_AUDIO_TECHPACK_SOURCE := true
AUDIO_FEATURE_ENABLED_MCS := true
##AUDIO_FEATURE_FLAGS
BOARD_SUPPORTS_OPENSOURCE_STHAL := true

ifneq ($(strip $(TARGET_USES_RRO)), true)
#Audio Specific device overlays
DEVICE_PACKAGE_OVERLAYS += vendor/qcom/opensource/audio-hal/primary-hal/configs/common/overlay
endif
PRODUCT_PACKAGES += fai__2.7.5_0.0__3.0.0_0.0__3.1.1.0_0.0__3.2.0_0.0__eai_2.7_enpu_v3.pmd
PRODUCT_PACKAGES += fai__4.8.2_0.0__3.0.0_0.0__3.1.1.0_0.0__3.2.0_0.0__eai_2.7_enpu_v3.pmd
PRODUCT_PACKAGES += fai__2.6.3_0.0__3.0.0_0.0__3.1.1.0_0.0__3.2.0_0.0__eai_2.7_enpu_v3.pmd
PRODUCT_PACKAGES += fai__2.0.0_0.1__3.0.0_0.0__3.1.0_0.0__3.2.0_0.0__eai_2.7_enpu3.pmd
PRODUCT_PACKAGES += fai__2.6.3_0.0__3.0.0_0.0__3.1.1_0.0__3.2.0_0.1__eai_2.10_enpuv3.pmd
PRODUCT_PACKAGES += fai__2.7.5_0.0__3.0.0_0.0__3.1.1_0.0__3.2.0_0.1__eai_2.10_enpuv3.pmd
PRODUCT_PACKAGES += fai__2.7.8_0.0__3.0.0_0.0__3.1.1_0.0__3.2.0_0.1__eai_2.10_enpuv3.pmd

# Audio configuration xml's related to HALLIDAY
QCV_FAMILY_SKUS := anorak
DEVICE_SKU := anorak

CONFIG_PAL_SRC_DIR := vendor/qcom/opensource/pal/configs/anorak
CONFIG_HAL_SRC_DIR := vendor/qcom/opensource/audio-hal/primary-hal/configs/anorak
#CONFIG_SKU_OUT_DIR := $(TARGET_COPY_OUT_VENDOR)/etc/audio/sku_$(DEVICE_SKU)
CONFIG_SKU_OUT_DIR := $(TARGET_COPY_OUT_VENDOR)/etc/
PRODUCT_COPY_FILES += \
    $(CONFIG_HAL_SRC_DIR)/audio_effects.conf:$(CONFIG_SKU_OUT_DIR)/audio_effects.conf \
    $(CONFIG_HAL_SRC_DIR)/audio_effects.xml:$(CONFIG_SKU_OUT_DIR)/audio_effects.xml \
    $(CONFIG_HAL_SRC_DIR)/microphone_characteristics.xml:$(TARGET_COPY_OUT_VENDOR)/etc/microphone_characteristics.xml \
    $(CONFIG_PAL_SRC_DIR)/card-defs.xml:$(TARGET_COPY_OUT_VENDOR)/etc/card-defs.xml \
    $(CONFIG_PAL_SRC_DIR)/mixer_paths_anorak_qxr.xml:$(CONFIG_SKU_OUT_DIR)/mixer_paths_anorak_qxr.xml \
    $(CONFIG_PAL_SRC_DIR)/mixer_paths_anorak_idp.xml:$(CONFIG_SKU_OUT_DIR)/mixer_paths_anorak_idp.xml \
    $(CONFIG_PAL_SRC_DIR)/resourcemanager_anorak_qxr.xml:$(CONFIG_SKU_OUT_DIR)/resourcemanager_anorak_qxr.xml \
    $(CONFIG_PAL_SRC_DIR)/resourcemanager_anorak_idp.xml:$(CONFIG_SKU_OUT_DIR)/resourcemanager_anorak_idp.xml \
    $(CONFIG_PAL_SRC_DIR)/usecaseKvManager.xml:$(TARGET_COPY_OUT_VENDOR)/etc/usecaseKvManager.xml \
    vendor/qcom/opensource/audio-hal/primary-hal/configs/common/media_codecs_vendor_audio.xml:$(TARGET_COPY_OUT_VENDOR)/etc/media_codecs_vendor_audio.xml \
    frameworks/native/data/etc/android.hardware.audio.pro.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.audio.pro.xml \
    frameworks/native/data/etc/android.hardware.audio.low_latency.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.audio.low_latency.xml

PRODUCT_COPY_FILES += \
    $(TOPDIR)vendor/qcom/opensource/audio-hal/primary-hal/configs/common/audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio_policy_configuration.xml \
    $(TOPDIR)frameworks/av/services/audiopolicy/config/a2dp_audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/a2dp_audio_policy_configuration.xml \
    $(TOPDIR)frameworks/av/services/audiopolicy/config/audio_policy_volumes.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio_policy_volumes.xml \
    $(TOPDIR)frameworks/av/services/audiopolicy/config/default_volume_tables.xml:$(TARGET_COPY_OUT_VENDOR)/etc/default_volume_tables.xml \
    $(TOPDIR)frameworks/av/services/audiopolicy/config/r_submix_audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/r_submix_audio_policy_configuration.xml \
    $(TOPDIR)frameworks/av/services/audiopolicy/config/usb_audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/usb_audio_policy_configuration.xml \
    $(TOPDIR)vendor/qcom/opensource/audio-hal/primary-hal/configs/common/bluetooth_qti_audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/bluetooth_qti_audio_policy_configuration.xml \
    $(TOPDIR)vendor/qcom/opensource/audio-hal/primary-hal/configs/common/bluetooth_qti_hearing_aid_audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/bluetooth_qti_hearing_aid_audio_policy_configuration.xml

#XML Audio configuration files
PRODUCT_COPY_FILES += \
    $(CONFIG_HAL_SRC_DIR)/audio_policy_configuration.xml:$(CONFIG_SKU_OUT_DIR)/audio_policy_configuration.xml

PRODUCT_COPY_FILES += \
    $(TOPDIR)vendor/qcom/opensource/audio-hal/primary-hal/configs/common/codec2/media_codecs_c2_audio.xml:vendor/etc/media_codecs_c2_audio.xml \
    $(TOPDIR)vendor/qcom/opensource/audio-hal/primary-hal/configs/common/codec2/service/1.0/c2audio.vendor.base-arm.policy:vendor/etc/seccomp_policy/c2audio.vendor.base-arm.policy \
    $(TOPDIR)vendor/qcom/opensource/audio-hal/primary-hal/configs/common/codec2/service/1.0/c2audio.vendor.base-arm64.policy:vendor/etc/seccomp_policy/c2audio.vendor.base-arm64.policy \
    $(TOPDIR)vendor/qcom/opensource/audio-hal/primary-hal/configs/common/codec2/service/1.0/c2audio.vendor.ext-arm.policy:vendor/etc/seccomp_policy/c2audio.vendor.ext-arm.policy \
    $(TOPDIR)vendor/qcom/opensource/audio-hal/primary-hal/configs/common/codec2/service/1.0/c2audio.vendor.ext-arm64.policy:vendor/etc/seccomp_policy/c2audio.vendor.ext-arm64.policy

# Reduce client buffer size for fast audio output tracks
PRODUCT_PROPERTY_OVERRIDES += \
    af.fast_track_multiplier=1

# Reduce AF standby time for playback threads (except offload)
PRODUCT_PROPERTY_OVERRIDES += \
   ro.audio.flinger_standbytime_ms=2000

# Low latency audio buffer size in frames
PRODUCT_PROPERTY_OVERRIDES += \
    vendor.audio_hal.period_size=192

# period multiplier for low latency capture tracks
PRODUCT_PROPERTY_OVERRIDES += \
    vendor.audio.ull_record_period_multiplier=2

##Ambisonic Capture
PRODUCT_PROPERTY_OVERRIDES += \
persist.vendor.audio.ambisonic.capture=false \
persist.vendor.audio.ambisonic.auto.profile=false

PRODUCT_PROPERTY_OVERRIDES += \
persist.vendor.audio.apptype.multirec.enabled=false

##fluencetype can be "fluence" or "fluencepro" or "none"
PRODUCT_PROPERTY_OVERRIDES += \
ro.vendor.audio.sdk.fluencetype=none\
persist.vendor.audio.fluence.voicecall=true\
persist.vendor.audio.fluence.voicerec=false\
persist.vendor.audio.fluence.speaker=true\
persist.vendor.audio.fluence.tmic.enabled=false

#
#snapdragon value add features
#
PRODUCT_PROPERTY_OVERRIDES += \
ro.qc.sdk.audio.ssr=false

##fluencetype can be "fluence" or "fluencepro" or "none"
PRODUCT_PROPERTY_OVERRIDES += \
ro.qc.sdk.audio.fluencetype=none\
persist.audio.fluence.voicecall=true\
persist.audio.fluence.voicerec=false\
persist.audio.fluence.speaker=true

##speaker protection v3 switch and ADSP AFE API version
PRODUCT_PROPERTY_OVERRIDES += \
persist.vendor.audio.spv3.enable=true\
persist.vendor.audio.avs.afe_api_version=2

#disable tunnel encoding
PRODUCT_PROPERTY_OVERRIDES += \
vendor.audio.tunnel.encode=false

#Disable RAS Feature by default
PRODUCT_PROPERTY_OVERRIDES += \
persist.vendor.audio.ras.enabled=false

#Buffer size in kbytes for compress offload playback
PRODUCT_PROPERTY_OVERRIDES += \
vendor.audio.offload.buffer.size.kb=32

#Enable offload audio video playback by default
PRODUCT_PROPERTY_OVERRIDES += \
audio.offload.video=true

#Enable audio track offload by default
PRODUCT_PROPERTY_OVERRIDES += \
vendor.audio.offload.track.enable=true

#Enable music through deep buffer
PRODUCT_PROPERTY_OVERRIDES += \
audio.deep_buffer.media=true

#enable voice path for PCM VoIP by default
PRODUCT_PROPERTY_OVERRIDES += \
vendor.voice.path.for.pcm.voip=true

#Enable multi channel aac through offload
PRODUCT_PROPERTY_OVERRIDES += \
vendor.audio.offload.multiaac.enable=true

#Enable DS2, Hardbypass feature for Dolby
PRODUCT_PROPERTY_OVERRIDES += \
vendor.audio.dolby.ds2.enabled=false\
vendor.audio.dolby.ds2.hardbypass=false

#Disable Multiple offload sesison
PRODUCT_PROPERTY_OVERRIDES += \
vendor.audio.offload.multiple.enabled=false

#Disable Compress passthrough playback
PRODUCT_PROPERTY_OVERRIDES += \
vendor.audio.offload.passthrough=false

#Disable surround sound recording
PRODUCT_PROPERTY_OVERRIDES += \
ro.vendor.audio.sdk.ssr=false

#enable dsp gapless mode by default
PRODUCT_PROPERTY_OVERRIDES += \
vendor.audio.offload.gapless.enabled=true

#enable pbe effects
PRODUCT_PROPERTY_OVERRIDES += \
vendor.audio.safx.pbe.enabled=false

#parser input buffer size(256kb) in byte stream mode
PRODUCT_PROPERTY_OVERRIDES += \
vendor.audio.parser.ip.buffer.size=262144

#flac sw decoder 24 bit decode capability
PRODUCT_PROPERTY_OVERRIDES += \
vendor.audio.flac.sw.decoder.24bit=true

#timeout crash duration set to 20sec before system is ready.
#timeout duration updates to default timeout of 5sec once the system is ready.
PRODUCT_PROPERTY_OVERRIDES += \
vendor.audio.hal.boot.timeout.ms=20000

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

#audio becoming noisy intent broadcast delay
PRODUCT_PROPERTY_OVERRIDES += \
audio.sys.noisy.broadcast.delay=600

#offload pausetime out duration to 3 secs to inline with other outputs
PRODUCT_PROPERTY_OVERRIDES += \
audio.sys.offload.pstimeout.secs=3

#Set AudioFlinger client heap size
PRODUCT_PROPERTY_OVERRIDES += \
ro.af.client_heap_size_kbyte=7168

#Set HAL buffer size to samples equal to 3 ms
PRODUCT_PROPERTY_OVERRIDES += \
vendor.audio_hal.in_period_size=144

#Set HAL buffer size to 3 ms
PRODUCT_PROPERTY_OVERRIDES += \
vendor.audio_hal.period_multiplier=3

#ADM Buffering size in ms
PRODUCT_PROPERTY_OVERRIDES += \
vendor.audio.adm.buffering.ms=2

#enable headset calibration
PRODUCT_PROPERTY_OVERRIDES += \
vendor.audio.volume.headset.gain.depcal=true

#enable dualmic fluence for voice communication
PRODUCT_PROPERTY_OVERRIDES += \
persist.audio.fluence.voicecomm=true

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

USE_XML_AUDIO_POLICY_CONF := 1

#enable keytone FR
PRODUCT_PROPERTY_OVERRIDES += \
vendor.audio.hal.output.suspend.supported=true

#Enable AAudio MMAP/NOIRQ data path
#2 is AAUDIO_POLICY_AUTO so it will try MMAP then fallback to Legacy path
PRODUCT_PROPERTY_OVERRIDES += aaudio.mmap_policy=2
#Allow EXCLUSIVE then fall back to SHARED.
PRODUCT_PROPERTY_OVERRIDES += aaudio.mmap_exclusive_policy=2
PRODUCT_PROPERTY_OVERRIDES += aaudio.hw_burst_min_usec=2000


#enable mirror-link feature
PRODUCT_PROPERTY_OVERRIDES += \
vendor.audio.enable.mirrorlink=false

#enable voicecall speaker stereo
PRODUCT_PROPERTY_OVERRIDES += \
persist.vendor.audio.voicecall.speaker.stereo=true

#enable AAC frame ctl for A2DP sinks
PRODUCT_PROPERTY_OVERRIDES += \
persist.vendor.bt.aac_frm_ctl.enabled=true

#enable VBR frame ctl
PRODUCT_PROPERTY_OVERRIDES += \
persist.vendor.bt.aac_vbr_frm_ctl.enabled=true

#enable dedicated proxy for hearing aid
PRODUCT_PROPERTY_OVERRIDES += \
persist.vendor.audio.ha_proxy.enabled=true

#add dynamic feature flags here
PRODUCT_PROPERTY_OVERRIDES += \
vendor.audio.feature.a2dp_offload.enable=true \
vendor.audio.feature.afe_proxy.enable=true \
vendor.audio.feature.anc_headset.enable=false \
vendor.audio.feature.battery_listener.enable=true \
vendor.audio.feature.compr_cap.enable=false \
vendor.audio.feature.compress_in.enable=true \
vendor.audio.feature.compress_meta_data.enable=true \
vendor.audio.feature.compr_voip.enable=false \
vendor.audio.feature.concurrent_capture.enable=true \
vendor.audio.feature.custom_stereo.enable=true \
vendor.audio.feature.display_port.enable=true \
vendor.audio.feature.dsm_feedback.enable=false \
vendor.audio.feature.dynamic_ecns.enable=true \
vendor.audio.feature.ext_hw_plugin.enable=false \
vendor.audio.feature.external_dsp.enable=false \
vendor.audio.feature.external_speaker.enable=false \
vendor.audio.feature.external_speaker_tfa.enable=false \
vendor.audio.feature.fluence.enable=true \
vendor.audio.feature.fm.enable=true \
vendor.audio.feature.hdmi_edid.enable=true \
vendor.audio.feature.hdmi_passthrough.enable=true \
vendor.audio.feature.hfp.enable=true \
vendor.audio.feature.hifi_audio.enable=false \
vendor.audio.feature.hwdep_cal.enable=false \
vendor.audio.feature.incall_music.enable=true \
vendor.audio.feature.multi_voice_session.enable=true \
vendor.audio.feature.keep_alive.enable=true \
vendor.audio.feature.kpi_optimize.enable=true \
vendor.audio.feature.maxx_audio.enable=false \
vendor.audio.feature.ras.enable=true \
vendor.audio.feature.record_play_concurency.enable=false \
vendor.audio.feature.src_trkn.enable=true \
vendor.audio.feature.spkr_prot.enable=true \
vendor.audio.feature.ssrec.enable=true \
vendor.audio.feature.usb_offload.enable=true \
vendor.audio.feature.usb_offload_burst_mode.enable=true \
vendor.audio.feature.usb_offload_sidetone_volume.enable=false \
vendor.audio.feature.deepbuffer_as_primary.enable=false \
vendor.audio.feature.vbat.enable=true \
vendor.audio.feature.wsa.enable=false \
vendor.audio.feature.audiozoom.enable=false \
vendor.audio.feature.snd_mon.enable=true \
vendor.audio.feature.dmabuf.cma.memory.enable=true \
vendor.audio.hdr.record.enable=false


PRODUCT_PACKAGES_ENG += \
    VoicePrintTest \
    VoicePrintDemo

PRODUCT_PACKAGES_DEBUG += \
    AudioSettings

ifeq ($(strip $(AUDIO_FEATURE_ENABLED_DEV_ARBI)),true)
PRODUCT_PACKAGES_DEBUG += \
    libaudiodevarb
endif

ifeq ($(call is-vendor-board-platform,QCOM),true)
ifeq ($(call is-platform-sdk-version-at-least,28),true)
PRODUCT_PACKAGES_DEBUG += \
    libqti_resampler_headers \
    lib_soundmodel_headers \
    libqti_vraudio_headers
endif
ifeq ($(strip $(AUDIO_FEATURE_ENABLED_3D_AUDIO)),true)
PRODUCT_PACKAGES_DEBUG += \
    libvr_object_engine \
    libvr_amb_engine \
    libhoaeffects_csim
endif
endif

ifeq ($(strip $(BOARD_SUPPORTS_SOUND_TRIGGER)),true)
PRODUCT_PACKAGES_DEBUG += \
    libadpcmdec
endif

AUDIO_FEATURE_ENABLED_GKI := true
BUILD_AUDIO_TECHPACK_SOURCE := true
