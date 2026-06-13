/*
** Copyright (c) 2019, 2021, The Linux Foundation. All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above
**     copyright notice, this list of conditions and the following
**     disclaimer in the documentation and/or other materials provided
**     with the distribution.
**   * Neither the name of The Linux Foundation nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
** WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
** ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
** BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
** BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
** WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
** OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
** IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
*
*  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
*  SPDX-License-Identifier: BSD-3-Clause-Clear
**/

#include <errno.h>
#include <expat.h>
#include <pthread.h>
#include <snd-card-def.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <agm/agm_list.h>

#define MAX_PATH 256
#define BUF_SIZE 1024


struct snd_prop_val_pair {
    char *prop;
    char *val;
    struct listnode list_node;
};

struct snd_dev_def {
    enum snd_node_type node_type;

    struct snd_dev_def_card *card;

    /* Mandatory properties from XML */
    unsigned int device;
    int type;
    char *name;
    char *so_name;

    struct listnode list_node;

    /* List of custom properties */
    struct listnode prop_val_list;
};

struct snd_dev_def_card {
    unsigned int card;
    int type;
    char *name;

    int refcnt;
    struct listnode list_node;
    /* child device details */
    struct listnode pcm_devs_list;
    struct listnode mixer_devs_list;
    struct listnode compr_devs_list;
};

static struct listnode snd_card_list;
static bool snd_card_list_init = false;
static pthread_rwlock_t snd_rwlock = PTHREAD_RWLOCK_INITIALIZER;

typedef enum {
    TAG_ROOT,
    TAG_CARD,
    TAG_DEVICE,
    TAG_PLUGIN,
    TAG_DEV_PROPS,
} snd_card_defs_xml_tags_t;

struct xml_userdata {
    char data_buf[BUF_SIZE];
    size_t offs;

    unsigned int card;
    char *card_name;
    struct snd_dev_def_card *cur_card_def;
    struct snd_dev_def *cur_dev_def;
    bool card_found;
    bool card_parsed;
    snd_card_defs_xml_tags_t current_tag;
};

static void snd_process_data_buf(struct xml_userdata *data, const XML_Char *tag_name);
static void snd_dev_def_init(struct xml_userdata *data, const XML_Char *tag_name, enum snd_node_type node_type);

static void snd_reset_data_buf(struct xml_userdata *data)
{
    data->offs = 0;
    data->data_buf[data->offs] = '\0';
}

static void snd_dev_set_type(struct snd_dev_def *dev_def, int type)
{
    if (dev_def)
        dev_def->type = type;
}

static void snd_start_tag(void *userdata, const XML_Char *tag_name,
                      const XML_Char **attr)
{
    struct xml_userdata *data = (struct xml_userdata *)userdata;
    enum snd_node_type node_type = -1;

    if (data->card_parsed)
        return;

    snd_reset_data_buf(data);

    if (!strcmp(tag_name, "card"))
        data->current_tag = TAG_CARD;

    if (!strcmp(tag_name, "pcm-device")) {
        data->current_tag = TAG_DEVICE;
        node_type = SND_NODE_TYPE_PCM;
    } else if (!strcmp(tag_name, "compress-device")) {
        data->current_tag = TAG_DEVICE;
        node_type = SND_NODE_TYPE_COMPR;
    } else if (!strcmp(tag_name, "mixer")) {
        data->current_tag = TAG_DEVICE;
        node_type = SND_NODE_TYPE_MIXER;
    } else if (strstr(tag_name, "plugin")) {
        snd_dev_set_type(data->cur_dev_def, SND_NODE_DEV_TYPE_PLUGIN);
        data->current_tag = TAG_PLUGIN;
    } else if (!strcmp(tag_name, "props")) {
        data->current_tag = TAG_DEV_PROPS;
    }

    if (data->current_tag != TAG_CARD && !data->card_found)
        return;

    snd_dev_def_init(data, tag_name, node_type);
}

static void snd_end_tag(void *userdata, const XML_Char *tag_name)
{
    struct xml_userdata *data = (struct xml_userdata *)userdata;

    if (data->card_parsed)
        return;
    if (data->current_tag != TAG_CARD && !data->card_found)
        return;

    snd_process_data_buf(data, tag_name);
    snd_reset_data_buf(data);
    if (!strcmp(tag_name, "mixer") || !strcmp(tag_name, "pcm-device") || !strcmp(tag_name, "compress-device"))
        data->current_tag = TAG_CARD;
    else if (strstr(tag_name, "plugin") || !strcmp(tag_name, "props"))
        data->current_tag = TAG_DEVICE;
    else if(!strcmp(tag_name, "card")) {
        data->current_tag = TAG_ROOT;
        if (data->card_found)
            data->card_parsed = true;
    }
}

