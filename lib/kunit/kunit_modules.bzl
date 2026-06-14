# List up KUnit test kernel modules.
# These modules will be combined with Lego module's Bazel files in the build script steps.

ko_list = [
    {
        "kunit_test" : [
                "lib/kunit/kunit_notifier.ko",
                "lib/kunit/kunit_manager.ko"
        ]
    }
]
