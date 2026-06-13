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
AUDIO_AGM += libagmipcservice
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
AUDIO_AGM += gtest_agm_test

#PAL Module
AUDIO_PAL := libar-pal
AUDIO_PAL += lib_bt_bundle
AUDIO_PAL += lib_bt_aptx
AUDIO_PAL += lib_bt_ble
AUDIO_PAL += catf
AUDIO_PAL += PalTest
AUDIO_PAL += libaudiochargerlistener
AUDIO_PAL += libhfp_pal
#PAL Service
AUDIO_PAL += libpalclient
AUDIO_PAL += libpalipcservice
AUDIO_PAL += libpaleventnotifier

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


AUDIO_MODULES := ftm_test_config
AUDIO_MODULES += ftm_test_config_sun-qrd-snd-card
AUDIO_MODULES += ftm_test_config_sun-qrd-sku2-snd-card
AUDIO_MODULES += audioadsprpcd
AUDIO_MODULES += MTP_acdb_cal.acdb
AUDIO_MODULES += MTP_workspaceFileXml.qwsp
AUDIO_MODULES += CDP_acdb_cal.acdb
AUDIO_MODULES += CDP_workspaceFileXml.qwsp
AUDIO_MODULES += QRD_acdb_cal.acdb
AUDIO_MODULES += QRD_workspaceFileXml.qwsp
AUDIO_MODULES += IDP_UPD_acdb_cal.acdb
AUDIO_MODULES += IDP_UPD_workspaceFileXml.qwsp
AUDIO_MODULES += QRD_sun_sku2_acdb_cal.acdb
AUDIO_MODULES += QRD_sun_sku2_workspaceFileXml.qwsp
AUDIO_MODULES += fai__2.3.0_0.1__3.0.0_0.0__eai_1.10.pmd
AUDIO_MODULES += fai__2.3.0_0.1__3.0.0_0.0__eai_1.36_enpu2_comp.pmd
AUDIO_MODULES += fai__2.0.0_0.1__3.0.0_0.0__eai_1.36_enpu2.pmd
AUDIO_MODULES += fai__2.7.2_0.0__3.0.0_0.0__eai_1.36_enpu2.pmd
AUDIO_MODULES += fai__2.7.20_0.0__3.0.0_0.0__eai_1.36_enpu2.pmd
AUDIO_MODULES += fai__3.0.0_0.0__eai_1.36_enpu2.pmd
AUDIO_MODULES += ffv__7.1.1_0.1__eai_4.8_enpu_v5.pmd
AUDIO_MODULES += ffv__7.1.1_0.2__eai_4.8_enpu_v5.pmd
AUDIO_MODULES += ffv__5.0.2_0.1__eai_4.8_enpu_v5.pmd
AUDIO_MODULES += hk01b_relu_eAI_4.6_eNPU_V5_adsp_i.pmd
AUDIO_MODULES += click.pcm
AUDIO_MODULES += double_click.pcm
AUDIO_MODULES += heavy_click.pcm
AUDIO_MODULES += pop.pcm
AUDIO_MODULES += reserved_1.pcm
AUDIO_MODULES += reserved_2.pcm
AUDIO_MODULES += reserved_3.pcm
AUDIO_MODULES += reserved_4.pcm
AUDIO_MODULES += reserved_5.pcm
AUDIO_MODULES += reserved_6.pcm
AUDIO_MODULES += reserved_7.pcm
AUDIO_MODULES += reserved_8.pcm
AUDIO_MODULES += texture_tick.pcm
AUDIO_MODULES += thud.pcm
AUDIO_MODULES += tick.pcm
AUDIO_MODULES += libfmpal
AUDIO_MODULES += event.eai
AUDIO_MODULES += music.eai
AUDIO_MODULES += speech.eai
AUDIO_MODULES += environment.eai
AUDIO_MODULES += libqtigefar
AUDIO_MODULES += audiodsd2pcmtest
AUDIO_MODULES += mm-audio-ftm
AUDIO_MODULES += libmcs
AUDIO_MODULES += libquasar
AUDIO_MODULES += sensors.dynamic_sensor_hal
AUDIO_MODULES += libvui_dmgr
AUDIO_MODULES += libvui_dmgr_client
AUDIO_MODULES += qsap_voiceui
AUDIO_MODULES += qsap_voiceui.policy
AUDIO_MODULES += libaudiofeaturestats
AUDIO_MODULES += libhotword_intf
AUDIO_MODULES += libcustomva_intf
AUDIO_MODULES += libvui_intf
AUDIO_MODULES += libVoiceSdk
AUDIO_MODULES += libtensorflowlite_c
AUDIO_MODULES += libqasr
AUDIO_MODULES += libQnnHtp
AUDIO_MODULES += libQnnHtpV79Skel
AUDIO_MODULES += libQnnHtpV79Stub
AUDIO_MODULES += libQnnSystem

AUDIO_MODULES += $(AUDIO_AGM)
AUDIO_MODULES += $(AUDIO_PAL)
AUDIO_MODULES += $(AUDIO_C2)
AUDIO_MODULES += $(AUDIO_TEST)

 # sound trigger aidl library
AUDIO_MODULES += libsoundtriggerhal.qti

# enable Listen Sound Model aidl 1.0
AUDIO_MODULES += \
    liblistensoundmodelaidl \
    liblistensoundmodel2vendor \
    vendor.qti.hardware.ListenSoundModelAidl-V1-ndk.vendor

# AIDL Audio modules

AUDIO_MODULES += \
    audiohalservice.qti \
    libaudiocorehal.qti \
    libaudiocorehal.default \
    libaudioeffecthal.qti

# add modules for fuzzing
ifneq ($(filter audio,$(QC_HWASAN))$(filter hwaddress,$(SANITIZE_TARGET)),)
AUDIO_MODULES += fuzz-audio-hal
endif

endif