static void snd_data_handler(void *userdata, const XML_Char *s, int len)
{
    struct xml_userdata *data = (struct xml_userdata *)userdata;

   if (len + data->offs >= sizeof(data->data_buf) ) {
       data->offs += len;
       /* string length overflow, return */
       return;
   } else {
       memcpy(data->data_buf + data->offs, s, len);
       data->offs += len;
   }
}

static void snd_dev_def_init(struct xml_userdata *data, const XML_Char *tag_name, enum snd_node_type node_type)
{
    struct snd_dev_def *dev_def = NULL;
    struct snd_dev_def_card *card_def = NULL;
    struct listnode *devs_list;

    card_def = data->cur_card_def;
    if (!card_def)
        return;

    if (node_type == SND_NODE_TYPE_PCM)
        devs_list = &card_def->pcm_devs_list;
    else if (node_type == SND_NODE_TYPE_COMPR)
        devs_list = &card_def->compr_devs_list;
    else if (node_type == SND_NODE_TYPE_MIXER)
        devs_list = &card_def->mixer_devs_list;
    else
        return;

    if (!(!strcmp(tag_name, "pcm-device") | !strcmp(tag_name, "compress-device") | !strcmp(tag_name, "mixer")))
        return;

    dev_def = calloc(1, sizeof(struct snd_dev_def));
    if (!dev_def)
        return;

    dev_def->node_type = node_type;
    dev_def->card = card_def;
    dev_def->type = SND_NODE_DEV_TYPE_HW;
    list_init(&dev_def->prop_val_list);
    list_add_tail(devs_list, &dev_def->list_node);
    data->cur_dev_def = dev_def;
}

static struct snd_dev_def_card *snd_parse_initialize_card_def(struct xml_userdata *data)
{
    struct snd_dev_def_card *card_def = NULL;

    if (data->cur_card_def)
        return data->cur_card_def;

    card_def = calloc(1, sizeof(struct snd_dev_def_card));
    if (!card_def)
        return card_def;

    data->card_found = true;
    data->cur_card_def = card_def;
    list_init(&card_def->pcm_devs_list);
    list_init(&card_def->mixer_devs_list);
    list_init(&card_def->compr_devs_list);
    return card_def;
}

static void snd_parse_card_properties(struct xml_userdata *data, const XML_Char *tag_name)
{
    struct snd_dev_def_card *card_def = NULL;
    unsigned int card;
    const char s[4] = ", ";
    char *token = NULL;
    char *tok_ptr;

    if (!strcmp(tag_name, "id")) {
        card = atoi(data->data_buf);
        if (card == data->card) {
            card_def = snd_parse_initialize_card_def(data);
            if (card_def)
                card_def->card = card;
        }
    } else if (!strcmp(tag_name, "name")) {
        card_def = data->cur_card_def;
        if (!card_def) {
            if (!data->card_name)
                return;
            token = strtok_r(data->data_buf, s, &tok_ptr);

            while (token != 0) {
                if (!strncmp(data->card_name, token, strlen(data->card_name))) {
                    card_def = snd_parse_initialize_card_def(data);
                    if (!card_def)
                        return;
                    goto card_found;
                }

                token = strtok_r(0, s, &tok_ptr);
            }
            return;
        }

card_found:
        if (token)
            strlcpy(data->data_buf, token, strlen(token) + 1);

        card_def->name = calloc(1, strlen(data->data_buf) + 1);
        if (!card_def->name)
            return;

        strlcpy(card_def->name, data->data_buf, strlen(data->data_buf) + 1);
    }
}

static void snd_parse_plugin_properties(struct xml_userdata *data, const XML_Char *tag_name)
{
    struct snd_dev_def *dev_def = NULL;

    dev_def = data->cur_dev_def;
    if (!dev_def)
        return;

    if (!strcmp(tag_name, "so-name")) {
        dev_def->so_name = calloc(1, strlen(data->data_buf) + 1);
        if (!dev_def->so_name)
            return;

        strlcpy(dev_def->so_name, data->data_buf, strlen(data->data_buf) + 1);
    }
}

