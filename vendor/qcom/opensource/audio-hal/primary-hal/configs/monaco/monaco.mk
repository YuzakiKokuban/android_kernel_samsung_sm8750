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

ifeq ($(TARGET_SUPPORTS_WEAR_ANDROID), true)
AUDIO_FEATURE_ENABLED_CODEC_2_0 := true
endif

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
ifeq ($(AUDIO_FEATURE_ENABLED_CODEC_2_0), true)
AUDIO_FEATURE_ENABLED_PAL_HIDL := true
endif
BOARD_USES_SRS_TRUEMEDIA := false
DTS_CODEC_M_ := false
MM_AUDIO_ENABLED_SAFX := true
AUDIO_FEATURE_ENABLED_HW_ACCELERATED_EFFECTS := false
AUDIO_FEATURE_ENABLED_AUDIOSPHERE := true
AUDIO_FEATURE_ENABLED_USB_TUNNEL := true
AUDIO_FEATURE_ENABLED_A2DP_OFFLOAD := true
ifeq ($(filter R% r%,$(TARGET_PLATFORM_VERSION)),)
AUDIO_FEATURE_ENABLED_3D_AUDIO := true
endif
AUDIO_FEATURE_ENABLED_AHAL_EXT := true
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
ifeq ($(TARGET_SUPPORTS_WEAR_AON),true)
AUDIO_FEATURE_ENABLE_BT_A2DP_LPI := true
AUDIO_FEATURE_ENABLED_MCS := true
endif
TARGET_USES_QTI_TINYCOMPRESS := true
##AUDIO_FEATURE_FLAGS
#AGM
AUDIO_AGM := libagmclient
AUDIO_AGM += libagmservice
AUDIO_AGM += vendor.qti.hardware.AGMIPC@1.0-impl
AUDIO_AGM += vendor.qti.hardware.AGMIPC@1.0-service
AUDIO_AGM += vendor.qti.hardware.AGMIPC@1.0-service.rc
AUDIO_AGM += libagm
AUDIO_AGM += agmplay
AUDIO_AGM += agmcap
AUDIO_AGM += libagmmixer
AUDIO_AGM += agmcompressplay
AUDIO_AGM += libagm_mixer_plugin
AUDIO_AGM += libagm_pcm_plugin
AUDIO_AGM += libagm_compress_plugin
AUDIO_AGM += agmcompresscap
AUDIO_AGM += agmvoiceui
AUDIO_AGM += agmhostless

#PAL Module
AUDIO_PAL := libar-pal
AUDIO_PAL += catf
AUDIO_PAL += libaudiochargerlistener
AUDIO_PAL += lib_bt_bundle
AUDIO_PAL += lib_bt_aptx
AUDIO_PAL += lib_bt_ble
BOARD_SUPPORTS_OPENSOURCE_STHAL := true

AUDIO_HARDWARE += audio.usb.default
AUDIO_HARDWARE += audio.r_submix.default
AUDIO_HARDWARE += audio.primary.monaco
AUDIO_HARDWARE += libhfp_pal

#HAL Wrapper
AUDIO_WRAPPER := libqahw
AUDIO_WRAPPER += libqahwwrapper

ifeq ($(AUDIO_FEATURE_ENABLED_CODEC_2_0), true)
#PAL Service
AUDIO_PAL += libpalclient
AUDIO_PAL += vendor.qti.hardware.pal@1.0-impl

# C2 Audio
AUDIO_C2 := libqc2audio_base
AUDIO_C2 += libqc2audio_utils
AUDIO_C2 += libqc2audio_platform
AUDIO_C2 += libqc2audio_core
AUDIO_C2 += libqc2audio_basecodec
AUDIO_C2 += libqc2audio_hooks
AUDIO_C2 += libqc2audio_swaudiocodec
AUDIO_C2 += libqc2audio_swaudiocodec_data_common
AUDIO_C2 += libqc2audio_hwaudiocodec
AUDIO_C2 += libqc2audio_hwaudiocodec_data_common
AUDIO_C2 += vendor.qti.media.c2audio@1.0-service
AUDIO_C2 += qc2audio_test
AUDIO_C2 += libEvrcSwCodec
AUDIO_C2 += libQcelp13SwCodec
endif

#HAL Test app
AUDIO_HAL_TEST_APPS := hal_play_test
AUDIO_HAL_TEST_APPS += hal_rec_test

