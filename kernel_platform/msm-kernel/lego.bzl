
# SPDX-License-Identifier: GPL-2.0
# COPYRIGHT(C) 2023 Samsung Electronics Co., Ltd. All Right Reserved.

lego_module_list_first = [

]

lego_module_list_second = [

]

lego_module_list = [
"drivers/battery/charger/max77775_charger/max77775_charger.ko",
"drivers/battery/charger/max77968_charger/max77968_charger.ko",
"drivers/battery/charger/sm5443_charger/sm5443-charger.ko",
"drivers/battery/common/sb_wireless.ko",
"drivers/battery/common/sec-battery.ko",
"drivers/battery/common/sec-direct-charger.ko",
"drivers/battery/common/sec_pd.ko",
"drivers/battery/core/sb-core.ko",
"drivers/battery/fuelgauge/max77775_fuelgauge/max77775-fuelgauge.ko",
"drivers/battery/wireless/cps4038-charger.ko",
"drivers/battery/wireless/stwlc89-charger.ko",
"drivers/fingerprint/fingerprint.ko",
"drivers/fingerprint/fingerprint_sysfs.ko",
"drivers/fingerprint/qfs4008_spidev.ko",
"drivers/firmware/cirrus/cl_dsp-debugfs.ko",
"drivers/firmware/cirrus/cl_dsp.ko",
"drivers/input/misc/hall/akm/akm.ko",
"drivers/input/misc/hall/hall_ic.ko",
"drivers/input/misc/hall/hall_ic_notifier.ko",
"drivers/input/sec_input/sec_common_fn.ko",
"drivers/input/sec_input/sec_input_notifier.ko",
"drivers/input/sec_input/synaptics_s3916t/synaptics_s3916t_spi.ko",
"drivers/input/sec_input/wacom/wez02.ko",
"drivers/knox/hdm/hdm.ko",
"drivers/knox/kzt/kzt.ko",
"drivers/knox/ngksm/ngksm.ko",
"drivers/kperfmon/kperfmon.ko",
"drivers/mfd/maxim/mfd_max77775.ko",
"drivers/misc/drb/dev_ril_bridge.ko",
"drivers/mstdrv/mstdrv.ko",
"drivers/muic/common/common_muic.ko",
"drivers/nfc/nxp_combined/nfc_nxp_sec.ko",
"drivers/nfc/snvm/snvm.ko",
"drivers/optics/flicker_sensor.ko",
"drivers/optics/flicker_test.ko",
"drivers/regulator/dio8018/dio8018.ko",
"drivers/regulator/s2dos07/s2dos07.ko",
"drivers/regulator/tps62864-regulator.ko",
"drivers/samsung/pm/sec_thermistor/sec_thermistor.ko",
"drivers/sdp/sdp.ko",
"drivers/sec_panel_notifier_v2/sec_panel_notifier_v2.ko",
"drivers/sensors/vl53l8/stmvl53l8.ko",
"drivers/staging/android/switch/sec_switch_class.ko",
"drivers/sti/abc/abc.ko",
"drivers/sti/abc/abc_hub.ko",
"drivers/sti/abc/sec_abc_detect_conn.ko",
"drivers/usb/common/vbus_notifier/vbus_notifier.ko",
"drivers/usb/notify/usb_notifier.ko",
"drivers/usb/notify/usb_notify_layer.ko",
"drivers/usb/notify/usb_vendor_hook_receiver.ko",
"drivers/usb/typec/common/pdic_notifier_module.ko",
"drivers/usb/typec/manager/if_cb_manager.ko",
"drivers/usb/typec/manager/usb_typec_manager.ko",
"drivers/usb/typec/maxim/pdic_max77775.ko",
"drivers/usb/vendor_notify/usb_vendor_receiver.ko",
"drivers/uwb/uwb.ko",
"drivers/vibrator/common/inputff/sec_vibrator_inputff_module.ko",
"drivers/vibrator/common/vib_info/vibrator_vib_info.ko",
"drivers/vibrator/cs/cs40l26/input-cs40l26-i2c.ko",
"mfd_s2mpb02.ko",
"pmic_class.ko",
"s2mpb02-regulator.ko",
"s2mpb03.ko",
"sound/soc/codecs/snd-soc-cs40l26.ko"
]

lego_dtbo_list = [
"pa3q_chn_hkx_w00_r12.dtbo",
"pa3q_chn_hkx_w00_r11.dtbo",
"pa3q_chn_hkx_w00_r10.dtbo",
"pa3q_chn_hkx_w00_r08.dtbo",
"pa3q_chn_hkx_w00_r04.dtbo",
"pa3q_chn_hkx_w00_r03.dtbo"
]
lego_model = 'pa3q'