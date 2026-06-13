# vendor opensource packages
ifneq ($(BOARD_VENDOR_QCOM_GPS_LOC_API_HARDWARE),)

PRODUCT_PACKAGES += libloc_api_v02
PRODUCT_PACKAGES += libgnsspps
PRODUCT_PACKAGES += libsynergy_loc_api
PRODUCT_PACKAGES += izat_remote_api_headers
PRODUCT_PACKAGES += loc_sll_if_headers
PRODUCT_PACKAGES += libloc_socket

ifneq ($(TARGET_SUPPORTS_WEARABLES),true)
PRODUCT_PACKAGES += liblocation_api_msg
PRODUCT_PACKAGES += liblocation_integration_api
PRODUCT_PACKAGES += liblocation_client_api
endif
endif#BOARD_VENDOR_QCOM_GPS_LOC_API_HARDWARE
