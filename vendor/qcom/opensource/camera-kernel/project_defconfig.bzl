load("@bazel_skylib//rules:write_file.bzl", "write_file")

#common_configs = [
#	"CONFIG_SPECTRA_ISP=y",
#	"CONFIG_SPECTRA_ICP=y",
#	"CONFIG_SPECTRA_JPEG=y",
#	"CONFIG_SPECTRA_SENSOR=y",
#	"CONFIG_SPECTRA_USE_CLK_CRM_API=y",
#	"CONFIG_SPECTRA_USE_RPMH_DRV_API=y",
#	"CONFIG_SPECTRA_LLCC_STALING=y",
#]
#
#dependency_config = [
#	"CONFIG_TARGET_SYNX_ENABLE=y",
#	"CONFIG_INTERCONNECT_QCOM=y",
#	"CONFIG_DOMAIN_ID_SECURE_CAMERA=y",
#	"CONFIG_DYNAMIC_FD_PORT_CONFIG=y",
#	"CONFIG_SECURE_CAMERA_25=y",
#	"CONFIG_MSM_MMRM=y",
#]
#
#project_configs = select({
#    # Project-specific configs
#    ":no_project": [],
#    ":pineapple": dependency_config + [
#        "CONFIG_SPECTRA_SECURE_CAMNOC_REG_UPDATE=y",
#    ],
#    ":sun": dependency_config + [
#        "CONFIG_SPECTRA_SECURE_DYN_PORT_CFG=y",
#        "CONFIG_SPECTRA_SECURE_CAMNOC_REG_UPDATE=y",
#    ],
#    ":canoe": [],
#})
#
#"""
#Return a label which defines a project-specific defconfig snippet to be
#applied on top of the platform defconfig.
#"""
#
#def get_project_defconfig(target, variant):
#    rule_name = "{}_{}_project_defconfig".format(target, variant)
#
#    write_file(
#        name = rule_name,
#        out = "{}.generated".format(rule_name),
#        content = common_configs + project_configs + [""],
#    )
#
#    return rule_name

common_configs = [
    "CONFIG_SAMSUNG_CAMERA=y",
    "CONFIG_CAMERA_FRAME_CNT_DBG=y",
    "CONFIG_CAMERA_SYSFS_V2=y",
    "CONFIG_SENSOR_RETENTION=y",
    "CONFIG_CAMERA_FRAME_CNT_CHECK=y",
    "CONFIG_SAMSUNG_FRONT_EEPROM=y",
    "CONFIG_SAMSUNG_REAR_DUAL=y",
    "CONFIG_USE_CAMERA_HW_BIG_DATA=y",
    "CONFIG_CAMERA_CDR_TEST=y",
    "CONFIG_CAMERA_HW_ERROR_DETECT=y",
    "CONFIG_SAMSUNG_DEBUG_HW_INFO=y",
    "CONFIG_SAMSUNG_ACTUATOR_READ_HALL_VALUE=y",
    "CONFIG_SAMSUNG_PMIC_FLASH=y",
    "CONFIG_SAMSUNG_FRONT_CAMERA_ACTUATOR=y",
    "CONFIG_SAMSUNG_DEBUG_SENSOR_I2C=y",
    "CONFIG_CAMERA_ADAPTIVE_MIPI=y",
    "CONFIG_SAMSUNG_LPAI_OIS=y",
]

project_configs = select({
    # Project-specific configs
    ":no_project": [],
    ":pa1q_project": common_configs + [
        "CONFIG_SEC_PA1Q_PROJECT=y",
        "CONFIG_SAMSUNG_REAR_TRIPLE=y",
    ],
    ":pa2q_project": common_configs + [
        "CONFIG_SEC_PA2Q_PROJECT=y",
        "CONFIG_SAMSUNG_REAR_TRIPLE=y",
    ],
    ":pa3q_project": common_configs + [
        "CONFIG_SEC_PA3Q_PROJECT=y",
        "CONFIG_SAMSUNG_REAR_TRIPLE=y",
        "CONFIG_SAMSUNG_REAR_QUADRA=y",
        "CONFIG_SAMSUNG_UW_CAMERA_ACTUATOR=y",
        "CONFIG_SAMSUNG_READ_BPC_FROM_OTP=y",
        "CONFIG_SAMSUNG_WACOM_NOTIFIER=y",
    ],
    ":psq_project": common_configs + [
        "CONFIG_SEC_PSQ_PROJECT=y",
    ],
    ":q7q_project": common_configs + [
        "CONFIG_SEC_Q7Q_PROJECT=y",
        "CONFIG_SAMSUNG_READ_BPC_FROM_OTP=y",
        "CONFIG_SAMSUNG_REAR_TRIPLE=y",
        "CONFIG_SAMSUNG_UW_CAMERA_ACTUATOR=y",
        "CONFIG_SAMSUNG_FRONT_TOP=y",
        "CONFIG_SAMSUNG_FRONT_TOP_EEPROM=y",
    ],
    ":q7mq_project": common_configs + [
        "CONFIG_SEC_Q7MQ_PROJECT=y",
        "CONFIG_SAMSUNG_READ_BPC_FROM_OTP=y",
        "CONFIG_SAMSUNG_REAR_TRIPLE=y",
    ],
})


"""
Return a label which defines a project-specific defconfig snippet to be
applied on top of the platform defconfig.
"""

def get_project_defconfig(target, variant):
    rule_name = "{}_{}_project_defconfig".format(target, variant)

    write_file(
        name = rule_name,
        out = "{}.generated".format(rule_name),
        content = project_configs + [""],
    )

    return rule_name
