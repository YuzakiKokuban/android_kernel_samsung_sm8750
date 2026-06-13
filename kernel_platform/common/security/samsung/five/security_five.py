# SPDX-License-Identifier: GPL-2.0

config = {
    "header": {
        "uuid": "bb0496e5-b4ec-4467-b49d-944423738954",
        "type": "SECURITY",
        "vendor": "SAMSUNG",
        "product": "five",
        "variant": "five",
        "name": "five",
    },
    "build": {
        "path": "security/five",
        "file": "security_five.py",
        "location": [
            {
                "src": "*.c *.h *.S *.der Makefile:update Kconfig:update",
                "dst": "security/samsung/five/",
            },
            {
                "src": "gki/*.c gki/*.h",
                "dst": "security/samsung/five/gki/",
            },
            {
                "src": "s_os/*.c s_os/*.h",
                "dst": "security/samsung/five/s_os/",
            },
        ],
    },
    "features": [
        {
            "label": "General",
            "type": "boolean",
            "configs": {
                "True": [],
                "False": [],
            },
            "value": True,
        }
    ],
    "kunit_test": {
        "build": {
            "location": [
                {
                    "src": "kunit_test/*.c kunit_test/*.h kunit_test/Makefile:cp",
                    "dst": "security/samsung/five/kunit_test/",
                }
            ],
        },
        "features": [
            {
                "label": "default",
                "configs": {
                    "True": [
                        "CONFIG_FIVE=y",
                        "CONFIG_FIVE_GKI_10=y",
                        "CONFIG_CRC16=y",
                        "CONFIG_SECURITY=y",
                        "CONFIG_NET=y",
                        "CONFIG_AUDIT=y",
                        "CONFIG_AUDITSYSCALL=y",
                        "CONFIG_MD=y",
                        "CONFIG_BLK_DEV_DM=y",
                        "CONFIG_BLK_DEV_LOOP=y",
                        "CONFIG_DM_VERITY=y",
                        "CONFIG_CRYPTO_SHA256=y",
                        "CONFIG_CRYPTO_SHA512=y",
                        "CONFIG_FIVE_DEBUG=y",
                        "CONFIG_FIVE_AUDIT_VERBOSE=y",
                        "CONFIG_DEBUG_KERNEL=y",
                        "CONFIG_DEBUG_INFO=y"
                    ],
                    "False": [],
                },
                "type": "boolean",
           },
        ]
    },
}


def load():
    return config


if __name__ == "__main__":
    print(load())
