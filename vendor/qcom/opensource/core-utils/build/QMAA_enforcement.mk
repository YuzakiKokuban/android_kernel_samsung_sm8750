# Copyright (c) 2018, The Linux Foundation. All rights reserved.

.PHONY:qmaa_enforcement

qmaa_enforcement:
	vendor/qcom/opensource/core-utils/build/QMAA_enforcement \
		"$(QMAA_HAL_LIST)" "$(QMAA_ENABLED_HAL_MODULES)"

ifeq ($(TARGET_USES_QMAA),true)
droidcore:qmaa_enforcement
droidcore-unbundled:qmaa_enforcement
endif
