TARGET_I2C = synaptics_s3916t_i2c
TARGET_SPI = synaptics_s3916t_spi

$(TARGET_SPI)-objs := synaptics_spi.o synaptics_core.o synaptics_interrupt.o synaptics_fn.o synaptics_fw.o \
  			synaptics_dump.o synaptics_cmd.o synaptics_ic_setting.o
$(TARGET_I2C)-objs := synaptics_i2c.o synaptics_core.o synaptics_interrupt.o synaptics_fn.o synaptics_fw.o \
  			synaptics_dump.o synaptics_cmd.o synaptics_ic_setting.o

ifneq ($(CONFIG_SAMSUNG_PRODUCT_SHIP), y)
  $(TARGET_SPI)-objs += synaptics_vendor.o
  $(TARGET_I2C)-objs += synaptics_vendor.o
endif

obj-$(CONFIG_TOUCHSCREEN_SYNAPTICS_S3916T_SPI) += $(TARGET_SPI).o
obj-$(CONFIG_TOUCHSCREEN_SYNAPTICS_S3916T_I2C) += $(TARGET_I2C).o

ccflags-y += -Wformat
