#BT
BOARD_HAVE_BLUETOOTH := true
BOARD_HAVE_BLUETOOTH_QCOM := true

#FM
BOARD_HAVE_QCOM_FM := true

ifneq ($(filter sdm660 msm8998, $(TARGET_BOARD_PLATFORM)),)
BOARD_ANT_WIRELESS_DEVICE := "qualcomm-hidl"
BOARD_HAS_QCA_FM_SOC := "cherokee"
endif

ifneq ($(filter msm8996, $(TARGET_BOARD_PLATFORM)),)
#BOARD_ANT_WIRELESS_DEVICE := "qualcomm-hidl"
endif

ifneq ($(filter msm8937 msm8953 msm8909, $(TARGET_BOARD_PLATFORM)),)
BOARD_ANT_WIRELESS_DEVICE := "vfs-prerelease"
endif

TARGET_USE_QTI_BT_CONFIGSTORE := true
TARGET_USE_QTI_BT_SAR := true
TARGET_USE_QTI_VND_FWK_DETECT := true
TARGET_USE_BT_DUN := false

ifeq ($(TARGET_BOARD_PLATFORM),msm8937)
ifeq ($(TARGET_BOARD_SUFFIX),_32go)
TARGET_USE_QTI_BT_SAR := false
endif
endif
