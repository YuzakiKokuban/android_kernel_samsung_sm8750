
.PHONY:vendor_prop_context_restriction_enforcement

vendor_prop_context_restriction_enforcement:
	python vendor/qcom/opensource/core-utils/build/vendor_prop_context_restriction.py --m error

ifneq ($(TARGET_BOARD_PLATFORM), qssi)
droidcore:vendor_prop_context_restriction_enforcement
droidcore-unbundled:vendor_prop_context_restriction_enforcement
endif
