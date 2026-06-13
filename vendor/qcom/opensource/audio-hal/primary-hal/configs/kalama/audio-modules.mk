ifeq ($(TARGET_USES_QMAA),true)
    ifneq ($(TARGET_USES_QMAA_OVERRIDE_AUDIO),true)
    #QMAA Mode is enabled
    TARGET_IS_HEADLESS := true
    endif
endif

#Packages that should not be installed in QMAA are enabled here
ifneq ($(TARGET_IS_HEADLESS),true)

#AGM
AUDIO_AGM := libagmclient
AUDIO_AGM += vendor.qti.hardware.AGMIPC@1.0-impl
#not needed for kalama products
#AUDIO_AGM += vendor.qti.hardware.AGMIPC@1.0-service
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

#PAL Service
AUDIO_PAL += libpalclient
AUDIO_PAL += vendor.qti.hardware.pal@1.0-impl

#PAL Module
AUDIO_PAL := libar-pal
AUDIO_PAL += lib_bt_bundle
AUDIO_PAL += lib_bt_aptx
AUDIO_PAL += lib_bt_ble
AUDIO_PAL += catf
AUDIO_PAL += PalTest
AUDIO_PAL += libaudiochargerlistener
AUDIO_PAL += libhfp_pal

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
AUDIO_C2 += c2audio.vendor.base-arm.policy
AUDIO_C2 += c2audio.vendor.ext-arm.policy
AUDIO_C2 += c2audio.vendor.base-arm64.policy
AUDIO_C2 += c2audio.vendor.ext-arm64.policy

AUDIO_TEST := mcs_test
AUDIO_TEST += ar_util_in_test_example
AUDIO_TEST += osal_in_test_example


#AUDIO_MODULES := audio.a2dp.default
AUDIO_MODULES := audio.usb.default
AUDIO_MODULES += audio.r_submix.default
#AUDIO_MODULES += audio.primary.kalama
AUDIO_MODULES += ftm_test_config
AUDIO_MODULES += ftm_test_config_kalama-qrd-snd-card
AUDIO_MODULES += ftm_test_config_kalama-grd-snd-card
AUDIO_MODULES += audioadsprpcd
AUDIO_MODULES += android.hardware.audio.service_64
AUDIO_MODULES += MTP_acdb_cal.acdb
AUDIO_MODULES += MTP_workspaceFileXml.qwsp
AUDIO_MODULES += CDP_acdb_cal.acdb
AUDIO_MODULES += CDP_workspaceFileXml.qwsp
AUDIO_MODULES += QRD_acdb_cal.acdb
AUDIO_MODULES += QRD_workspaceFileXml.qwsp
AUDIO_MODULES += IDP_UPD_acdb_cal.acdb
AUDIO_MODULES += IDP_UPD_workspaceFileXml.qwsp
AUDIO_MODULES += GRD_acdb_cal.acdb
AUDIO_MODULES += GRD_workspaceFileXml.qwsp
AUDIO_MODULES += fai__2.3.0_0.1__3.0.0_0.0__eai_1.10.pmd
AUDIO_MODULES += fai__2.3.0_0.1__3.0.0_0.0__eai_1.36_enpu2_comp.pmd
AUDIO_MODULES += fai__2.0.0_0.1__3.0.0_0.0__eai_1.36_enpu2.pmd
AUDIO_MODULES += fai__2.7.2_0.0__3.0.0_0.0__eai_1.36_enpu2.pmd
AUDIO_MODULES += fai__2.7.20_0.0__3.0.0_0.0__eai_1.36_enpu2.pmd
AUDIO_MODULES += fai__3.0.0_0.0__eai_1.36_enpu2.pmd
AUDIO_MODULES += ffv__5.0.1_0.1__3.0.0_0.0__eai_2.10_enpu3.pmd
AUDIO_MODULES += ffv__5.0.1_0.1__eai_2.10_enpu3.pmd
AUDIO_MODULES += libfmpal
AUDIO_MODULES += event.eai
AUDIO_MODULES += music.eai
AUDIO_MODULES += speech.eai
AUDIO_MODULES += libqtigefar
AUDIO_MODULES += audiodsd2pcmtest
AUDIO_MODULES += mm-audio-ftm
AUDIO_MODULES += libmcs
AUDIO_MODULES += libquasar
AUDIO_MODULES += sensors.dynamic_sensor_hal

ifeq ($(PRODUCT_ENABLE_QESDK),true)
AUDIO_MODULES += libvui_dmgr
AUDIO_MODULES += libvui_dmgr_client
AUDIO_MODULES += qsap_voiceui
AUDIO_MODULES += qsap_voiceui.rc
AUDIO_MODULES += qsap_voiceui.policy
endif

AUDIO_MODULES += $(AUDIO_AGM)
AUDIO_MODULES += $(AUDIO_PAL)
AUDIO_MODULES += $(AUDIO_C2)
AUDIO_MODULES += $(AUDIO_TEST)

# for HIDL related packages
AUDIO_MODULES += \
    android.hardware.audio@2.0-service_64 \
    android.hardware.audio@2.0-impl \
    android.hardware.audio.effect@2.0-impl \
    android.hardware.soundtrigger@2.1-impl \
    android.hardware.audio@4.0 \
    android.hardware.audio.common@4.0 \
    android.hardware.audio.common@4.0-util \
    android.hardware.audio@4.0-impl \
    android.hardware.audio.effect@4.0 \
    android.hardware.audio.effect@4.0-impl

# enable audio hidl hal 5.0
AUDIO_MODULES += \
    android.hardware.audio@5.0 \
    android.hardware.audio.common@5.0 \
    android.hardware.audio.common@5.0-util \
    android.hardware.audio@5.0-impl \
    android.hardware.audio.effect@5.0 \
    android.hardware.audio.effect@5.0-impl

# enable audio hidl hal 6.0
AUDIO_MODULES += \
    android.hardware.audio@6.0 \
    android.hardware.audio.common@6.0 \
    android.hardware.audio.common@6.0-util \
    android.hardware.audio@6.0-impl \
    android.hardware.audio.effect@6.0 \
    android.hardware.audio.effect@6.0-impl

# enable audio hidl hal 7.0
AUDIO_MODULES += \
    android.hardware.audio@7.0 \
    android.hardware.audio.common@7.0 \
    android.hardware.audio.common@7.0-util \
    android.hardware.audio@7.0-impl \
    android.hardware.audio.effect@7.0 \
    android.hardware.audio.effect@7.0-impl

# enable audio hidl hal 7.1
AUDIO_MODULES += \
    android.hardware.audio@7.1-impl

# enable sound trigger hidl hal 2.2
AUDIO_MODULES += \
    android.hardware.soundtrigger@2.2-impl \

# enable sound trigger hidl hal 2.3
AUDIO_MODULES += \
    android.hardware.soundtrigger@2.3-impl \

# enable Listen Sound Model hidl 1.0
AUDIO_MODULES += \
    vendor.qti.hardware.ListenSoundModel@1.0 \
    vendor.qti.hardware.ListenSoundModel@1.0-impl

# AIDL Audio modules

AUDIO_MODULES += \
    audiohalservice.qti \
    libaudiocorehal.qti \
    libaudioeffecthal.qti

endif