PRODUCT_PACKAGES += $(AUDIO_HARDWARE)
PRODUCT_PACKAGES += $(AUDIO_WRAPPER)
PRODUCT_PACKAGES += $(AUDIO_HAL_TEST_APPS)
PRODUCT_PACKAGES += ftm_test_config
PRODUCT_PACKAGES += ftm_test_config_monaco-idp-snd-card

PRODUCT_PACKAGES += IDP_acdb_cal_monaco_slate.acdb
PRODUCT_PACKAGES += IDP_workspaceFileXml_monaco_slate.qwsp
PRODUCT_PACKAGES += IDP_acdb_cal_monaco_slate_amic.acdb
PRODUCT_PACKAGES += IDP_workspaceFileXml_monaco_slate_amic.qwsp
PRODUCT_PACKAGES += IDP_acdb_cal_monaco_slate_wsa.acdb
PRODUCT_PACKAGES += IDP_workspaceFileXml_monaco_slate_wsa.qwsp
PRODUCT_PACKAGES += fai__2.6.0_0.0__3.0.0_0.0__eai_1.10.pmd
PRODUCT_PACKAGES += fai__3.0.0_0.0__eai_1.10.pmd

PRODUCT_PACKAGES += IDP_acdb_cal_monaco.acdb
PRODUCT_PACKAGES += IDP_workspaceFileXml_monaco.qwsp
PRODUCT_PACKAGES += IDP_acdb_cal_monaco_amic.acdb
PRODUCT_PACKAGES += IDP_workspaceFileXml_monaco_amic.qwsp
PRODUCT_PACKAGES += IDP_acdb_cal_monaco_wsa.acdb
PRODUCT_PACKAGES += IDP_workspaceFileXml_monaco_wsa.qwsp

PRODUCT_PACKAGES += mm-audio-ftm
PRODUCT_PACKAGES += libvui_dmgr
PRODUCT_PACKAGES += libvui_dmgr_client
PRODUCT_PACKAGES += qsap_voiceui
PRODUCT_PACKAGES += qsap_voiceui.policy

ifeq ($(strip $(AUDIO_FEATURE_ENABLED_MCS)), true)
 PRODUCT_PACKAGES += libmcs
endif

ifeq ($(AUDIO_FEATURE_ENABLED_DLKM),true)
BOARD_VENDOR_KERNEL_MODULES += \
    $(KERNEL_MODULES_OUT)/spf_core_dlkm.ko \
    $(KERNEL_MODULES_OUT)/audio_pkt_dlkm.ko \
    $(KERNEL_MODULES_OUT)/gpr_dlkm.ko \
    $(KERNEL_MODULES_OUT)/audio_prm_dlkm.ko \
    $(KERNEL_MODULES_OUT)/audpkt_ion_dlkm.ko \
    $(KERNEL_MODULES_OUT)/q6_pdr_dlkm.ko \
    $(KERNEL_MODULES_OUT)/q6_notifier_dlkm.ko \
    $(KERNEL_MODULES_OUT)/adsp_loader_dlkm.ko \
    $(KERNEL_MODULES_OUT)/pinctrl_lpi_dlkm.ko \
    $(KERNEL_MODULES_OUT)/swr_dlkm.ko \
    $(KERNEL_MODULES_OUT)/wcd_core_dlkm.ko \
    $(KERNEL_MODULES_OUT)/swr_ctrl_dlkm.ko \
    $(KERNEL_MODULES_OUT)/stub_dlkm.ko \
    $(KERNEL_MODULES_OUT)/wcd9xxx_dlkm.ko \
    $(KERNEL_MODULES_OUT)/bolero_cdc_dlkm.ko \
    $(KERNEL_MODULES_OUT)/va_macro_dlkm.ko \
    $(KERNEL_MODULES_OUT)/rx_macro_dlkm.ko \
    $(KERNEL_MODULES_OUT)/tx_macro_dlkm.ko \
    $(KERNEL_MODULES_OUT)/pmw5100-spmi_dlkm.ko \
    $(KERNEL_MODULES_OUT)/besbev_dlkm.ko \
    $(KERNEL_MODULES_OUT)/besbev-slave_dlkm.ko \
    $(KERNEL_MODULES_OUT)/wsa883x_dlkm.ko \
    $(KERNEL_MODULES_OUT)/machine_dlkm.ko\
    $(KERNEL_MODULES_OUT)/snd_event_dlkm.ko
endif

