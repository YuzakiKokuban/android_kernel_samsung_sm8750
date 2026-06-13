MM_AUDIO := libaudiohal
MM_AUDIO += audiosphere
MM_AUDIO += audiosphere.xml
MM_AUDIO += libbinauralrenderer_wrapper.qti
MM_AUDIO += libhoaeffects_csim
MM_AUDIO += libhoaeffects.qti
MM_AUDIO += liblistenjni.qti
MM_AUDIO += liblistensoundmodel2.qti
MM_AUDIO += libvr_amb_engine
MM_AUDIO += libvraudio_client.qti
MM_AUDIO += libvraudio
MM_AUDIO += libvr_object_engine
MM_AUDIO += libvr_sam_wrapper
MM_AUDIO += libaudio-resampler
MM_AUDIO += libaudioprocessing
MM_AUDIO += libaudiopolicymanagerdefault
MM_AUDIO += libaudiopolicyenginedefault

MM_AUDIO += mixerops_objdump
MM_AUDIO += test-mixer
MM_AUDIO += test-resampler
MM_AUDIO += mixerops_benchmark
MM_AUDIO += resampler_tests

# AIDL AHAL VENDOR EXTENSION
MM_AUDIO += libaudiohalvendorextn

PRODUCT_SYSTEM_EXT_PROPERTIES += \
    ro.audio.ihaladaptervendorextension_enabled=true

MM_AUDIO_DBG := MhaPlayerDemoApp
MM_AUDIO_DBG += MhaRecorderDemoApp

PRODUCT_PACKAGES += $(MM_AUDIO)
PRODUCT_PACKAGES_DEBUG += $(MM_AUDIO_DBG)
