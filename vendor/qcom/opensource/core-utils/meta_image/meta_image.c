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

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <sys/stat.h>
#include "meta_format.h"

#define MAX_IMAGES           32
#define IMG_OFFSET_ALIGNMENT 4096

enum MetaImage
{
    undefined,
    bootloader,
    radio
};
enum MetaImage meta_image = undefined;

meta_header_t      meta_header;
img_header_entry_t img_header[MAX_IMAGES];
size_t             img_padding[MAX_IMAGES];
char               zeropad[IMG_OFFSET_ALIGNMENT];

struct device_info di;

void usage()
{
    printf("\nUsage:\n");
    printf("meta_image -o <output_file> -v <version> <ptn1> <file1> [[<ptn2> <file2>] ... ]\n");
    printf("  max %d '<ptn> <file>' pairs\n", MAX_IMAGES);
    printf("eg: meta_image sbl1 sbl1.mbn rpm rpm.mbn tz tz.mbn hyp hyp.mbn aboot emmc_appsboot.mbn -o bootloader.img -v M8916AAAAAULGD21210017.1\n");
}

void read_devinfo ()
{
    FILE *fin = fopen("devinfo.bin", "rb");
    size_t opsize;

    if (!fin) return;

    opsize = fread(&di, sizeof(di), 1, fin);
    fclose(fin);

    if ((opsize != 1) || (memcmp(di.magic, DEVICE_MAGIC, DEVICE_MAGIC_SIZE) != 0)) {
        printf("Corrupted devinfo - using default\n");
        memset(&di, 0, sizeof(di));
    }
}

void write_devinfo ()
{
    FILE *fout;
    size_t opsize;

    /* If image exists, just update it */
    read_devinfo();

    /* No devinfo.bin, create new one */
    memcpy(di.magic, DEVICE_MAGIC, DEVICE_MAGIC_SIZE);
    di.is_unlocked = 1;
    di.is_tampered = 0;
    di.is_verified = 0;
    di.charger_screen_enabled = 1;
    memset(di.display_panel, 0, sizeof(di.display_panel));

    if (meta_image == bootloader)
        memcpy(di.bootloader_version, meta_header.img_version, sizeof(di.bootloader_version));
    else if (meta_image == radio)
        memcpy(di.radio_version, meta_header.img_version, sizeof(di.radio_version));

    fout = fopen("devinfo.bin", "wb");
    opsize = fwrite(&di, sizeof(di), 1, fout);
    if (opsize != 1) {
        printf("Error: failed to write to devinfo.bin\n");
    }
    fclose(fout);
}

int extract_images (char *file_path)
{
    FILE *fp = NULL;
    FILE *fout = NULL;
    void *buffer;
    int i, images = 0;
    size_t opsize;

    printf ("Extracting images...\n");

    if(strstr(file_path, "bootloader"))
        meta_image = bootloader;
    else
        meta_image = radio;

    if(file_path != NULL) {
        fp = fopen(file_path, "rb");
        if (!fp) {
            printf("image file not found\n");
            return 1;
        }

        opsize = fread(&meta_header, sizeof(meta_header), 1, fp);
        if (opsize == 1)
            opsize = fread(&img_header, sizeof(img_header), 1, fp);
        if (opsize != 1)
            goto corrupted;

        meta_header.img_version[MAX_VERSION_LEN-1] = '\0';
        printf("Image version: %s\n", meta_header.img_version);

        if ((meta_header.meta_hdr_sz != sizeof(meta_header)) ||
            (meta_header.img_hdr_sz != sizeof(img_header))) {
            printf("Incompatible meta image format\n");
            goto corrupted;
        }

        images = meta_header.img_hdr_sz / sizeof(img_header_entry_t);
        for(i=0; i<images; i++) {
            if((img_header[i].ptn_name[0] == '\0') ||
                (img_header[i].start_offset == 0) ||
                (img_header[i].size == 0))
               break;

            printf("%16s %8d %8d\n", img_header[i].ptn_name, img_header[i].start_offset, img_header[i].size);

            fout = fopen(strcat(img_header[i].ptn_name,".bin"), "wb");
            if (!fout) {
                printf("Unable to write output file\n");
                goto corrupted;
            }
            fseek(fp, img_header[i].start_offset, SEEK_SET);
            buffer = malloc(img_header[i].size);
            if (!buffer) {
                printf("Error: out of memory\n");
                goto corrupted;
            }
            opsize = fread(buffer, img_header[i].size, 1, fp);
            if (opsize != 1) {
                printf("Failed to read input image for %s partition\n", img_header[i].ptn_name);
                goto corrupted;
            }
            opsize = fwrite(buffer, img_header[i].size, 1, fout);
            if (opsize != 1) {
                printf("Failed to write output image %s.bin\n", img_header[i].ptn_name);
                goto corrupted;
            }
            fclose(fout);
        }
        fclose(fp);
    }

    write_devinfo();

    return 0;

corrupted:
    fclose(fp);
    if (fout) fclose(fout);
    printf("Corrupted file\n");
    return 1;
}

