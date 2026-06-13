# Samsung sec ABC Feature
obj-$(CONFIG_SEC_ABC)		+= abc.o

# Samsung sec ABC COMMON Feature
abc-$(CONFIG_SEC_ABC_COMMON)	+= abc_common.o

abc-$(CONFIG_SEC_ABC_COMMON)		+= abc_spec_manager.o

abc-$(CONFIG_SEC_ABC_SPEC_TYPE1)		+= abc_spec_manager_type1.o

# Samsung sec ABC MOTTO Feature
abc-$(CONFIG_SEC_ABC_MOTTO)		+= abc_motto.o

# Samsung sec ABC Kunit test Feature


GCOV_PROFILE_abc_common.o := $(CONFIG_SEC_KUNIT)

GCOV_PROFILE_abc_spec_manager.o := $(CONFIG_SEC_KUNIT)

GCOV_PROFILE_abc_spec_manager_type1.o := $(CONFIG_SEC_KUNIT)

# Samsung sec ABC_HUB Feature
obj-$(CONFIG_SEC_ABC_HUB)	+= abc_hub.o

# Samsung sec ABC_HUB Core Feature
abc_hub-$(CONFIG_SEC_ABC_HUB_CORE)	+= abc_hub_core.o

# Samsung sec ABC_HUB Sub Module(Booting time check) Feature
abc_hub-$(CONFIG_SEC_ABC_HUB_BOOTC) += abc_hub_bootc.o


GCOV_PROFILE_abc_hub_core.o := $(CONFIG_SEC_KUNIT)

GCOV_PROFILE_abc_hub_bootc.o := $(CONFIG_SEC_KUNIT)

subdir-ccflags-y := -Wformat

# Samsung Connector Check Feature
obj-$(CONFIG_SEC_ABC_DETECT_CONN)	+= sec_abc_detect_conn.o
sec_abc_detect_conn-objs := connector_setup.o connector_state.o connector_disconnected_count.o
ccflags-y := -Wformat
