LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE        := libagmmixer
LOCAL_MODULE_OWNER  := qti
LOCAL_MODULE_TAGS   := optional
LOCAL_VENDOR_MODULE := true

LOCAL_CFLAGS        += -Wno-unused-parameter -Wno-unused-result
LOCAL_SRC_FILES     := agmmixer.c

LOCAL_HEADER_LIBRARIES := \
    libagm_headers \
    libacdb_headers

#if android version is R, refer to qtitinyxx otherwise use upstream ones
#This assumes we would be using AR code only for Android R and subsequent versions.
ifneq ($(filter 11 R, $(PLATFORM_VERSION)),)
LOCAL_SHARED_LIBRARIES += libqti-tinyalsa
else
LOCAL_SHARED_LIBRARIES += libtinyalsa
endif


LOCAL_SHARED_LIBRARIES += \
     libexpat

include $(BUILD_SHARED_LIBRARY)

# Build agmplay
include $(CLEAR_VARS)

LOCAL_MODULE        := agmplay
LOCAL_MODULE_OWNER  := qti
LOCAL_MODULE_TAGS   := optional
LOCAL_VENDOR_MODULE := true

LOCAL_CFLAGS        += -Wno-unused-parameter -Wno-unused-result
LOCAL_CFLAGS        += -DBACKEND_CONF_FILE=\"/vendor/etc/backend_conf.xml\"
LOCAL_CFLAGS        += -fexceptions
LOCAL_SRC_FILES     := AgmPlay.cpp \
AgmPlayer.cpp \
RiffWaveParser.cpp \
ChunkParser.cpp \
PlaybackCommand.cpp \
PlaybackCommandParser.cpp

LOCAL_HEADER_LIBRARIES := \
    libagm_headers \
    libacdb_headers

#if android version is R, refer to qtitinyxx otherwise use upstream ones
#This assumes we would be using AR code only for Android R and subsequent versions.
ifneq ($(filter 11 R, $(PLATFORM_VERSION)),)
LOCAL_SHARED_LIBRARIES += libqti-tinyalsa
else
LOCAL_SHARED_LIBRARIES += libtinyalsa
endif

LOCAL_SHARED_LIBRARIES += \
    libagmmixer

include $(BUILD_EXECUTABLE)
include $(CLEAR_VARS)

LOCAL_MODULE        := agmcap
LOCAL_MODULE_OWNER  := qti
LOCAL_MODULE_TAGS   := optional
LOCAL_VENDOR_MODULE := true

LOCAL_CFLAGS        += -Wno-unused-parameter -Wno-unused-result
LOCAL_CFLAGS        += -DBACKEND_CONF_FILE=\"/vendor/etc/backend_conf.xml\"
LOCAL_SRC_FILES     := AgmCap.cpp

LOCAL_HEADER_LIBRARIES := \
    libagm_headers \
    libacdb_headers

#if android version is R, refer to qtitinyxx otherwise use upstream ones
#This assumes we would be using AR code only for Android R and subsequent versions.
ifneq ($(filter 11 R, $(PLATFORM_VERSION)),)
LOCAL_SHARED_LIBRARIES += libqti-tinyalsa
else
LOCAL_SHARED_LIBRARIES += libtinyalsa
endif

LOCAL_SHARED_LIBRARIES += \
    libagmmixer

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

LOCAL_MODULE        := agmhostless
LOCAL_MODULE_OWNER  := qti
LOCAL_MODULE_TAGS   := optional
LOCAL_VENDOR_MODULE := true

LOCAL_CFLAGS        += -Wno-unused-parameter -Wno-unused-result
LOCAL_CFLAGS        += -DBACKEND_CONF_FILE=\"/vendor/etc/backend_conf.xml\"
LOCAL_SRC_FILES     := agmhostless.c

LOCAL_HEADER_LIBRARIES := \
    libagm_headers \
    libacdb_headers

