#Enable AAudio MMAP/NOIRQ data path
#2 is AAUDIO_POLICY_AUTO so it will try MMAP then fallback to Legacy path
PRODUCT_PROPERTY_OVERRIDES += aaudio.mmap_policy=2
#Allow EXCLUSIVE then fall back to SHARED.
PRODUCT_PROPERTY_OVERRIDES += aaudio.mmap_exclusive_policy=2
PRODUCT_PROPERTY_OVERRIDES += aaudio.hw_burst_min_usec=2000

# spf hdr record either true or false
AUDIO_HAL_PROP += \
vendor.audio.hdr.spf.record.enable=false

# spf hdr record either true or false
AUDIO_HAL_PROP += \
vendor.audio.hdr.record.enable=false

#compress offload
AUDIO_HAL_PROP += \
vendor.audio.offload.buffer.size.kb=32

# compress capture feature related
AUDIO_HAL_PROP += \
vendor.audio.compress_capture.enabled=true \
vendor.audio.compress_capture.aac=true
# compress capture end

#AIDL HAL enabled
AUDIO_HAL_PROP += \
vendor.audio.hal.aidl.enabled=true

PRODUCT_VENDOR_PROPERTIES += \
    $(AUDIO_HAL_PROP)