ifeq ($(AUDIO_FEATURE_ENABLED_DLKM),true)
ifeq ($(TARGET_SUPPORTS_WEAR_AON),true)
BOARD_VENDOR_KERNEL_MODULES += \
    $(KERNEL_MODULES_OUT)/cc_dlkm.ko \
    $(KERNEL_MODULES_OUT)/audio_cc_ipc_dlkm.ko
endif
endif

#Audio DLKM
AUDIO_DLKM := spf_core_dlkm.ko
AUDIO_DLKM += audio_pkt_dlkm.ko
AUDIO_DLKM += gpr_dlkm.ko
AUDIO_DLKM += audio_prm_dlkm.ko
AUDIO_DLKM += audpkt_ion_dlkm.ko
AUDIO_DLKM += q6_pdr_dlkm.ko
AUDIO_DLKM += q6_notifier_dlkm.ko
AUDIO_DLKM += adsp_loader_dlkm.ko
AUDIO_DLKM += pinctrl_lpi_dlkm.ko
AUDIO_DLKM += swr_dlkm.ko
AUDIO_DLKM += wcd_core_dlkm.ko
AUDIO_DLKM += swr_ctrl_dlkm.ko
AUDIO_DLKM += stub_dlkm.ko
AUDIO_DLKM += wcd9xxx_dlkm.ko
AUDIO_DLKM += bolero_cdc_dlkm.ko
AUDIO_DLKM += va_macro_dlkm.ko
AUDIO_DLKM += rx_macro_dlkm.ko
AUDIO_DLKM += tx_macro_dlkm.ko
AUDIO_DLKM += pmw5100-spmi_dlkm.ko
AUDIO_DLKM += besbev_dlkm.ko
AUDIO_DLKM += besbev-slave_dlkm.ko
AUDIO_DLKM += wsa883x_dlkm.ko
AUDIO_DLKM += machine_dlkm.ko
AUDIO_DLKM += snd_event_dlkm.ko
ifeq ($(TARGET_SUPPORTS_WEAR_AON),true)
AUDIO_DLKM += cc_dlkm.ko
AUDIO_DLKM += audio_cc_ipc_dlkm.ko
endif

PRODUCT_PACKAGES += $(AUDIO_DLKM)

DEVICE_SKU := monaco

CONFIG_PAL_SRC_DIR := $(TOPDIR)$(BOARD_OPENSOURCE_DIR)/pal/configs/monaco
CONFIG_HAL_SRC_DIR := $(TOPDIR)$(BOARD_OPENSOURCE_DIR)/audio-hal/primary-hal/configs/monaco
CONFIG_HAL_COMMON_SRC_DIR := $(TOPDIR)$(BOARD_OPENSOURCE_DIR)/audio-hal/primary-hal/configs/common
CONFIG_SKU_OUT_DIR := $(TARGET_COPY_OUT_VENDOR)/etc/audio/sku_$(DEVICE_SKU)

ifneq ($(strip $(TARGET_USES_RRO)), true)
#Audio Specific device overlays
DEVICE_PACKAGE_OVERLAYS += $(CONFIG_HAL_COMMON_SRC_DIR)/overlay
endif
PRODUCT_PACKAGES += $(AUDIO_AGM)
PRODUCT_PACKAGES += $(AUDIO_PAL)
ifeq ($(AUDIO_FEATURE_ENABLED_CODEC_2_0), true)
PRODUCT_PACKAGES += $(AUDIO_C2)
endif