int main(int argc, char *argv[])
{
    int i, entry=0;
    int images=0;
    int start_offset = sizeof(meta_header_t) + sizeof(img_header);
    struct stat st;
    FILE *f[MAX_IMAGES] = {NULL}, *meta_image = NULL;
    void *buffer;
    size_t opsize;

    meta_header.magic = META_HEADER_MAGIC;
    meta_header.major_version = 1;
    meta_header.minor_version = 0;
    meta_header.meta_hdr_sz = sizeof(meta_header);
    meta_header.img_hdr_sz = sizeof(img_header);

    for (i=1; i<argc; i++) {

        if (strcmp(argv[i], "-o") == 0)  {
            /* Output file name */
            meta_image = fopen(argv[++i], "wb");
        } else if (strcmp(argv[i], "-v") == 0)  {
            /* Image version string */
            strncpy(meta_header.img_version, argv[++i], 32);
        } else if (strcmp(argv[i], "-x") == 0)  {
            /* Special case - extract images from meta_image */
            return extract_images(argv[++i]);
        } else {
            /* Populate entry in img_header for ptn name, size & offset */
            if (entry > MAX_IMAGES) {
                printf("Error: more than %d partitions specified\n", MAX_IMAGES);
                goto errorhandler;
            }
            strncpy(img_header[entry].ptn_name, argv[i], MAX_GPT_NAME_SIZE-1);

            img_padding[entry] = IMG_OFFSET_ALIGNMENT - (start_offset % IMG_OFFSET_ALIGNMENT);
            if (img_padding[entry] == IMG_OFFSET_ALIGNMENT)
                img_padding[entry] = 0;
            start_offset += img_padding[entry];

            img_header[entry].start_offset = start_offset;

            i++;
            if (stat(argv[i], &st) == -1) {
                printf("Error: failed to stat input image file %s\n", argv[i]);
                goto errorhandler;
            }
            img_header[entry].size = st.st_size;
            start_offset += st.st_size;
            f[entry] = fopen(argv[i], "rb");
            entry++;
            images++;
        }
    }

    if(meta_image == NULL) {
        printf("Error: output file not specificed\n");
        usage();
        exit(-1);
    }

    printf ("Generating meta image from %d images...\n", images);

    opsize = fwrite (&meta_header, sizeof(meta_header), 1, meta_image);
    if (opsize == 1)
        opsize = fwrite (img_header, sizeof(img_header), 1, meta_image);
    if (opsize != 1) {
        printf("Error: failed to write to output meta image file\n");
        goto errorhandler;
    }

    printf ("\n          ptn_name start_offset  size\n");

    for (i=0; i<images; i++) {
        printf ("  %16s %8d %8d\n", img_header[i].ptn_name,
                                    img_header[i].start_offset,
                                    img_header[i].size);

        buffer = malloc(img_header[i].size);
        if (!buffer) {
            printf("Error: out of memory\n");
            goto errorhandler;
        }
        opsize = fread(buffer, img_header[i].size, 1, f[i]);
        if (opsize != 1) {
            printf("Error: failed to read from source image %s\n", img_header[i].ptn_name);
            goto errorhandler;
        }
        if (img_padding[i] != 0)
            opsize = fwrite(zeropad, img_padding[i], 1, meta_image);
        if (opsize == 1)
            opsize = fwrite (buffer, img_header[i].size, 1, meta_image);
        if (opsize != 1) {
            printf("Error: failed to write to output meta image\n");
            goto errorhandler;
        }
        fclose(f[i]);
        free(buffer);
    }

    printf ("Done.\n");
    fclose(meta_image);
    return 0;

errorhandler:
    for (i=0; i < MAX_IMAGES; i++) {
        if (!f[i]) break;
        fclose(f[i]);
    }
    fclose(meta_image);
    return 1;
}
