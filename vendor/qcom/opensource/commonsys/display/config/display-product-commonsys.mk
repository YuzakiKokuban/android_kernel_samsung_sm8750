PRODUCT_PACKAGES += libdisplayconfig.system \
                    libgralloc.system.qti \
                    libdrm \
                    liblayerext.qti \
                    libsmomoconfig.qti \
                    libcomposerextn.qti \
                    libdisplayconfig.system.qti

SOONG_CONFIG_NAMESPACES += qtidisplaycommonsys
# Soong Keys
SOONG_CONFIG_qtidisplaycommonsys := displayextension composer3ext
# Soong Values

# displayextension controls global compile time disablement of SF extensions
SOONG_CONFIG_qtidisplaycommonsys_displayextension := false

# Variables can be added here on a transient basis to merge
# features that are not yet consumed in keystone
# Once the feature has been consumed, these can be removed
# and the feature can be enabled/disabled at run time via android
# properties
SOONG_CONFIG_qtidisplaycommonsys_composer3ext := false

ifeq ($(call is-vendor-board-platform,QCOM),true)
    SOONG_CONFIG_qtidisplaycommonsys_displayextension := true
    SOONG_CONFIG_qtidisplaycommonsys_composer3ext := true
endif