PRODUCT_COPY_FILES += \
    $(CONFIG_HAL_SRC_DIR)/audio_io_policy.conf:$(CONFIG_SKU_OUT_DIR)/audio_io_policy.conf \
    $(CONFIG_HAL_SRC_DIR)/audio_effects.conf:$(CONFIG_SKU_OUT_DIR)/audio_effects.conf \
    $(CONFIG_HAL_SRC_DIR)/audio_effects.xml:$(CONFIG_SKU_OUT_DIR)/audio_effects.xml \
    $(CONFIG_HAL_SRC_DIR)/mixer_paths_monaco_idp.xml:$(CONFIG_SKU_OUT_DIR)/mixer_paths_monaco_idp.xml \
    $(CONFIG_HAL_SRC_DIR)/mixer_paths_monaco_idp_amic.xml:$(CONFIG_SKU_OUT_DIR)/mixer_paths_monaco_idp_amic.xml \
    $(CONFIG_HAL_SRC_DIR)/mixer_paths_monaco_idp_wsa.xml:$(CONFIG_SKU_OUT_DIR)/mixer_paths_monaco_idp_wsa.xml \
    $(CONFIG_HAL_SRC_DIR)/mixer_paths_monaco_idp_slate.xml:$(CONFIG_SKU_OUT_DIR)/mixer_paths_monaco_idp_slate.xml \
    $(CONFIG_HAL_SRC_DIR)/mixer_paths_monaco_idp_slate_amic.xml:$(CONFIG_SKU_OUT_DIR)/mixer_paths_monaco_idp_slate_amic.xml \
    $(CONFIG_HAL_SRC_DIR)/mixer_paths_monaco_idp_slate_wsa.xml:$(CONFIG_SKU_OUT_DIR)/mixer_paths_monaco_idp_slate_wsa.xml \
    $(CONFIG_HAL_SRC_DIR)/audio_configs.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio_configs.xml \
    $(CONFIG_HAL_SRC_DIR)/audio_configs_stock.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio_configs_stock.xml \
    frameworks/native/data/etc/android.hardware.audio.pro.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.audio.pro.xml \
    $(CONFIG_PAL_SRC_DIR)/resourcemanager_monaco_idp.xml:$(CONFIG_SKU_OUT_DIR)/resourcemanager_monaco_idp.xml \
    $(CONFIG_PAL_SRC_DIR)/resourcemanager_monaco_idp_amic.xml:$(CONFIG_SKU_OUT_DIR)/resourcemanager_monaco_idp_amic.xml \
    $(CONFIG_PAL_SRC_DIR)/resourcemanager_monaco_idp_wsa.xml:$(CONFIG_SKU_OUT_DIR)/resourcemanager_monaco_idp_wsa.xml \
    $(CONFIG_PAL_SRC_DIR)/resourcemanager_monaco_idp_slate.xml:$(CONFIG_SKU_OUT_DIR)/resourcemanager_monaco_idp_slate.xml \
    $(CONFIG_PAL_SRC_DIR)/resourcemanager_monaco_idp_slate_amic.xml:$(CONFIG_SKU_OUT_DIR)/resourcemanager_monaco_idp_slate_amic.xml \
    $(CONFIG_PAL_SRC_DIR)/resourcemanager_monaco_idp_slate_wsa.xml:$(CONFIG_SKU_OUT_DIR)/resourcemanager_monaco_idp_slate_wsa.xml \
    $(CONFIG_PAL_SRC_DIR)/usecaseKvManager.xml:$(TARGET_COPY_OUT_VENDOR)/etc/usecaseKvManager.xml \
    $(CONFIG_HAL_SRC_DIR)/card-defs.xml:$(TARGET_COPY_OUT_VENDOR)/etc/card-defs.xml

ifeq ($(AUDIO_FEATURE_ENABLED_MCS),true)
PRODUCT_COPY_FILES += \
    $(CONFIG_HAL_SRC_DIR)/mcs_defs_monaco_idp_slate.xml:$(CONFIG_SKU_OUT_DIR)/mcs_defs_monaco_idp_slate.xml
endif

#XML Audio configuration files
ifeq ($(TARGET_SUPPORTS_WEAR_ANDROID), true)
PRODUCT_COPY_FILES += \
    $(CONFIG_HAL_SRC_DIR)/audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio/audio_policy_configuration.xml
endif
ifeq ($(TARGET_SUPPORTS_WEAR_OS), true)
PRODUCT_COPY_FILES += \
    $(CONFIG_HAL_SRC_DIR)/audio_policy_configuration_lw.xml:$(CONFIG_SKU_OUT_DIR)/audio_policy_configuration.xml
else
PRODUCT_COPY_FILES += \
    $(CONFIG_HAL_COMMON_SRC_DIR)/audio_policy_configuration.xml:$(CONFIG_SKU_OUT_DIR)/audio_policy_configuration.xml
