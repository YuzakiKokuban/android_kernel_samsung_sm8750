################################################################################
# Copyright (c) 2022, STMicroelectronics - All Rights Reserved
#
# This file is part of VL53L8 Kernel Driver and is dual licensed,
# either 'STMicroelectronics Proprietary license'
# or 'BSD 3-clause "New" or "Revised" License' , at your option.
#
################################################################################
#
# 'STMicroelectronics Proprietary license'
#
################################################################################
#
# License terms: STMicroelectronics Proprietary in accordance with licensing
# terms at www.st.com/sla0081
#
# STMicroelectronics confidential
# Reproduction and Communication of this document is strictly prohibited unless
# specifically authorized in writing by STMicroelectronics.
#
#
################################################################################
#
# Alternatively, VL53L8 Kernel Driver may be distributed under the terms of
# 'BSD 3-clause "New" or "Revised" License', in which case the following
# provisions apply instead of the ones mentioned above :
#
################################################################################
#
# License terms: BSD 3-clause "New" or "Revised" License.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
# list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors
# may be used to endorse or promote products derived from this software
# without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#
###############################################################################/

# Parameters
MODULE := stmvl53l8
CONFIG_STMVL53L8 := $(CONFIG_SENSORS_VL53L8)

# Configure the build type : RELEASE, DEBUG
BUILD_TYPE = RELEASE
#BUILD_TYPE = DEBUG

# Sets the servicing mode to use i.e. INTERRUPT or DEFAULT
RANGE_SERVICING = INTERRUPT


# Set to TRUE to enable TCDM dump
TCDM_DUMP_ENABLE = FALSE

# Set to TRUE to enable rad2perp calibration
RAD2PERP_CAL_ENABLE = FALSE

# Set to TRUE to enable code for internal testing
LEGACY_CODE = FALSE

# Set to TRUE to enable logging functions.
LOG_ENABLE = FALSE

# Set to TRUE to enable force error.
FORCE_ERROR = FALSE

# Set to TRUE to enable legacy results data. Only use if legacy config's available

LEGACY_RESULTS_DATA = FALSE

# Set to TRUE to enable legacy results data. Only use if legacy config's available
EXTENDED_RESULTS_DATA = TRUE

KDIR = /lib/modules/$(shell uname -r)/build

ccflags-y += -I$(srctree)/drivers/sensors/vl53l8/inc
ccflags-y += -I$(srctree)/drivers/sensors/vl53l8/bare_driver/common/inc
ccflags-y += -I$(srctree)/drivers/sensors/vl53l8/bare_driver/api/inc
ccflags-y += -I$(srctree)/drivers/sensors/vl53l8/bare_driver/dci/inc
ccflags-y += -I$(srctree)/drivers/sensors/vl53l8/platform/inc

ccflags-y += -Wall -Werror -Wno-missing-braces

KERNEL_DRIVER_OBJS = \
	src/vl53l8_k_module.o \
	src/vl53l8_k_ioctl_controls.o \
	src/vl53l8_k_gpio_utils.o \
	src/vl53l8_k_error_converter.o \
	src/vl53l8_k_range_wait_handler.o \
	src/vl53l8_k_glare_filter.o

PLATFORM_OBJS = \
	platform/src/vl53l5_platform.o \
	platform/src/vl53l5_platform_init.o \
	platform/src/vl53l5_platform_maths.o \
	platform/src/vl53l5_platform_log.o

BARE_DRIVER_OBJS = \
	bare_driver/api/src/vl53l5_api_core.o \
	bare_driver/api/src/vl53l5_api_power.o \
	bare_driver/api/src/vl53l5_api_ranging.o \
	bare_driver/api/src/vl53l5_api_range_decode.o \
	bare_driver/api/src/vl53l5_api_calibration_decode.o\
	bare_driver/dci/src/vl53l5_core_decode.o\
	bare_driver/dci/src/vl53l5_dci_core.o \
	bare_driver/dci/src/vl53l5_dci_decode.o \
	bare_driver/dci/src/vl53l5_dci_helpers.o \
	bare_driver/dci/src/vl53l5_dci_ranging.o \
	bare_driver/dci/src/vl53l5_dci_utils.o \
	bare_driver/dci/src/vl53l5_decode_switch.o\
	bare_driver/dci/src/vl53l5_calibration_decode.o \
	bare_driver/dci/src/page_map_switch.o \
	bare_driver/common/src/vl53l5_checks.o \
	bare_driver/common/src/vl53l5_commands.o \
	bare_driver/common/src/vl53l5_error_handler.o \
	bare_driver/common/src/vl53l5_load_firmware.o \
	bare_driver/common/src/vl53l5_register_utils.o \
	bare_driver/common/src/vl53l5_rom_boot.o

ifeq "$(BUILD_TYPE)" "DEBUG"
  $(warning BUILD_TYPE=$(BUILD_TYPE))
  ccflags-y += -DDEBUG -DVL53L5_LOG_ENABLE
endif

