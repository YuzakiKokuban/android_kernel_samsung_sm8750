LOCAL_PATH := $(call my-dir)

ifeq ($(call is-vendor-board-platform,QCOM),true)

# HAL module implemenation stored in
# hw/<POWERS_HARDWARE_MODULE_ID>.<ro.hardware>.so
include $(CLEAR_VARS)

LOCAL_MODULE_RELATIVE_PATH := hw
# KEYSTONE(I1132378f14428bf511f3cea4f419e90a6e89f823,b/181709127)
LOCAL_SHARED_LIBRARIES := android.hardware.common-V2-ndk android.hardware.common.fmq-V1-ndk libfmq libutils liblog libcutils libdl libxml2 libbase libbinder_ndk android.hardware.power-V5-ndk
LOCAL_HEADER_LIBRARIES += libutils_headers
LOCAL_HEADER_LIBRARIES += libhardware_headers
LOCAL_SRC_FILES := power-common.c metadata-parser.c utils.c list.c hint-data.c powerhintparser.c Power.cpp main.cpp PowerHintSession.cpp
LOCAL_C_INCLUDES := external/libxml2/include \
                    external/icu/icu4c/source/common

# Include target-specific files.
ifeq ($(call is-board-platform-in-list, msm8974), true)
LOCAL_SRC_FILES += power-8974.c
endif

ifeq ($(call is-board-platform-in-list, msm8226), true)
LOCAL_SRC_FILES += power-8226.c
endif

ifeq ($(call is-board-platform-in-list, msm8610), true)
LOCAL_SRC_FILES += power-8610.c
endif

ifeq ($(call is-board-platform-in-list, apq8084), true)
LOCAL_SRC_FILES += power-8084.c
endif

ifeq ($(call is-board-platform-in-list, msm8994), true)
LOCAL_SRC_FILES += power-8994.c
endif

ifeq ($(call is-board-platform-in-list, msm8996), true)
LOCAL_SRC_FILES += power-8996.c
endif

ifeq ($(call is-board-platform-in-list,msm8937), true)
LOCAL_SRC_FILES += power-8952.c
endif

ifeq ($(call is-board-platform-in-list,msm8952), true)
LOCAL_SRC_FILES += power-8952.c
endif

ifeq ($(call is-board-platform-in-list,msm8953), true)
LOCAL_SRC_FILES += power-8953.c
endif

ifeq ($(call is-board-platform-in-list,msm8998 apq8098_latv), true)
LOCAL_SRC_FILES += power-8998.c
endif

ifeq ($(call is-board-platform-in-list,sdm660), true)
LOCAL_SRC_FILES += power-660.c
endif

ifeq ($(call is-board-platform-in-list,sdm845), true)
LOCAL_SRC_FILES += power-845.c
endif

ifeq ($(call is-board-platform-in-list,sdm710), true)
LOCAL_SRC_FILES += power-710.c
endif

ifeq ($(call is-board-platform-in-list,qcs605), true)
LOCAL_SRC_FILES += power-710.c
endif

ifeq ($(call is-board-platform-in-list,trinket), true)
LOCAL_SHARED_LIBRARIES := liblog libcutils libdl libxml2
LOCAL_SRC_FILES := power.c metadata-parser.c utils.c list.c hint-data.c powerhintparser.c
LOCAL_SRC_FILES += power-6125.c
endif

ifeq ($(call is-board-platform-in-list,msmnile), true)
LOCAL_SRC_FILES += power-msmnile.c
endif

ifeq ($(TARGET_USES_INTERACTION_BOOST),true)
    LOCAL_CFLAGS += -DINTERACTION_BOOST
endif

ifeq ($(call is-board-platform-in-list,trinket), true)
LOCAL_MODULE := power.qcom
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS += -Wno-unused-parameter -Wno-unused-variable
LOCAL_VENDOR_MODULE := true
include $(BUILD_SHARED_LIBRARY)
else
LOCAL_MODULE := android.hardware.power-service
LOCAL_INIT_RC := android.hardware.power-service.rc
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS += -Wno-unused-parameter -Wno-unused-variable
LOCAL_VENDOR_MODULE := true
LOCAL_VINTF_FRAGMENTS := power.xml
include $(BUILD_EXECUTABLE)
endif

ifeq ($(TARGET_BOARD_PLATFORM), sun)
include $(CLEAR_VARS)

LOCAL_SHARED_LIBRARIES := android.hardware.common-V2-ndk android.hardware.common.fmq-V1-ndk libfmq liblog libcutils libdl libxml2 libbase libutils libbinder_ndk android.hardware.power-V5-ndk libbinder libclang_rt.ubsan_standalone
LOCAL_HEADER_LIBRARIES += libutils_headers
LOCAL_HEADER_LIBRARIES += libhardware_headers
LOCAL_SRC_FILES := power-common.c metadata-parser.c utils.c list.c hint-data.c powerhintparser.c Power.cpp fuzzer.cpp PowerHintSession.cpp
LOCAL_C_INCLUDES := external/libxml2/include \
                    external/icu/icu4c/source/common

ifeq ($(TARGET_USES_INTERACTION_BOOST),true)
    LOCAL_CFLAGS += -DINTERACTION_BOOST
endif

LOCAL_MODULE := aidl_fuzzer_power_service

LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS += -Wno-unused-parameter -Wno-unused-variable
LOCAL_VENDOR_MODULE := true

LOCAL_STATIC_LIBRARIES += libbinder_random_parcel

include $(BUILD_FUZZ_TEST)
endif

endif
