CURRENT_PATH := $(call my-dir)

LOCAL_PATH:= $(call my-dir)

# Build Header library to expose effect headers
include $(CLEAR_VARS)
LOCAL_MODULE := libaudioeffectsaidlqti_headers
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include
LOCAL_VENDOR_MODULE := true
include $(BUILD_HEADER_LIBRARY)


#Build static library used by all effects
include $(CLEAR_VARS)
LOCAL_MODULE:= libaudioeffecthal_base_impl_static
LOCAL_VENDOR_MODULE := true
LOCAL_MODULE_OWNER := qti

LOCAL_C_FLAGS += -Werror -Wall -Wextra -Wthread-safety

LOCAL_SRC_FILES:= \
        EffectThread.cpp \
        EffectImpl.cpp \
        EffectContext.cpp

LOCAL_SHARED_LIBRARIES:= \
    $(EFFECTS_DEFAULTS_SHARED_LIBRARIES)

LOCAL_HEADER_LIBRARIES:= $(EFFECTS_DEFAULTS_HEADERS_LIBRARIES)

include $(BUILD_STATIC_LIBRARY)

# build base effects library
include $(CLEAR_VARS)

LOCAL_MODULE:= libaudioeffecthal.qti
LOCAL_VENDOR_MODULE := true
LOCAL_MODULE_OWNER := qti
LOCAL_MODULE_RELATIVE_PATH := hw

LOCAL_C_FLAGS += -Werror -Wall -Wextra

LOCAL_SRC_FILES:= \
        EffectConfig.cpp \
        EffectFactory.cpp \
        EffectMain.cpp

LOCAL_STATIC_LIBRARIES := libaudioeffecthal_base_impl_static
LOCAL_VINTF_FRAGMENTS := audioeffectservice_qti.xml

LOCAL_SHARED_LIBRARIES:= \
    $(EFFECTS_DEFAULTS_SHARED_LIBRARIES) \
    libtinyxml2

LOCAL_HEADER_LIBRARIES:= $(EFFECTS_DEFAULTS_HEADERS_LIBRARIES)

include $(BUILD_SHARED_LIBRARY)

include $(CURRENT_PATH)/qcom-effects/Android.mk