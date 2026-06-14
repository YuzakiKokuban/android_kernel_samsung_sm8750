TARGET = wez02

$(TARGET)-objs	:= wacom_i2c.o wacom_core.o wacom_sec.o wacom_elec.o wez02_flash.o
obj-$(CONFIG_INPUT_WACOM_WEZ02) += $(TARGET).o

ccflags-y += -Wformat