ifeq "$(BUILD_TYPE)" "RELEASE"
  $(warning BUILD_TYPE=$(BUILD_TYPE))
endif

ifeq "$(RANGE_SERVICING)" "INTERRUPT"
  $(warning RANGE_SERVICING=$(RANGE_SERVICING))
  ccflags-y	+= -DVL53L8_INTERRUPT
  KERNEL_DRIVER_OBJS += src/vl53l8_k_interrupt.o
endif

ifeq "$(LOG_ENABLE)" "TRUE"
  $(warning LOG_ENABLE=$(LOG_ENABLE))
  ccflags-y += -DVL53L8_KERNEL_LOG
endif

ifeq "$(TCDM_DUMP_ENABLE)" "TRUE"
  $(warning TCDM_DUMP_ENABLE=$(TCDM_DUMP_ENABLE))
  ccflags-y += -DVL53L5_TCDM_DUMP
  BARE_DRIVER_OBJS += bare_driver/common/src/vl53l5_tcdm_dump.o
endif

ifeq "$(LEGACY_CODE)" "TRUE"
  $(warning LEGACY_CODE=$(LEGACY_CODE))
  ccflags-y += -DSTM_VL53L8_SUPPORT_LEGACY_CODE
else
  ccflags-y += -DSS_SUPPORT_SEC_CODE
endif

ifeq "$(FORCE_ERROR)" "TRUE"
  $(warning FORCE_ERROR=$(FORCE_ERROR))
  ccflags-y += -DVL53L8_FORCE_ERROR_COMMAND
endif

ifeq "$(RAD2PERP_CAL_ENABLE)" "TRUE"
  $(warning RAD2PERP_CAL_ENABLE=$(RAD2PERP_CAL_ENABLE))
  ccflags-y += -DVL53L8_RAD2PERP_CAL
endif

ifeq "$(LEGACY_RESULTS_DATA)" "TRUE"
  $(warning LEGACY_RESULTS_DATA=$(LEGACY_RESULTS_DATA))
  BARE_DRIVER_OBJS +=  \
	bare_driver/dci/src/vl53l5_results_decode.o
  ccflags-y += -I$(srctree)/drivers/sensors/vl53l8/bare_driver/results/inc
  ccflags-y += -DVL53L5_RESULTS_DATA_ENABLED
endif

ifeq "$(EXTENDED_RESULTS_DATA)" "TRUE"
  $(warning EXTENDED_RESULTS_DATA=$(EXTENDED_RESULTS_DATA))
  BARE_DRIVER_OBJS += \
	bare_driver/patch/src/vl53l5_patch_api_core.o \
	bare_driver/patch/src/vl53l5_patch_core_decode.o \
	bare_driver/patch/src/vl53l5_patch_decode_switch.o \
	bare_driver/patch/src/vl53l5_tcpm_patch_0_core_decode.o \
	bare_driver/patch/src/vl53l5_tcpm_patch_0_decode_switch.o \
	bare_driver/patch/src/vl53l5_tcpm_patch_0_results_decode.o \
	bare_driver/patch/src/vl53l5_tcpm_patch_1_core_decode.o \
	bare_driver/patch/src/vl53l5_tcpm_patch_1_decode_switch.o
  ccflags-y += -I$(srctree)/drivers/sensors/vl53l8/bare_driver/patch/inc
  ccflags-y += -DVL53L5_PATCH_DATA_ENABLED
endif

obj-$(CONFIG_STMVL53L8)	+= $(MODULE).o

# # Kernel level objects
$(MODULE)-objs +=	$(KERNEL_DRIVER_OBJS)
$(MODULE)-objs +=	$(BARE_DRIVER_OBJS)
$(MODULE)-objs +=	$(PLATFORM_OBJS)

all:
	make -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean

# Insert the module using insmod with the configured module parameters
insert:
	sudo insmod $(MODULE).ko

dtb:
	-sudo dtoverlay -r $(MODULE)
	-sudo rm -f /boot/overlays/$(MODULE).dtbo
	dtc -@ -I dts -O dtb -o $(MODULE)_spi.dtbo $(MODULE)_spi.dts
	sudo cp $(MODULE)_spi.dtbo /boot/overlays/$(MODULE).dtbo
	sudo dtoverlay $(MODULE)

dtb_gpio:
	-sudo dtoverlay -r $(MODULE)
	-sudo rm -f /boot/overlays/$(MODULE).dtbo
	dtc -@ -I dts -O dtb -o $(MODULE)_spi_gpio.dtbo $(MODULE)_spi_gpio.dts
	sudo cp $(MODULE)_spi_gpio.dtbo /boot/overlays/$(MODULE).dtbo
	sudo dtoverlay $(MODULE)

clean_dtb:
	sudo rm -f $(MODULE).dtbo
	sudo rm -f /boot/overlays/$(MODULE).dtbo
	sudo dtoverlay -r $(MODULE)

# Remove the module using rmmod
remove:
	-sudo rmmod $(MODULE)
