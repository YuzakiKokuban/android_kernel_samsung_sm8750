pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = sound_trigger.pc
EXTRA_DIST = $(pkgconfig_DATA)

AM_CFLAGS = -Wundef \
        -Wstrict-prototypes \
        -Wno-trigraphs \
        -g -O0 \
        -fno-inline \
        -fno-short-enums \
        -fpic \
        $(TINYALSA_CFLAGS) \
        $(HARDWARE_CFLAGS) \
        $(SYSTEM_MEDIA_CFLAGS) \
        $(AUDIO_ROUTE_CFLAGS) \
        -I ./st_extn \
        -I $(PKG_CONFIG_SYSROOT_DIR)/usr/include/audio-kernel \
        -I .

c_sources = sound_trigger_hw.c \
            sound_trigger_platform.c \
            st_session.c \
            st_hw_session_lsm.c \
            st_hw_session_gcs.c \
            st_hw_common.c

if QSTHW_API
c_sources += st_extn/st_hw_extn_intf.c
c_sources += st_extn/st_hw_extn.c
AM_CFLAGS += -DST_EXTN_ENABLED
endif

if USE_KEEP_ALIVE
AM_CFLAGS += -DUSE_KEEP_ALIVE
endif

if SOUND_TRIGGER_CPU_AFFINITY_SET
AM_CFLAGS += -DSOUND_TRIGGER_CPU_AFFINITY_SET
endif

lib_includedir = $(includedir)/sound_trigger
lib_include_HEADERS = sound_trigger_prop_intf.h sound_trigger_hw.h

lib_LTLIBRARIES = sound_trigger.primary.default.la
sound_trigger_primary_default_la_CC = @CC@
sound_trigger_primary_default_la_SOURCES = $(c_sources)
sound_trigger_primary_default_la_LIBADD = -ltinyalsa -laudioroute -lexpat -lcutils -llog -ldl
sound_trigger_primary_default_la_CFLAGS = $(AM_CFLAGS) -DPLATFORM_DEFAULT -DSOUND_TRIGGER_PLATFORM=default -D__unused=__attribute__\(\(__unused__\)\) -DUINT_MAX=\(~0U\) -Dstrlcat=g_strlcat -include glib.h -DUSE_GLIB @GLIB_CFLAGS@ -DLINUX_ENABLED -DST_EXTN_ENABLED -D_GNU_SOURCE -D__packed=__attribute__\(\(packed\)\)
sound_trigger_primary_default_la_CFLAGS += -DNDEBUG
sound_trigger_primary_default_la_LDFLAGS = -shared -module -avoid-version $(GLIB_LIBS)
