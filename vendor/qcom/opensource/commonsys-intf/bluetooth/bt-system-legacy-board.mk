#ANT
ifneq ($(filter sdm660 msm8998, $(TARGET_BOARD_PLATFORM)),)
BOARD_ANT_WIRELESS_DEVICE := "qualcomm-hidl"
endif

ifneq ($(filter msm8937 msm8953 msm8909, $(TARGET_BOARD_PLATFORM)),)
BOARD_ANT_WIRELESS_DEVICE := "vfs-prerelease"
endif

#BT
BOARD_HAVE_BLUETOOTH := true
BOARD_HAVE_BLUETOOTH_QCOM := true

#FM
BOARD_HAVE_QCOM_FM := true
