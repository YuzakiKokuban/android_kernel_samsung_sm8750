# Copyright (c) 2018, The Linux Foundation. All rights reserved.

.PHONY: qssi_violators
qssi_violators: $(PRODUCT_OUT)/module-info.json
# Remove existing QSSI violators list (if present)
	if [ -s $$OUT/QSSI_violators.txt ]; then rm $$OUT/QSSI_violators.txt; fi
# Generate QSSI violators list
	vendor/qcom/opensource/core-utils/build/QSSI_violators

# module-info.json is not included when ONE_SHOT_MAKEFILE,
# hence disable qssi_violators for that as well.
# Also, QSSI enforcement is needed only for android-P(and above) new-launch devices.
ifndef ONE_SHOT_MAKEFILE
  ifdef PRODUCT_SHIPPING_API_LEVEL
    ifneq ($(call math_gt_or_eq,$(PRODUCT_SHIPPING_API_LEVEL),28),)
      droidcore: qssi_violators
      droidcore-unbundled: qssi_violators
    endif
  else # PRODUCT_SHIPPING_API_LEVEL is undefined
    droidcore: qssi_violators
    droidcore-unbundled: qssi_violators
  endif
endif