static void snd_parse_device_properties(struct xml_userdata *data, const XML_Char *tag_name)
{
    struct snd_dev_def *dev_def = NULL;

    if (!strcmp(tag_name, "pcm-device") || !strcmp(tag_name, "compress-device") || !strcmp(tag_name, "mixer"))
        return;

    dev_def = data->cur_dev_def;
    if (!dev_def)
        return;

    if (!strcmp(tag_name, "id")) {
        dev_def->device = atoi(data->data_buf);
    } else if (!strcmp(tag_name, "name")) {
        dev_def->name = calloc(1, strlen(data->data_buf) + 1);
        if (!dev_def->name)
            return;

        strlcpy(dev_def->name, data->data_buf, strlen(data->data_buf) + 1);
    }
}

static void snd_parse_device_custom_properties(struct xml_userdata *data, const XML_Char *tag_name)
{
    struct snd_dev_def *dev_def = NULL;
    struct snd_prop_val_pair *pv_pair = NULL;

    if (!strcmp(tag_name, "props"))
        return;

    dev_def = data->cur_dev_def;
    if (!dev_def)
        return;

    /* parse custom properties */
    if (!strlen(data->data_buf) || !strlen(tag_name))
        return;

    pv_pair = calloc(1, sizeof(struct snd_prop_val_pair));
    if (!pv_pair)
        return;

    pv_pair->prop = calloc(1, strlen(tag_name) + 1);
    if (!pv_pair->prop) {
        free(pv_pair);
        return;
    }
    strlcpy(pv_pair->prop, tag_name, strlen(tag_name) + 1);
    pv_pair->val = calloc(1, strlen(data->data_buf) + 1);
    if (!pv_pair->val) {
        free(pv_pair->prop);
        free(pv_pair);
        return;
    }
    strlcpy(pv_pair->val, data->data_buf, strlen(data->data_buf) + 1);
    list_add_tail(&dev_def->prop_val_list, &pv_pair->list_node);
}

static void snd_process_data_buf (struct xml_userdata *data, const XML_Char *tag_name)
{
    if (data->offs <= 0)
        return;

    data->data_buf[data->offs] = '\0';

    if (data->card_parsed)
        return;

    if (data->current_tag == TAG_ROOT)
        return;

    if (data->current_tag == TAG_CARD)
        snd_parse_card_properties(data, tag_name);
    else if (data->current_tag == TAG_PLUGIN)
        snd_parse_plugin_properties(data, tag_name);
    else if (data->current_tag == TAG_DEVICE)
        snd_parse_device_properties(data, tag_name);
    else
        snd_parse_device_custom_properties(data, tag_name);
}

static void snd_free_card_devs_def(struct listnode *dev_list)
{
    struct snd_dev_def *dev_def = NULL;
    struct snd_prop_val_pair *pv_pair;
    struct listnode *pv_pair_node, *dev_node, *temp, *temp2;

    list_for_each_safe(dev_node, temp, dev_list) {
        dev_def = node_to_item(dev_node, struct snd_dev_def, list_node);
        list_remove(dev_node);
        free(dev_def->name);
        free(dev_def->so_name);
        list_for_each_safe(pv_pair_node, temp2, &dev_def->prop_val_list) {
            pv_pair = node_to_item(pv_pair_node, struct snd_prop_val_pair, list_node);

            list_remove(pv_pair_node);
            free(pv_pair->prop);
            free(pv_pair->val);
            free(pv_pair);
        }
        free(dev_def);
    }
}

static void snd_free_card_def(struct snd_dev_def_card *card_def)
{
    struct listnode *dev_list;

    if (!card_def)
        return;

    dev_list = &card_def->pcm_devs_list;
    snd_free_card_devs_def(dev_list);

    dev_list = &card_def->compr_devs_list;
    snd_free_card_devs_def(dev_list);

    dev_list = &card_def->mixer_devs_list;
    snd_free_card_devs_def(dev_list);

    free(card_def->name);

    free(card_def);
}

