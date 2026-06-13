AM_CPPFLAGS := -I $(PKG_CONFIG_SYSROOT_DIR)/usr/include/agm/ -I ${top_srcdir}/src
bin_PROGRAMS :=  agm_ipc_test
agm_ipc_test_SOURCES   = ${top_srcdir}/src/agm_test.c
agm_ipc_test_CPPFLAGS := $(AM_CPPFLAGS)
agm_ipc_test_LDADD    = -lagmclient

bin_PROGRAMS +=  agmtest
agmtest_SOURCES   = ${top_srcdir}/src/agm_test.c
agmtest_CPPFLAGS := $(AM_CPPFLAGS)
agmtest_LDADD    = -lagm