endif
PRODUCT_COPY_FILES += \
    $(TOPDIR)frameworks/av/services/audiopolicy/config/a2dp_audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/a2dp_audio_policy_configuration.xml \
    $(TOPDIR)frameworks/av/services/audiopolicy/config/audio_policy_volumes.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio_policy_volumes.xml \
    $(TOPDIR)frameworks/av/services/audiopolicy/config/default_volume_tables.xml:$(TARGET_COPY_OUT_VENDOR)/etc/default_volume_tables.xml \
    $(TOPDIR)frameworks/av/services/audiopolicy/config/r_submix_audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/r_submix_audio_policy_configuration.xml \
    $(TOPDIR)frameworks/av/services/audiopolicy/config/usb_audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/usb_audio_policy_configuration.xml \
    $(CONFIG_HAL_COMMON_SRC_DIR)/bluetooth_qti_audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/bluetooth_qti_audio_policy_configuration.xml

# C2 Audio files
ifeq ($(AUDIO_FEATURE_ENABLED_CODEC_2_0), true)
PRODUCT_COPY_FILES += \
    $(CONFIG_HAL_SRC_DIR)/media_codecs_c2_audio.xml:vendor/etc/media_codecs_c2_audio.xml \
    $(CONFIG_HAL_COMMON_SRC_DIR)/media_codecs_vendor_audio.xml:vendor/etc/media_codecs_vendor_audio.xml \
    $(CONFIG_HAL_COMMON_SRC_DIR)/codec2/service/1.0/c2audio.vendor.base-arm.policy:vendor/etc/seccomp_policy/c2audio.vendor.base-arm.policy \
    $(CONFIG_HAL_COMMON_SRC_DIR)/codec2/service/1.0/c2audio.vendor.base-arm64.policy:vendor/etc/seccomp_policy/c2audio.vendor.base-arm64.policy \
    $(CONFIG_HAL_COMMON_SRC_DIR)/codec2/service/1.0/c2audio.vendor.ext-arm.policy:vendor/etc/seccomp_policy/c2audio.vendor.ext-arm.policy \
    $(CONFIG_HAL_COMMON_SRC_DIR)/codec2/service/1.0/c2audio.vendor.ext-arm64.policy:vendor/etc/seccomp_policy/c2audio.vendor.ext-arm64.policy
endif

# Low latency audio buffer size in frames
PRODUCT_PROPERTY_OVERRIDES += \
    vendor.audio_hal.period_size=192

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

##speaker protection v3 switch and ADSP AFE API version
PRODUCT_PROPERTY_OVERRIDES += \
persist.vendor.audio.spv3.enable=true\
persist.vendor.audio.avs.afe_api_version=2

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

#disable tunnel encoding
PRODUCT_PROPERTY_OVERRIDES += \
vendor.audio.tunnel.encode=false

#Disable RAS Feature by default
PRODUCT_PROPERTY_OVERRIDES += \
persist.vendor.audio.ras.enabled=false

#Buffer size in kbytes for compress offload playback
PRODUCT_PROPERTY_OVERRIDES += \
vendor.audio.offload.buffer.size.kb=32

#Enable audio track offload by default
PRODUCT_PROPERTY_OVERRIDES += \
vendor.audio.offload.track.enable=true

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

#timeout crash duration set to 20sec before system is ready.
#timeout duration updates to default timeout of 5sec once the system is ready.
PRODUCT_PROPERTY_OVERRIDES += \
vendor.audio.hal.boot.timeout.ms=20000

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

#split a2dp DSP supported encoder list
PRODUCT_PROPERTY_OVERRIDES += \
persist.vendor.bt.a2dp_offload_cap=sbc-aptx-aptxtws-aptxhd-aac-ldac

# A2DP offload support
PRODUCT_PROPERTY_OVERRIDES += \
ro.bluetooth.a2dp_offload.supported=true

# Disable A2DP offload
PRODUCT_PROPERTY_OVERRIDES += \
persist.bluetooth.a2dp_offload.disabled=false

#enable software decoders for ALAC and APE
PRODUCT_PROPERTY_OVERRIDES += \
vendor.audio.use.sw.alac.decoder=true
PRODUCT_PROPERTY_OVERRIDES += \
vendor.audio.use.sw.ape.decoder=true

#enable software decoder for MPEG-H
PRODUCT_PROPERTY_OVERRIDES += \
vendor.audio.use.sw.mpegh.decoder=true

#enable hw aac encoder by default
PRODUCT_PROPERTY_OVERRIDES += \
vendor.audio.hw.aac.encoder=false

#offload minimum duration set to 30sec
PRODUCT_PRODUCT_PROPERTIES += \
audio.offload.min.duration.secs=30

#Set AudioFlinger client heap size
PRODUCT_PRODUCT_PROPERTIES += \
ro.af.client_heap_size_kbyte=7168