void *snd_card_def_get_card(unsigned int card)
{
    FILE *file;
    XML_Parser parser;
    void *buf;
    int bytes_read, len = 0;
    char *snd_card_name = NULL;
    bool card_found = false;
    struct listnode *snd_card_node, *temp;
    struct xml_userdata card_data;
    struct snd_dev_def_card *card_def = NULL;
    char filename[MAX_PATH];

    snprintf(filename, MAX_PATH, "/proc/asound/card%d/id", card);
    if (access(filename, F_OK ) != -1 ) {
        file = fopen(filename, "r");
        if (!file) {
            printf("open %s: failed\n", filename);
        } else {
            snd_card_name = calloc(1, BUF_SIZE);
            if (!snd_card_name)
                return NULL;

            if (fgets(snd_card_name, BUF_SIZE - 1, file)) {
                len = strlen(snd_card_name);
                snd_card_name[len - 1] = '\0';
                card = UINT_MAX;
            } else {
                free(snd_card_name);
                snd_card_name = NULL;
            }
            fclose(file);
        }
    }
    file = NULL;
    pthread_rwlock_wrlock(&snd_rwlock);
    if (snd_card_list_init == false) {
        list_init(&snd_card_list);
        snd_card_list_init = true;
    }
    memset(&card_data, 0, sizeof(card_data));

    list_for_each_safe(snd_card_node, temp, &snd_card_list) {
        card_def = node_to_item(snd_card_node, struct snd_dev_def_card, list_node);
        if (snd_card_name) {
            if (!strncmp(snd_card_name, card_def->name, strlen(snd_card_name)))
                card_found = true;
        } else if (card_def->card == card) {
            card_found = true;
        }
        if (card_found) {
            card_def->refcnt++;
            pthread_rwlock_unlock(&snd_rwlock);
            if (snd_card_name != NULL)
               free(snd_card_name);
            return card_def;
        }
    }

    card_def = NULL;
    /* read XML */
    file = fopen(CARD_DEF_FILE, "r");
    if (!file) {
        pthread_rwlock_unlock(&snd_rwlock);
        if (snd_card_name != NULL )
           free(snd_card_name);
        return NULL;
    }

    parser = XML_ParserCreate(NULL);
    if (!parser) {
        fclose(file);
        pthread_rwlock_unlock(&snd_rwlock);
        if (snd_card_name != NULL)
           free(snd_card_name);
        return NULL;
    }

    card_data.card = card;
    card_data.card_name = snd_card_name;
    XML_SetUserData(parser, &card_data);
    XML_SetElementHandler(parser, snd_start_tag, snd_end_tag);
    XML_SetCharacterDataHandler(parser, snd_data_handler);

    /* parse XML */
    for (;;) {
        buf = XML_GetBuffer(parser, BUF_SIZE);
        if (buf == NULL)
            goto ret;
        bytes_read = fread(buf, 1, BUF_SIZE, file);

        if (bytes_read < 0)
            goto ret;

        if (XML_ParseBuffer(parser, bytes_read,
                            bytes_read == 0) == XML_STATUS_ERROR) {
            goto ret;
        }

        if (bytes_read == 0)
            break;
    }

    card_def = card_data.cur_card_def;
    if (card_def) {
        list_add_tail(&snd_card_list, &card_def->list_node);
        card_def->refcnt++;
    }
ret:
    if (snd_card_name != NULL)
       free(snd_card_name);
    card_data.card_name = NULL;
    XML_ParserFree(parser);
    fclose(file);
    pthread_rwlock_unlock(&snd_rwlock);
    return card_def;
}

void snd_card_def_put_card(void *card_node)
{
    struct snd_dev_def_card *defs = (struct snd_dev_def_card *)card_node;
    struct snd_dev_def_card *card_def = NULL;
    struct listnode *snd_card_node, *temp;

    if (!defs)
        return;

    pthread_rwlock_wrlock(&snd_rwlock);
    list_for_each_safe(snd_card_node, temp, &snd_card_list) {
        card_def = node_to_item(snd_card_node, struct snd_dev_def_card, list_node);
        if (card_def == defs) {
            card_def->refcnt--;
            if (!card_def->refcnt) {
                list_remove(snd_card_node);
                snd_free_card_def(card_def);
            }
        }
    }
    pthread_rwlock_unlock(&snd_rwlock);
}

void *snd_card_def_get_node(void *card_node, unsigned int id, int type)
{
    struct snd_dev_def_card *card_def = (struct snd_dev_def_card *)card_node;
    struct snd_dev_def *dev_def = NULL;
    struct listnode *dev_node, *temp, *devs_list;

    if (!card_def)
        return NULL;

    if (type >= SND_NODE_TYPE_MAX)
        return NULL;

    pthread_rwlock_rdlock(&snd_rwlock);
    if (type == SND_NODE_TYPE_PCM)
        devs_list = &card_def->pcm_devs_list;
    else if (type == SND_NODE_TYPE_COMPR)
        devs_list = &card_def->compr_devs_list;
    else
        devs_list = &card_def->mixer_devs_list;

    list_for_each_safe(dev_node, temp, devs_list) {
        dev_def = node_to_item(dev_node, struct snd_dev_def, list_node);
        if (dev_def->device == id) {
            pthread_rwlock_unlock(&snd_rwlock);
            return dev_def;
        }
    }

    pthread_rwlock_unlock(&snd_rwlock);
    return NULL;
}

