obj-$(CONFIG_KUNIT) +=			kunit.o

kunit-objs +=				test.o \
					resource.o \
					static_stub.o \
					string-stream.o \
					assert.o \
					try-catch.o \
					executor.o \
					attributes.o

ifeq ($(CONFIG_KUNIT)$(CONFIG_SEC_KUNIT),yy)
kunit-objs +=				mock.o \
					common-mocks.o \
					kunit-stream.o \
					strerror.o
endif

ifeq ($(CONFIG_KUNIT_DEBUGFS),y)
kunit-objs +=				debugfs.o
endif

# KUnit 'hooks' are built-in even when KUnit is built as a module.
obj-y +=				hooks.o

obj-$(CONFIG_KUNIT_TEST) +=		kunit-test.o
ifeq ($(CONFIG_KUNIT)$(CONFIG_SEC_KUNIT),yy)
obj-$(CONFIG_KUNIT_TEST) +=		mock-test.o \
					mock-macro-test.o \
					strerror-test.o
endif

# string-stream-test compiles built-in only.
ifeq ($(CONFIG_KUNIT_TEST),y)
obj-$(CONFIG_KUNIT_TEST) +=		string-stream-test.o
endif

obj-$(CONFIG_KUNIT_EXAMPLE_TEST) +=	kunit-example-test.o
obj-$(CONFIG_KUNIT_MANAGER) +=		kunit_manager.o kunit_notifier.o
