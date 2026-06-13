dtbo-$(CONFIG_ARCH_SUN)   := sun-camera.dtbo
ifneq ($(filter ne1q% ne2q% pa1q% pa2q%, $(PROJECT_NAME)),)
dtbo-$(CONFIG_ARCH_SUN)   += pa2q/sun-camera-sensor-pa2q-r00.dtbo \
				pa2q/sun-camera-sensor-pa2q-r01.dtbo \
				pa2q/sun-camera-sensor-pa2q-r02.dtbo \
				pa2q/sun-camera-sensor-pa2q-r03.dtbo \
				pa2q/sun-camera-sensor-pa2q-r04.dtbo \
				pa2q/sun-camera-sensor-pa2q-r05.dtbo \
				pa2q/sun-camera-sensor-pa2q-r06.dtbo \
				pa2q/sun-camera-sensor-pa2q-r07.dtbo \
				pa2q/sun-camera-sensor-pa2q-r08.dtbo \
				pa2q/sun-camera-sensor-pa2q-r09.dtbo \
				pa2q/sun-camera-sensor-pa2q-r10.dtbo \
				pa2q/sun-camera-sensor-pa2q-r11.dtbo \
				pa2q/sun-camera-sensor-pa2q-r12.dtbo \
				pa2q/sun-camera-sensor-pa2q-r13.dtbo \
				pa2q/sun-camera-sensor-pa2q-r14.dtbo \
				pa2q/sun-camera-sensor-pa2q-r15.dtbo
else ifneq ($(filter ne3q% pa3q%, $(PROJECT_NAME)),)
dtbo-$(CONFIG_ARCH_SUN)   += pa3q/sun-camera-sensor-pa3q-r00.dtbo \
				pa3q/sun-camera-sensor-pa3q-r01.dtbo \
				pa3q/sun-camera-sensor-pa3q-r02.dtbo \
				pa3q/sun-camera-sensor-pa3q-r03.dtbo \
				pa3q/sun-camera-sensor-pa3q-r04.dtbo \
				pa3q/sun-camera-sensor-pa3q-r05.dtbo \
				pa3q/sun-camera-sensor-pa3q-r06.dtbo \
				pa3q/sun-camera-sensor-pa3q-r07.dtbo \
				pa3q/sun-camera-sensor-pa3q-r08.dtbo \
				pa3q/sun-camera-sensor-pa3q-r09.dtbo \
				pa3q/sun-camera-sensor-pa3q-r10.dtbo \
				pa3q/sun-camera-sensor-pa3q-r11.dtbo \
				pa3q/sun-camera-sensor-pa3q-r12.dtbo \
				pa3q/sun-camera-sensor-pa3q-r13.dtbo \
				pa3q/sun-camera-sensor-pa3q-r14.dtbo \
				pa3q/sun-camera-sensor-pa3q-r15.dtbo
else ifneq ($(filter psq%, $(PROJECT_NAME)),)
dtbo-$(CONFIG_ARCH_SUN)   += psq/sun-camera-sensor-psq-r00.dtbo
else ifneq ($(filter q7q%, $(PROJECT_NAME)),)
dtbo-$(CONFIG_ARCH_SUN)   += q7q/sun-camera-sensor-q7q-r00.dtbo \
				q7q/sun-camera-sensor-q7q-r01.dtbo
else ifneq ($(filter q7mq%, $(PROJECT_NAME)),)
dtbo-$(CONFIG_ARCH_SUN)   += q7mq/sun-camera-sensor-q7mq-r00.dtbo
else
dtbo-$(CONFIG_ARCH_SUN)   += sun-camera-sensor-mtp.dtbo \
				sun-camera-sensor-rumi.dtbo \
				sun-camera-sensor-cdp.dtbo  \
				sun-camera-sensor-qrd.dtbo
endif
