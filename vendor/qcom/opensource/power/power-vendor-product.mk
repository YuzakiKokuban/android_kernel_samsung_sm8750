#Power product definitions
# PRODUCT_PACKAGES += android.hardware.power-service
# PRODUCT_PACKAGES += android.hardware.power-impl

#Powerhint File
ifeq ($(TARGET_BOARD_PLATFORM),msmnile)
PRODUCT_COPY_FILES += vendor/qcom/opensource/power/config/msmnile/powerhint.xml:$(TARGET_COPY_OUT_VENDOR)/etc/powerhint.xml
else ifeq ($(TARGET_BOARD_PLATFORM),kona)
PRODUCT_COPY_FILES += vendor/qcom/opensource/power/config/kona/powerhint.xml:$(TARGET_COPY_OUT_VENDOR)/etc/powerhint.xml
else ifeq ($(TARGET_BOARD_PLATFORM),lito)
PRODUCT_COPY_FILES += vendor/qcom/opensource/power/config/lito/powerhint.xml:$(TARGET_COPY_OUT_VENDOR)/etc/powerhint.xml
else ifeq ($(TARGET_BOARD_PLATFORM),atoll)
PRODUCT_COPY_FILES += vendor/qcom/opensource/power/config/atoll/powerhint.xml:$(TARGET_COPY_OUT_VENDOR)/etc/powerhint.xml
else ifeq ($(TARGET_BOARD_PLATFORM),lahaina)
PRODUCT_COPY_FILES += vendor/qcom/opensource/power/config/lahaina/powerhint.xml:$(TARGET_COPY_OUT_VENDOR)/etc/powerhint.xml
else ifeq ($(TARGET_BOARD_PLATFORM),holi)
PRODUCT_COPY_FILES += vendor/qcom/opensource/power/config/holi/powerhint.xml:$(TARGET_COPY_OUT_VENDOR)/etc/powerhint.xml
else ifeq ($(TARGET_BOARD_PLATFORM),taro)
PRODUCT_COPY_FILES += vendor/qcom/opensource/power/config/taro/powerhint.xml:$(TARGET_COPY_OUT_VENDOR)/etc/powerhint.xml
else ifeq ($(TARGET_BOARD_PLATFORM),kalama)
PRODUCT_COPY_FILES += vendor/qcom/opensource/power/config/kalama/powerhint.xml:$(TARGET_COPY_OUT_VENDOR)/etc/powerhint.xml
else ifeq ($(TARGET_BOARD_PLATFORM),pineapple)
PRODUCT_COPY_FILES += vendor/qcom/opensource/power/config/pineapple/powerhint.xml:$(TARGET_COPY_OUT_VENDOR)/etc/powerhint.xml
else ifeq ($(TARGET_BOARD_PLATFORM),sun)
ifneq ($(filter pa1q%, $(TARGET_PRODUCT)),)
PRODUCT_COPY_FILES += vendor/qcom/opensource/power/config/sun/powerhint_PA1.xml:$(TARGET_COPY_OUT_VENDOR)/etc/powerhint.xml
else ifneq ($(filter pa2q%, $(TARGET_PRODUCT)),)
PRODUCT_COPY_FILES += vendor/qcom/opensource/power/config/sun/powerhint_PA2.xml:$(TARGET_COPY_OUT_VENDOR)/etc/powerhint.xml
else ifneq ($(filter pa3q%, $(TARGET_PRODUCT)),)
PRODUCT_COPY_FILES += vendor/qcom/opensource/power/config/sun/powerhint_PA3.xml:$(TARGET_COPY_OUT_VENDOR)/etc/powerhint.xml
else
PRODUCT_COPY_FILES += vendor/qcom/opensource/power/config/sun/powerhint.xml:$(TARGET_COPY_OUT_VENDOR)/etc/powerhint.xml
endif
else ifeq ($(TARGET_BOARD_PLATFORM),blair)
PRODUCT_COPY_FILES += vendor/qcom/opensource/power/config/holi/powerhint.xml:$(TARGET_COPY_OUT_VENDOR)/etc/powerhint.xml
else ifeq ($(TARGET_BOARD_PLATFORM),pitti)
PRODUCT_COPY_FILES += vendor/qcom/opensource/power/config/pitti/powerhint.xml:$(TARGET_COPY_OUT_VENDOR)/etc/powerhint.xml
else ifeq ($(TARGET_BOARD_PLATFORM),volcano)
PRODUCT_COPY_FILES += vendor/qcom/opensource/power/config/volcano/powerhint.xml:$(TARGET_COPY_OUT_VENDOR)/etc/powerhint.xml
else ifeq ($(TARGET_BOARD_PLATFORM),niobe)
PRODUCT_COPY_FILES += vendor/qcom/opensource/power/config/niobe/powerhint.xml:$(TARGET_COPY_OUT_VENDOR)/etc/powerhint.xml
else ifeq ($(TARGET_BOARD_PLATFORM),parrot)
PRODUCT_COPY_FILES += vendor/qcom/opensource/power/config/parrot/powerhint.xml:$(TARGET_COPY_OUT_VENDOR)/etc/powerhint.xml
endif
