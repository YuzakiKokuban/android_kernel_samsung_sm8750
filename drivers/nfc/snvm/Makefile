#
# Makefile for Samsung Secure Element
#
ifeq ($(CONFIG_STAR_K250A_LEGO), m)
obj-$(CONFIG_STAR_K250A_LEGO) = snvm.o

snvm-y += sec_star.o \
                        protocol/ese_data.o \
			protocol/ese_memory.o \
                        protocol/ese_iso7816_t1.o \
                        hal/ese_i2c.o \
                        hal/ese_spi.o \
                        hal/ese_hal.o \
			sec_k250a.o
else
obj-$(CONFIG_STAR_K250A_LEGO) += sec_star.o \
			protocol/ese_data.o \
			protocol/ese_memory.o \
			protocol/ese_iso7816_t1.o \
			hal/ese_i2c.o \
			hal/ese_spi.o \
			hal/ese_hal.o \
			sec_k250a.o
endif
