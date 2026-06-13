
Background:
===========

There are two versions of the ion.h UAPI header, one contained in libion, the
other in the actual kernel UAPI folder - these UAPI headers do not contain
identical definitions.  Most Android modules depend on the version of ion.h in
libion.  When Android modules (1) try to include ion.h from libion via #include
<linux/ion.h>, and (2) try to simultaneously include a UAPI header from the
kernel, they will pick up ion.h from the kernel - so, the following .mk file
would cause a compilation error:

-------------------------------------------------------------------------------
LOCAL_SHARED_LIBRARIES  += libion
LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr
LOCAL_C_INCLUDES        += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
-------------------------------------------------------------------------------

To circumvent this issue, it suffices to manually include the header paths to
libion, so that the libion header is picked up before the kernel's - this is
demonstrated in the .mk file below:

-------------------------------------------------------------------------------
LOCAL_C_INCLUDES        += system/memory/libion/include
LOCAL_C_INCLUDES        += system/memory/libion/kernel-headers

LOCAL_SHARED_LIBRARIES  += libion
LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr
LOCAL_C_INCLUDES        += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
-------------------------------------------------------------------------------

Path Change of libion in Android R, Resulting Problems:
=======================================================

The location of libion was changed in Android R, from system/core/libion to
system/memory/libion.  Ideally, we would like to insulte libion-dependent
modules (both .mk and .bp modules) from further path changes.  Additionally,
libion-dependent .bp modules that need to include the header paths to compile,
that are also shared across R and an older version of Android, cannot include
paths to both libion directories, since Soong will gripe about missing
dependencies.  That is, the following .bp would cause a compilation error:

-------------------------------------------------------------------------------
cc_binary {
    name: "i-will-break",
    srcs: ["i-will-break.c"],
    include_dirs: ["system/core/libion/kernel-headers",
                   "system/core/libion/include",
                   "system/memory/libion/kernel-headers",
                   "system/memory/libion/include"],
    shared_libs: ["libion"],
    header_libs: ["qti_kernel_headers"]
}
-------------------------------------------------------------------------------

Solution:
=========

Our solution is to provide wrappers for both .mk and .bp files, *per Android
version*, which will ensure the correct header paths are used.  That is, the
wrappers are duplicated per Android version, such that the right path is
specified by the wrapper (for a given Android version).

The .mk wrapper is a variable LIBION_HEADER_PATH_WRAPPER defined in
../AndroidBoardCommon.mk (defining it there makes the variable globally
accessible).  LIBION_HEADER_PATH_WRAPPER stores the path towards
./libion_path.mk, which in turn defines a variable LIBION_HEADER_PATHS that
contains the header paths.  We use LIBION_HEADER_PATH_WRAPPER as follows:

-------------------------------------------------------------------------------
include $(LIBION_HEADER_PATH_WRAPPER)
LOCAL_C_INCLUDES += $(LIBION_HEADER_PATHS)

LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr
LOCAL_C_INCLUDES        += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
-------------------------------------------------------------------------------

The wrapper for .bp files is a cc_defaults "libion_header_paths", placed inside
of ./Android.bp - this cc_defaults has an include_dirs property that contains
the header paths, and is used as follows:

-------------------------------------------------------------------------------
cc_binary {
    name: "i-shall-compile",
    srcs: ["i-shall-compile.c"],
    defaults: ["libion_header_paths"],
    shared_libs: ["libion"],
    header_libs: ["qti_kernel_headers"]
}
-------------------------------------------------------------------------------