#if android version is R, refer to qtitinyxx otherwise use upstream ones
#This assumes we would be using AR code only for Android R and subsequent versions.
ifneq ($(filter 11 R, $(PLATFORM_VERSION)),)
LOCAL_SHARED_LIBRARIES += libqti-tinyalsa
else
LOCAL_SHARED_LIBRARIES += libtinyalsa
endif

LOCAL_SHARED_LIBRARIES += \
    libagmmixer

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

LOCAL_MODULE        := agmcompressplay
LOCAL_MODULE_OWNER  := qti
LOCAL_MODULE_TAGS   := optional
LOCAL_VENDOR_MODULE := true

LOCAL_CFLAGS        += -Wno-unused-parameter -Wno-unused-result
LOCAL_CFLAGS        += -DBACKEND_CONF_FILE=\"/vendor/etc/backend_conf.xml\"

LOCAL_C_INCLUDES    += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
LOCAL_ADDITIONAL_DEPENDENCIES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

LOCAL_SRC_FILES     := agmcompressplay.c

LOCAL_HEADER_LIBRARIES := \
    libagm_headers \
    libacdb_headers

# Use flag based selection to use QTI vs open source tinycompress project

ifeq ($(TARGET_USES_QTI_TINYCOMPRESS),true)
LOCAL_SHARED_LIBRARIES += libqti-tinyalsa\
                          libqti-tinycompress
else
LOCAL_C_INCLUDES += $(TOP)/external/tinycompress/include
LOCAL_SHARED_LIBRARIES += libtinyalsa\
                          libtinycompress
endif

LOCAL_SHARED_LIBRARIES += \
    libagmmixer

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

LOCAL_MODULE        := agmcompresscap
LOCAL_MODULE_OWNER  := qti
LOCAL_MODULE_TAGS   := optional
LOCAL_VENDOR_MODULE := true

LOCAL_CFLAGS        += -Wno-unused-parameter -Wno-unused-result
LOCAL_CFLAGS        += -DBACKEND_CONF_FILE=\"/vendor/etc/backend_conf.xml\"

LOCAL_C_INCLUDES    += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
LOCAL_ADDITIONAL_DEPENDENCIES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

LOCAL_SRC_FILES     := agmcompresscap.c

LOCAL_HEADER_LIBRARIES := \
    libagm_headers \
    libacdb_headers

# Use flag based selection to use QTI vs open source tinycompress project

ifeq ($(TARGET_USES_QTI_TINYCOMPRESS),true)
LOCAL_SHARED_LIBRARIES += libqti-tinyalsa\
                          libqti-tinycompress
else
LOCAL_C_INCLUDES += $(TOP)/external/tinycompress/include
LOCAL_SHARED_LIBRARIES += libtinyalsa\
                          libtinycompress
endif

LOCAL_SHARED_LIBRARIES += \
    libagmmixer

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

LOCAL_MODULE        := agmvoiceui
LOCAL_MODULE_OWNER  := qti
LOCAL_MODULE_TAGS   := optional
LOCAL_VENDOR_MODULE := true

LOCAL_CFLAGS        += -Wno-unused-parameter -Wno-unused-result
LOCAL_CFLAGS        += -DBACKEND_CONF_FILE=\"/vendor/etc/backend_conf.xml\"
LOCAL_SRC_FILES     := agm_voiceui.c

LOCAL_HEADER_LIBRARIES := \
    libagm_headers \
    libacdb_headers

#if android version is R, refer to qtitinyxx otherwise use upstream ones
#This assumes we would be using AR code only for Android R and subsequent versions.
ifneq ($(filter 11 R, $(PLATFORM_VERSION)),)
LOCAL_SHARED_LIBRARIES += libqti-tinyalsa
else
LOCAL_SHARED_LIBRARIES += libtinyalsa
endif

LOCAL_SHARED_LIBRARIES += \
    libagmmixer

include $(BUILD_EXECUTABLE)