PRODUCT_PRODUCT_PROPERTIES += \
af.fast_track_multiplier=1

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
persist.vendor.audio.fluence.voicecomm=true

ifeq ($(AUDIO_FEATURE_ENABLED_CODEC_2_0), true)
#enable c2 based encoders/decoders as default NT decoders/encoders
PRODUCT_PROPERTY_OVERRIDES += \
vendor.audio.c2.preferred=true

#Enable dmaBuf heap usage by C2 components
PRODUCT_PROPERTY_OVERRIDES += \
debug.c2.use_dmabufheaps=1

#Enable qc2 audio sw flac frame decode
PRODUCT_PROPERTY_OVERRIDES += \
vendor.qc2audio.per_frame.flac.dec.enabled=true

ifneq ($(GENERIC_ODM_IMAGE),true)
$(warning "Enabling codec2.0 SW only for non-generic odm build variant")
#Rank OMX SW codecs lower than OMX HW codecs
PRODUCT_PROPERTY_OVERRIDES += debug.stagefright.omx_default_rank=0
endif
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

#add dynamic feature flags here
PRODUCT_PROPERTY_OVERRIDES += \
vendor.audio.feature.a2dp_offload.enable=true \
vendor.audio.feature.afe_proxy.enable=true \
vendor.audio.feature.anc_headset.enable=true \
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
vendor.audio.feature.spkr_prot.enable=false \
vendor.audio.feature.ssrec.enable=true \
vendor.audio.feature.usb_offload.enable=true \
vendor.audio.feature.usb_offload_burst_mode.enable=true \
vendor.audio.feature.usb_offload_sidetone_volume.enable=false \
vendor.audio.feature.deepbuffer_as_primary.enable=false \
vendor.audio.feature.vbat.enable=true \
vendor.audio.feature.wsa.enable=false \
vendor.audio.feature.audiozoom.enable=false \
vendor.audio.feature.snd_mon.enable=true \
vendor.audio.gsl.shmem.dmaheap.uncached=true

# for HIDL related packages
PRODUCT_PACKAGES += \
    android.hardware.audio@2.0-service \
    android.hardware.audio@2.0-impl \
    android.hardware.audio.effect@2.0-impl \
    android.hardware.soundtrigger@2.1-impl \
    android.hardware.audio@4.0 \
    android.hardware.audio.common@4.0 \
    android.hardware.audio.common@4.0-util \
    android.hardware.audio@4.0-impl \
    android.hardware.audio.effect@4.0 \
    android.hardware.audio.effect@4.0-impl \
    vendor.qti.hardware.audiohalext@1.0 \
    vendor.qti.hardware.audiohalext@1.0-impl \
    vendor.qti.hardware.audiohalext-utils

# enable audio hidl hal 5.0
PRODUCT_PACKAGES += \
    android.hardware.audio@5.0 \
    android.hardware.audio.common@5.0 \
    android.hardware.audio.common@5.0-util \
    android.hardware.audio@5.0-impl \
    android.hardware.audio.effect@5.0 \
    android.hardware.audio.effect@5.0-impl

# enable audio hidl hal 6.0
PRODUCT_PACKAGES += \
    android.hardware.audio@6.0 \
    android.hardware.audio.common@6.0 \
    android.hardware.audio.common@6.0-util \
    android.hardware.audio@6.0-impl \
    android.hardware.audio.effect@6.0 \
    android.hardware.audio.effect@6.0-impl

# enable audio hidl hal 7.0
PRODUCT_PACKAGES += \
    android.hardware.audio@7.0 \
    android.hardware.audio.common@7.0 \
    android.hardware.audio.common@7.0-util \
    android.hardware.audio@7.0-impl \
    android.hardware.audio.effect@7.0 \
    android.hardware.audio.effect@7.0-impl

# enable audio hidl hal 7.1
PRODUCT_PACKAGES += \
    android.hardware.audio@7.1-impl

# enable sound trigger hidl hal 2.2
PRODUCT_PACKAGES += \
    android.hardware.soundtrigger@2.2-impl \

# enable sound trigger hidl hal 2.3
PRODUCT_PACKAGES += \
    android.hardware.soundtrigger@2.3-impl \

# enable Listen Sound Model hidl 1.0
PRODUCT_PACKAGES += \
    vendor.qti.hardware.ListenSoundModel@1.0 \
    vendor.qti.hardware.ListenSoundModel@1.0-impl

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
