/*
 * Copyright (c) 2015,2018, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of The Linux Foundation nor
 *       the names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior written
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef _META_FORMAT_H_
#define _META_FORMAT_H_

#include <inttypes.h>

#define META_HEADER_MAGIC  0xce1ad63c
#define MAX_GPT_NAME_SIZE  72
#define MAX_VERSION_LEN    64

typedef struct meta_header {
    uint32_t  magic;           /* 0xce1ad63c */
    uint16_t  major_version;   /* (0x1) - reject images with higher major versions */
    uint16_t  minor_version;   /* (0x0) - allow images with higer minor versions */
    char      img_version[MAX_VERSION_LEN];
                               /* Top level version for images in this meta */
    uint16_t  meta_hdr_sz;     /* size of this header */
    uint16_t  img_hdr_sz;      /* size of img_header_entry list */
} meta_header_t;

typedef struct img_header_entry {
    char      ptn_name[MAX_GPT_NAME_SIZE];  /* single-byte; rely on usage module to
                                             truncate for proper sizing */
    uint32_t  start_offset;
    uint32_t  size;
} img_header_entry_t;


#define DEVICE_MAGIC "ANDROID-BOOT!"
#define DEVICE_MAGIC_SIZE 13
#define MAX_PANEL_ID_LEN 64

struct device_info
{
    char     magic[DEVICE_MAGIC_SIZE];
    uint32_t is_unlocked;
    uint32_t is_tampered;
    uint32_t is_verified;
    uint32_t charger_screen_enabled;
    char     display_panel[MAX_PANEL_ID_LEN];
    char     bootloader_version[MAX_VERSION_LEN];
    char     radio_version[MAX_VERSION_LEN];
};

#endif
