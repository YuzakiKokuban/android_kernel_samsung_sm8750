/*
 * Copyright (c) 2013-2016, 2020-2021, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Changes from Qualcomm Innovation Center are provided under the following license:
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef VERIFY_PRINT_ERROR
#define VERIFY_PRINT_ERROR
#endif

#define VERIFY_PRINT_INFO 0

#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
#include <unistd.h>
#include <errno.h>
#include "verify.h"
#include "AEEStdErr.h"
#include <cutils/properties.h>

#ifndef ADSP_DEFAULT_LISTENER_NAME
#define ADSP_DEFAULT_LISTENER_NAME "libadsp_default_listener.so"
#endif
#ifndef ADSP_LIBHIDL_NAME
#define ADSP_LIBHIDL_NAME "libhidlbase.so"
#endif

typedef int (*adsp_default_listener_start_t)(int argc, char *argv[]);

#ifndef LINUX_ENABLED
#define ADSP_LIB_DIRECTORY_PATH "/data/vendor/audio_dsp"

static void set_adsp_lib_path()
{
    errno = 0;
    if (access(ADSP_LIB_DIRECTORY_PATH, F_OK) == 0) {
        if (setenv("DSP_LIBRARY_PATH", ADSP_LIB_DIRECTORY_PATH, 1)) {
            VERIFY_EPRINTF("setting DSP_LIBRARY_PATH failed: %s", strerror(errno));
        } else {
            VERIFY_IPRINTF("DSP_LIBRARY_PATH set to %s, %s", getenv("DSP_LIBRARY_PATH"), strerror(errno));
        }
    } else {
        VERIFY_EPRINTF("%s not found: %s", ADSP_LIB_DIRECTORY_PATH, strerror(errno));
    }
}
#else
static void set_adsp_lib_path() {}
#endif


int main(int argc, char *argv[]) {

    int nErr = 0;
    void *adsphandler = NULL, *libhidlbaseHandler = NULL;
    adsp_default_listener_start_t listener_start;
    char bootmode[PROPERTY_VALUE_MAX];

    VERIFY_EPRINTF("audio adsp daemon starting");
    if (property_get("ro.bootmode", bootmode, NULL)) {
        VERIFY_EPRINTF("property ro.bootmode is %s", bootmode);
        if (!strncmp(bootmode, "charger", 7)) {
            VERIFY_EPRINTF("device in charging, exit daemon");
            return nErr;
        }
    }
    if(NULL != (libhidlbaseHandler = dlopen(ADSP_LIBHIDL_NAME, RTLD_NOW))) {
        set_adsp_lib_path();
        while (1) {
            if(NULL != (adsphandler = dlopen(ADSP_DEFAULT_LISTENER_NAME, RTLD_NOW))) {
                if(NULL != (listener_start =
                    (adsp_default_listener_start_t)dlsym(adsphandler,
                    "adsp_default_listener_start"))) {
                    VERIFY_IPRINTF("adsp_default_listener_start called");
                    nErr = listener_start(argc, argv);
                }
                if(0 != dlclose(adsphandler)) {
                    VERIFY_EPRINTF("dlclose failed");
                }
            } else {
                VERIFY_EPRINTF("audio adsp daemon error %s", dlerror());
            }
            if (nErr == AEE_ECONNREFUSED) {
                VERIFY_EPRINTF("fastRPC device driver is disabled, retrying...");
            }
            VERIFY_EPRINTF("audio adsp daemon will restart after 25ms...");
            usleep(25000);
        }
        if(0 != dlclose(libhidlbaseHandler)) {
            VERIFY_EPRINTF("libhidlbase dlclose failed");
        }
    }
    VERIFY_EPRINTF("audio adsp daemon exiting %x", nErr);
    return nErr;
}
