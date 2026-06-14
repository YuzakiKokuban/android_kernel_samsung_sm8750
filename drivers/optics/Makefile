KBUILD_CFLAGS += -Wno-unused-variable -Wno-unused-function -Wno-unused-label -Wno-unused-parameter -Wno-frame-larger-than= -Wno-vla -Wno-uninitialized

obj-$(CONFIG_SENSORS_FLICKER_SELF_TEST) += flicker_test.o

KBUILD_CFLAGS += -Wno-unused-variable -Wno-unused-function -Wno-unused-label -Wno-unused-parameter -Wno-frame-larger-than= -Wno-vla -Wno-uninitialized -Wno-incompatible-function-pointer-types

stk-src += stk6d2x.c stk6d2x_fifo.c stk6d2x_sec.c common_i2c.c common_timer.c common_gpio_sec.c

flicker_sensor-y := $(stk-src:.c=.o)
flicker_sensor-$(CONFIG_FLICKER_PWM_CALIBRATION) += stk6d2x_cal.o

obj-$(CONFIG_SENSORS_STK6D2X) += flicker_sensor.o