int snd_card_def_get_num_node(void *card_node, int type)
{
    struct snd_dev_def_card *card_def = (struct snd_dev_def_card *)card_node;
    struct listnode *temp, *dev_node, *devs_list;
    int num_devs = 0;

    if (!card_def)
        return 0;

    if (type >= SND_NODE_TYPE_MAX)
        return 0;

    pthread_rwlock_rdlock(&snd_rwlock);
    if (type == SND_NODE_TYPE_PCM)
        devs_list = &card_def->pcm_devs_list;
    else if (type == SND_NODE_TYPE_COMPR)
        devs_list = &card_def->compr_devs_list;
    else
        devs_list = &card_def->mixer_devs_list;

    list_for_each_safe(dev_node, temp, devs_list)
        num_devs++;

    pthread_rwlock_unlock(&snd_rwlock);
    return num_devs;
}

int snd_card_def_get_nodes_for_type(void *card_node, int type,
                                    void **list, int num_nodes)
{
    struct snd_dev_def_card *card_def = (struct snd_dev_def_card *)card_node;
    struct snd_dev_def *dev_def;
    struct listnode *temp, *dev_node, *devs_list;
    int num_devs = 0, i = 0;

    if (!card_def)
        return -EINVAL;

    if (type >= SND_NODE_TYPE_MAX)
        return -EINVAL;

    pthread_rwlock_rdlock(&snd_rwlock);
    if (type == SND_NODE_TYPE_PCM)
        devs_list = &card_def->pcm_devs_list;
    else if (type == SND_NODE_TYPE_COMPR)
        devs_list = &card_def->compr_devs_list;
    else
        devs_list = &card_def->mixer_devs_list;

    list_for_each_safe(dev_node, temp, devs_list)
        num_devs++;

    if (num_nodes > num_devs) {
        pthread_rwlock_unlock(&snd_rwlock);
        return -EINVAL;
    }

    list_for_each_safe(dev_node, temp, devs_list) {
        dev_def = node_to_item(dev_node, struct snd_dev_def, list_node);
        list[i++] = dev_def;
        if (i == num_nodes)
            break;
    }

    pthread_rwlock_unlock(&snd_rwlock);
    return 0;
}

int snd_card_def_get_int(void *node, const char *prop, int *val)
{
    struct snd_dev_def *dev_def = (struct snd_dev_def *)node;
    struct snd_prop_val_pair *pv_pair;
    struct listnode *pv_pair_node, *temp;
    int ret = -EINVAL;

    if (!dev_def)
        return ret;

    pthread_rwlock_rdlock(&snd_rwlock);
    if (!strcmp(prop, "type")) {
        *val = dev_def->type;
        pthread_rwlock_unlock(&snd_rwlock);
        return 0;
    } else if (!strcmp(prop, "id")) {
        *val = dev_def->device;
        pthread_rwlock_unlock(&snd_rwlock);
        return 0;
    }

    list_for_each_safe(pv_pair_node, temp, &dev_def->prop_val_list) {
        pv_pair = node_to_item(pv_pair_node, struct snd_prop_val_pair, list_node);
        if (!strcmp(pv_pair->prop, prop)) {
            *val = atoi(pv_pair->val);
            pthread_rwlock_unlock(&snd_rwlock);
            return 0;
        }
    }

    pthread_rwlock_unlock(&snd_rwlock);
    return ret;
}

int snd_card_def_get_str(void *node, const char *prop, char **val)
{
    struct snd_dev_def *dev_def = (struct snd_dev_def *)node;
    struct snd_prop_val_pair *pv_pair;
    struct listnode *pv_pair_node, *temp;
    int ret = -EINVAL;

    if (!dev_def)
        return ret;

    pthread_rwlock_rdlock(&snd_rwlock);
    if (!strcmp(prop, "so-name")) {
        if (dev_def->so_name)
            *val = dev_def->so_name;

        pthread_rwlock_unlock(&snd_rwlock);
        return 0;
    }

    if (!strcmp(prop, "name")) {
        if (dev_def->name)
            *val = dev_def->name;

        pthread_rwlock_unlock(&snd_rwlock);
        return 0;
    }

    list_for_each_safe(pv_pair_node, temp, &dev_def->prop_val_list) {
        pv_pair = node_to_item(pv_pair_node, struct snd_prop_val_pair, list_node);

        if (!strcmp(pv_pair->prop, prop)) {
            *val = pv_pair->val;
            pthread_rwlock_unlock(&snd_rwlock);
            return 0;
        }
    }

    pthread_rwlock_unlock(&snd_rwlock);
    return ret;
}
