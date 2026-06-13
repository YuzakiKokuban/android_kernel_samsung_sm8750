PRODUCT_COPY_FILES += vendor/qcom/opensource/display-core/snapalloc/resources/camera_alignments.json:$(TARGET_COPY_OUT_VENDOR)/etc/display/camera_alignments.json
PRODUCT_COPY_FILES += vendor/qcom/opensource/display-core/snapalloc/resources/cpu_alignments.json:$(TARGET_COPY_OUT_VENDOR)/etc/display/cpu_alignments.json
PRODUCT_COPY_FILES += vendor/qcom/opensource/display-core/snapalloc/resources/default_alignments.json:$(TARGET_COPY_OUT_VENDOR)/etc/display/default_alignments.json
PRODUCT_COPY_FILES += vendor/qcom/opensource/display-core/snapalloc/resources/display_alignments.json:$(TARGET_COPY_OUT_VENDOR)/etc/display/display_alignments.json
PRODUCT_COPY_FILES += vendor/qcom/opensource/display-core/snapalloc/resources/formats.json:$(TARGET_COPY_OUT_VENDOR)/etc/display/formats.json
PRODUCT_COPY_FILES += vendor/qcom/opensource/display-core/snapalloc/resources/graphics_alignments.json:$(TARGET_COPY_OUT_VENDOR)/etc/display/graphics_alignments.json
PRODUCT_COPY_FILES += vendor/qcom/opensource/display-core/snapalloc/resources/ubwc_alignments.json:$(TARGET_COPY_OUT_VENDOR)/etc/display/ubwc_alignments.json
PRODUCT_COPY_FILES += vendor/qcom/opensource/display-core/snapalloc/resources/video_alignments.json:$(TARGET_COPY_OUT_VENDOR)/etc/display/video_alignments.json

#QDCM calibration json file for nt37801 panel
PRODUCT_COPY_FILES += vendor/qcom/opensource/display-core/config/qdcm_calib_data_nt37801_amoled_cmd_mode_dsi_csot_panel_with_DSC.json:$(TARGET_COPY_OUT_VENDOR)/etc/display/qdcm_calib_data_nt37801_amoled_cmd_mode_dsi_csot_panel_with_DSC.json
PRODUCT_COPY_FILES += vendor/qcom/opensource/display-core/config/qdcm_calib_data_nt37801_amoled_video_mode_dsi_csot_panel_with_DSC.json:$(TARGET_COPY_OUT_VENDOR)/etc/display/qdcm_calib_data_nt37801_amoled_video_mode_dsi_csot_panel_with_DSC.json
PRODUCT_COPY_FILES += vendor/qcom/opensource/display-core/config/qdcm_calib_data_nt37801_amoled_cmd_mode_dsi_csot_panel_with_DSC_CPHY.json:$(TARGET_COPY_OUT_VENDOR)/etc/display/qdcm_calib_data_nt37801_amoled_cmd_mode_dsi_csot_panel_with_DSC_CPHY.json
PRODUCT_COPY_FILES += vendor/qcom/opensource/display-core/config/qdcm_calib_data_nt37801_amoled_video_mode_dsi_csot_panel_with_DSC_CPHY.json:$(TARGET_COPY_OUT_VENDOR)/etc/display/qdcm_calib_data_nt37801_amoled_video_mode_dsi_csot_panel_with_DSC_CPHY.json

#Backlight calibration xml file for nt37801 amoled panels
PRODUCT_COPY_FILES += vendor/qcom/opensource/display-core/config/backlight_calib_nt37801_amoled_cmd_mode_dsi_csot_panel_with_DSC_CPHY.xml:$(TARGET_COPY_OUT_VENDOR)/etc/display/backlight_calib_nt37801_amoled_cmd_mode_dsi_csot_panel_with_DSC_CPHY.xml
PRODUCT_COPY_FILES += vendor/qcom/opensource/display-core/config/backlight_calib_nt37801_amoled_cmd_mode_dsi_csot_panel_with_DSC_CPHY.xml:$(TARGET_COPY_OUT_VENDOR)/etc/display/backlight_calib_nt37801_amoled_video_mode_dsi_csot_panel_with_DSC_CPHY.xml
PRODUCT_COPY_FILES += vendor/qcom/opensource/display-core/config/backlight_calib_nt37801_amoled_cmd_mode_dsi_csot_panel_with_DSC_CPHY.xml:$(TARGET_COPY_OUT_VENDOR)/etc/display/backlight_calib_nt37801_amoled_cmd_mode_dsi_csot_panel_with_DSC.xml
PRODUCT_COPY_FILES += vendor/qcom/opensource/display-core/config/backlight_calib_nt37801_amoled_cmd_mode_dsi_csot_panel_with_DSC_CPHY.xml:$(TARGET_COPY_OUT_VENDOR)/etc/display/backlight_calib_nt37801_amoled_video_mode_dsi_csot_panel_with_DSC.xml

#SDR Dimming config file for nt37801, display id is 4630946916234099603
PRODUCT_COPY_FILES += vendor/qcom/opensource/display-core/config/display_id_4630946916234099603.xml:$(TARGET_COPY_OUT_VENDOR)/etc/displayconfig/display_id_4630946916234099603.xml
PRODUCT_COPY_FILES += vendor/qcom/opensource/display-core/config/sdm_display_resolution_extn.xml:$(TARGET_COPY_OUT_VENDOR)/etc/display/sdm_display_resolution_extn.xml

ifneq ($(TARGET_HAS_LOW_RAM),true)
#Multi-stc libraries config xml file
PRODUCT_COPY_FILES += vendor/qcom/opensource/display-core/config/snapdragon_color_libs_config.xml:$(TARGET_COPY_OUT_VENDOR)/etc/snapdragon_color_libs_config.xml
endif
