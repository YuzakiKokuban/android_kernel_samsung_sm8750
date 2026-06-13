LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE:= libqcompostprocbundle
LOCAL_VENDOR_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := soundfx
LOCAL_MODULE_OWNER := qti

LOCAL_C_FLAGS += -Werror -Wall -Wextra

LOCAL_SRC_FILES:= \
        OffloadBundleAidl.cpp \
        OffloadBundleContext.cpp \
        BassBoostContext.cpp \
        EqualizerContext.cpp \
        ReverbContext.cpp \
        VirtualizerContext.cpp \
        ParamDelegator.cpp

LOCAL_STATIC_LIBRARIES := libaudioeffecthal_base_impl_static

LOCAL_SHARED_LIBRARIES:= \
    $(EFFECTS_DEFAULTS_SHARED_LIBRARIES) \
    libar-pal

LOCAL_HEADER_LIBRARIES:= \
    $(EFFECTS_DEFAULTS_HEADERS_LIBRARIES) \
    libacdb_headers

include $(BUILD_SHARED_LIBRARY)
