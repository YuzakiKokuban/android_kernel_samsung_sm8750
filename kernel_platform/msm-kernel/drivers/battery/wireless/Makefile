obj-$(CONFIG_WIRELESS_CHARGER_CPS4038)	+= cps4038-charger.o

cps4038-charger-$(CONFIG_WIRELESS_CHARGER_CPS4038) += cps4038_fod.o cps4038_cmfet.o cps4038_charger.o

ccflags-y := -Wformat

GCOV_PROFILE_cps4038_charger.o		:= $(CONFIG_SEC_KUNIT)

obj-$(CONFIG_WIRELESS_CHARGER_STWLC89)	+= stwlc89-charger.o

stwlc89-charger-$(CONFIG_WIRELESS_CHARGER_STWLC89) += stwlc89_fod.o stwlc89_cmfet.o stwlc89_charger.o stwlc89_debug.o

ccflags-y := -Wformat