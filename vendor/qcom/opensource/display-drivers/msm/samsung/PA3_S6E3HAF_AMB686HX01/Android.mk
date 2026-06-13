XXD := /usr/bin/xxd
SED := /bin/sed

#Translate .dat file to .h to cover the case which can not use request_firmware(Recovery Mode)
CLEAR_TMP := $(shell rm -f PA3_S6E3HAF_AMB686HX01_PDF_DATA)
CLEAR_CURRENT := $(shell rm -f $(DISPLAY_BLD_DIR)/msm/samsung/PA3_S6E3HAF_AMB686HX01/PA3_S6E3HAF_AMB686HX01_PDF.h)
COPY_TO_HERE := $(shell cp -vf $(DISPLAY_BLD_DIR)/msm/samsung/panel_data_file/PA3_S6E3HAF_AMB686HX01.dat PA3_S6E3HAF_AMB686HX01_PDF_DATA)
DATA_TO_HEX := $(shell $(XXD) -i PA3_S6E3HAF_AMB686HX01_PDF_DATA > $(DISPLAY_BLD_DIR)/msm/samsung/PA3_S6E3HAF_AMB686HX01/PA3_S6E3HAF_AMB686HX01_PDF.h)
ADD_NULL_CHR := $(shell $(SED) -i -e 's/\([0-9a-f]\)$$/\0, 0x00/' $(DISPLAY_BLD_DIR)/msm/samsung/PA3_S6E3HAF_AMB686HX01/PA3_S6E3HAF_AMB686HX01_PDF.h)
