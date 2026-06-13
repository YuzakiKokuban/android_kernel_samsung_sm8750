/*
** Copyright (c) 2020, The Linux Foundation. All rights reserved.
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
**/

/*
** Changes from Qualcomm Innovation Center are provided under the following license:
** Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted (subject to the limitations in the
** disclaimer below) provided that the following conditions are met:
**
**    * Redistributions of source code must retain the above copyright
**      notice, this list of conditions and the following disclaimer.
**
**    * Redistributions in binary form must reproduce the above
**      copyright notice, this list of conditions and the following
**      disclaimer in the documentation and/or other materials provided
**      with the distribution.
**
**    * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
**      contributors may be used to endorse or promote products derived
**      from this software without specific prior written permission.
**
** NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
** GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
** HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
** WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
** MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
** ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
** GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
** INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
** IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
** OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
** IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**/

#define LOG_TAG "agm_client_wrapper"

#include <errno.h>
#include <agm/agm_api.h>
#include <gio/gio.h>
#include "utils.h"

#define AGM_OBJECT_PATH "/org/qti/agm"
#define AGM_MODULE_IFACE "org.Qti.Agm"
#define AGM_SESSION_IFACE "org.Qti.Agm.Session"
#define AGM_DBUS_CONNECTION "org.Qti.AgmService"
#define AGM_MAX_G_OBJ_PATH 128
#define AGM_DBUS_ASYNC_CALL_TIMEOUT_MS 1000

typedef struct {
    GDBusConnection *conn;
    GDBusProxy *proxy;
    char g_obj_path[AGM_MAX_G_OBJ_PATH];
    GHashTable *ses_hash_table;
} agm_client_module_data;

typedef struct {
    GDBusConnection *conn;
    GDBusProxy *proxy;
    char *obj_path;
    uint32_t session_id;
    GThread *thread_loop;
    GMainLoop *loop;
    GList *callbacks;
    GMainLoop *ses_loop;
    GThread *ses_thread_loop;
    guint sub_id;
    GMutex mutex;
    GCond cond;
    void *buf;
    uint32_t buf_size;
} agm_client_session_data;

typedef struct {
    uint32_t session_id;
    void *client_data;
    agm_event_cb cb;
    uint32_t evt_type;
    guint sub_id_callback_event;
} agm_callback_data;

static agm_client_module_data *mdata = NULL;

static GDBusConnection *get_dbus_connection() {
    GError *error = NULL;
    GDBusConnection *conn = NULL;
    conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);

    if (conn == NULL) {
        AGM_LOGE("%s: Error in getting dbus connection: %s", __func__,
                  error->message);
        return NULL;
    }

    return conn;
}

static GDBusProxy *get_proxy_object(GDBusConnection *connection,
                                    GDBusProxyFlags flags,
                                    const gchar *name,
                                    const gchar *object_path,
                                    const gchar *interface_name) {
    GError *error = NULL;
    GDBusProxy *proxy;

    AGM_LOGD("%s\n", __func__);

    proxy = g_dbus_proxy_new_sync(connection,
                          flags,
                          NULL,
                          name,
                          object_path,
                          interface_name,
                          NULL,
                          &error);

    if (proxy == NULL)
        AGM_LOGE("%s: Error in getting dbus proxy: %s", __func__,
                  error->message);
    return proxy;
}

static int initialize_module_data() {
    int rc = 0;

    mdata = (agm_client_module_data *)
                         g_malloc0(sizeof(agm_client_module_data));
    mdata->conn = get_dbus_connection();

    if (mdata->conn == NULL) {
        AGM_LOGE("%s: Error in getting dbus connection", __func__);
        rc = -EINVAL;
        free(mdata);
        mdata = NULL;
        return rc;
    }

    mdata->proxy = get_proxy_object(mdata->conn,
                                    G_DBUS_PROXY_FLAGS_NONE,
                                    AGM_DBUS_CONNECTION,
                                    AGM_OBJECT_PATH,
                                    AGM_MODULE_IFACE);
    if (mdata->proxy == NULL) {
        AGM_LOGE("%s: Couldn't get proxy object\n", __func__);
        rc = -EINVAL;
        free(mdata);
        mdata = NULL;
        return rc;
    }

    g_snprintf(mdata->g_obj_path, AGM_MAX_G_OBJ_PATH,
               "%s", AGM_OBJECT_PATH);

    mdata->ses_hash_table = g_hash_table_new(g_direct_hash, g_direct_equal);
    return rc;
}

static void on_emit_signal_callback(GDBusConnection *conn,
                                    const gchar *sender_name,
                                    const gchar *object_path,
                                    const gchar *interface_name,
                                    const gchar *signal_name,
                                    GVariant *parameters,
                                    gpointer data) {
    agm_callback_data *cb_data = (agm_callback_data *)data;
    GVariant *array_v;
    GVariantIter arg_i;
    struct agm_event_cb_params event_params;
    struct agm_event_cb_params *event_params_l;
    gsize element_size = sizeof(guchar);
    gsize n_elements = 0;
    gconstpointer value;

    AGM_LOGD("%s\n", __func__);

    g_variant_iter_init(&arg_i, parameters);
    g_variant_iter_next(&arg_i, "u", &event_params.source_module_id);
    g_variant_iter_next(&arg_i, "u", &event_params.event_id);
    g_variant_iter_next(&arg_i, "u", &event_params.event_payload_size);
    event_params_l = (struct agm_event_cb_params*) calloc(1,
                     (sizeof(struct agm_event_cb_params) +
                     event_params.event_payload_size));
    array_v = g_variant_iter_next_value(&arg_i);
    value = g_variant_get_fixed_array(array_v, &n_elements, element_size);
    memcpy(&event_params_l->event_payload[0], value, n_elements);
    g_variant_unref(array_v);

    event_params_l->source_module_id = event_params.source_module_id;
    event_params_l->event_id = event_params.event_id;
    event_params_l->event_payload_size = event_params.event_payload_size;

    cb_data->cb(cb_data->session_id, event_params_l, cb_data->client_data);
}

static void on_emit_ses_signal_callback(GDBusConnection *conn,
                                    const gchar *sender_name,
                                    const gchar *object_path,
                                    const gchar *interface_name,
                                    const gchar *signal_name,
                                    GVariant *parameters,
                                    gpointer data) {

    agm_client_session_data *ses_data = NULL;
    GVariantIter arg_i;
    GVariant *array_v;
    gconstpointer value;
    gsize n_elements;
    gsize element_size = sizeof(guchar);
    uint32_t dir = 0;
    uint32_t status = 0;
    uint32_t session_id = 0;

    g_variant_iter_init(&arg_i, parameters);
    g_variant_iter_next(&arg_i, "u", &dir);
    g_variant_iter_next(&arg_i, "u", &status);
    g_variant_iter_next(&arg_i, "u", &session_id);

    if ((ses_data = (agm_client_session_data *)
                        g_hash_table_lookup(mdata->ses_hash_table,
                                       GINT_TO_POINTER(session_id))) == NULL) {
        AGM_LOGE("%s: Invalid session id received\n", __func__);
    }

    AGM_LOGD("Waking up session threads read/write = %d thread sts:%u, sid:%d\n",
        dir, status, session_id);

    if(!dir){
        array_v = g_variant_iter_next_value(&arg_i);
        value = g_variant_get_fixed_array(array_v, &n_elements, element_size);
        AGM_LOGD("%s:n_elements %d, element_size %d, %p\n", __func__,
            n_elements, element_size, ses_data->buf);
        if (n_elements <= ses_data->buf_size) {
            memcpy(ses_data->buf, value, n_elements);
            ses_data->buf_size = n_elements;
        } else  {
            AGM_LOGE("Insufficient bytes size to copy bytes read\n");
        }
        g_variant_unref(array_v);
    }

    g_mutex_lock(&ses_data->mutex);
    g_cond_signal(&ses_data->cond);
    g_mutex_unlock(&ses_data->mutex);
}

static void free_callbacks(agm_client_session_data *ses_data) {
    GList *node;
    agm_callback_data *cb_data;
    g_assert(mdata != NULL);

    for (node = ses_data->callbacks; node != NULL; node = node->next) {
        if (node->data != NULL) {
            cb_data = (agm_callback_data *)node->data;
            g_dbus_connection_signal_unsubscribe(mdata->conn,
                                              cb_data->sub_id_callback_event);
            free(node->data);
            node->data = NULL;
        }
    }

    g_list_free(ses_data->callbacks);
}

static int subscribe_ses_callback_event(agm_client_session_data *ses_data,
                                    bool subscribe) {
    GVariant *result;
    GError *error = NULL;
    gint ret = 0;
    GHashTableIter iter;
    gpointer key, value;
    GList *node = NULL;
    agm_callback_data *node_data = NULL;

    g_assert(mdata != NULL);

    if (subscribe) {
        AGM_LOGD("Subscribing to session events\n");
        ses_data->sub_id = g_dbus_connection_signal_subscribe(
                                          mdata->conn,
                                          NULL,
                                          AGM_SESSION_IFACE,
                                          "AgmSesEventCb",
                                          ses_data->obj_path,
                                          NULL,
                                          G_DBUS_SIGNAL_FLAGS_NONE,
                                          on_emit_ses_signal_callback,
                                          ses_data,
                                          NULL);

    } else {
        AGM_LOGD("Unsubscribing to session events\n");
        g_dbus_connection_signal_unsubscribe(mdata->conn, ses_data->sub_id);
    }

    return 0;
}

static int subscribe_callback_event(agm_client_session_data *ses_data,
                                    bool subscribe,
                                    agm_callback_data *cb_data) {
    GVariant *result;
    GError *error = NULL;
    gint ret = 0;
    GHashTableIter iter;
    gpointer key, value;
    GList *node = NULL;
    agm_callback_data *node_data = NULL;

    AGM_LOGD("%s\n", __func__);
    g_assert(mdata != NULL);
    g_assert(cb_data != NULL);

    if (subscribe) {
        cb_data->sub_id_callback_event = g_dbus_connection_signal_subscribe(
                                          mdata->conn,
                                          NULL,
                                          AGM_SESSION_IFACE,
                                          "AgmEventCb",
                                          ses_data->obj_path,
                                          NULL,
                                          G_DBUS_SIGNAL_FLAGS_NONE,
                                          on_emit_signal_callback,
                                          cb_data,
                                          NULL);

        ses_data->callbacks = g_list_prepend(ses_data->callbacks, cb_data);
    } else {
        for (node = ses_data->callbacks; node != NULL; node = node->next) {
            node_data = (agm_callback_data *)node->data;
            if (node_data != NULL) {
                if (node_data->session_id == cb_data->session_id &&
                    node_data->evt_type == cb_data->evt_type &&
                    node_data->sub_id_callback_event ==
                                             cb_data->sub_id_callback_event) {
                        g_dbus_connection_signal_unsubscribe(mdata->conn,
                                               cb_data->sub_id_callback_event);
                        ses_data->callbacks = g_list_remove(
                                                           ses_data->callbacks,
                                                           node_data);
                        free(node_data);
                        node_data = NULL;
                        break;
                }
            }
        }
    }

    return 0;
}

static void *signal_threadloop(void *cookie) {
    agm_client_session_data *ses_data = (agm_client_session_data *)cookie;

    AGM_LOGD("Enter %s %p\n", __func__, ses_data);
    if (!ses_data) {
        AGM_LOGE("Invalid thread params");
        goto exit;
    }

    ses_data->loop = g_main_loop_new(NULL, FALSE);
    AGM_LOGD("initiate main loop run for callbacks");
    g_main_loop_run(ses_data->loop);

    AGM_LOGD("out of main loop\n");
    g_main_loop_unref(ses_data->loop);

exit:
    return NULL;
}

static void *ses_signal_threadloop(void *cookie) {
    agm_client_session_data *ses_data = (agm_client_session_data *)cookie;

    AGM_LOGD("Enter %s %p\n", __func__, ses_data);
    if (!ses_data) {
        AGM_LOGE("Invalid thread params");
        goto exit;
    }

    ses_data->ses_loop = g_main_loop_new(NULL, FALSE);
    AGM_LOGE("initiate ses main loop run for callbacks");
    g_main_loop_run(ses_data->ses_loop);

    AGM_LOGE("out of ses main loop\n");
    g_main_loop_unref(ses_data->ses_loop);

exit:
    return NULL;
}


static int agm_session_deregister_cb(uint32_t session_id,
                                     enum event_type evt_type,
                                     void *client_data) {
    GVariant *value_1, *value_2, *value_3, *argument;
    GVariant *result = NULL;
    GError *error = NULL;
    int rc = 0;
    agm_client_session_data *ses_data = NULL;
    char *obj_path = NULL;
    agm_callback_data *cb_data;
    GList *node = NULL;
    uint64_t data;

    AGM_LOGD("%s\n", __func__);

    data = (uint64_t)client_data;

    value_1 = g_variant_new_uint32(session_id);
    value_2 = g_variant_new_uint32(evt_type);
    value_3 = g_variant_new_uint64(data);

    argument = g_variant_new("(@u@u@t)", value_1, value_2, value_3);

    if (mdata == NULL) {
        if ((rc = initialize_module_data()) != 0)
            return rc;
    }

    result = g_dbus_proxy_call_sync(mdata->proxy,
                                    "AgmSessionDeRegisterCb",
                                    argument,
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &error);

    if (result == NULL) {
        AGM_LOGE("%s: Error invoking AgmSessionDeRegisterCb: %s\n", __func__,
                  error->message);
        g_error_free(error);
        rc = -EINVAL;
        return rc;
    }

    if ((ses_data = (agm_client_session_data *)
                        g_hash_table_lookup(mdata->ses_hash_table,
                                       GINT_TO_POINTER(session_id))) == NULL) {
        ses_data = (agm_client_session_data *)
                                    g_malloc0(sizeof(agm_client_session_data));

        g_variant_get(result, "(o)", &ses_data->obj_path);
        ses_data->conn = mdata->conn;

        ses_data->proxy = g_dbus_proxy_new_sync(ses_data->conn,
                                G_DBUS_PROXY_FLAGS_NONE,
                                NULL,
                                AGM_DBUS_CONNECTION,
                                ses_data->obj_path,
                                AGM_SESSION_IFACE,
                                NULL,
                                &error);

        if (ses_data->proxy == NULL) {
            AGM_LOGE("%s: Error in getting dbus proxy: %s", __func__,
                      error->message);
            rc = -EINVAL;
            free(ses_data);
            ses_data = NULL;
            g_error_free(error);
            goto exit;
        }

        ses_data->session_id = session_id;
        /* add session to sessions hash table */
        g_hash_table_insert(mdata->ses_hash_table,
                            GINT_TO_POINTER
                            (ses_data->session_id),
                            ses_data);
    }

    cb_data = (agm_callback_data *)malloc(sizeof(agm_callback_data));
    cb_data->client_data = client_data;
    cb_data->session_id = session_id;
    cb_data->cb = NULL;

    if (subscribe_callback_event(ses_data, false, cb_data)) {
        AGM_LOGE("Unable to subscribe for callback event\n");
        g_error_free(error);
        g_free(ses_data);
        g_free(cb_data);
        rc = -EINVAL;
    }

    g_free(cb_data);
    cb_data = NULL;
    g_variant_unref(result);

exit:
    return rc;
}

int agm_session_register_cb(uint32_t session_id,
                            agm_event_cb cb,
                            enum event_type evt_type,
                            void *client_data) {
    GVariant *value_1, *value_2, *value_3, *argument;
    GVariant *result = NULL;
    GError *error = NULL;
    int rc = 0;
    agm_client_session_data *ses_data = NULL;
    char *obj_path = NULL;
    agm_callback_data *cb_data;
    gchar thread_name[16] = "";
    uint64_t data;

    AGM_LOGD("%s\n", __func__);

    if (cb == NULL)
        return agm_session_deregister_cb(session_id, evt_type, client_data);

    data = (uint64_t)client_data;

    value_1 = g_variant_new_uint32(session_id);
    value_2 = g_variant_new_uint32(evt_type);
    value_3 = g_variant_new_uint64(data);

    argument = g_variant_new("(@u@u@t)", value_1, value_2, value_3);

    if (mdata == NULL) {
        if ((rc = initialize_module_data()) != 0)
            return rc;
    }

    result = g_dbus_proxy_call_sync(mdata->proxy,
                                    "AgmSessionRegisterCb",
                                    argument,
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &error);

    if (result == NULL) {
        AGM_LOGE("%s: Error invoking AgmSessionRegisterCb: %s\n", __func__,
                  error->message);
        g_error_free(error);
        rc = -EINVAL;
        return rc;
    }

    if ((ses_data = (agm_client_session_data *)
                     g_hash_table_lookup(mdata->ses_hash_table,
                                     GINT_TO_POINTER(session_id))) == NULL) {
        ses_data = (agm_client_session_data *)
                                     g_malloc0(sizeof(agm_client_session_data));

        g_variant_get(result, "(o)", &ses_data->obj_path);
        ses_data->conn = mdata->conn;

        ses_data->proxy = g_dbus_proxy_new_sync(ses_data->conn,
                                G_DBUS_PROXY_FLAGS_NONE,
                                NULL,
                                AGM_DBUS_CONNECTION,
                                ses_data->obj_path,
                                AGM_SESSION_IFACE,
                                NULL,
                                &error);

        if (ses_data->proxy == NULL) {
            AGM_LOGE("%s: Error in getting dbus proxy: %s", __func__,
                      error->message);
            rc = -EINVAL;
            free(ses_data);
            ses_data = NULL;
            g_error_free(error);
            goto exit;
        }

        ses_data->session_id = session_id;
        snprintf(thread_name, sizeof(thread_name), "agm_loop_%d", session_id);
        AGM_LOGD("%s:create thread %s\n", __func__, thread_name);
        ses_data->thread_loop = g_thread_try_new(thread_name, signal_threadloop,
                                ses_data, &error);
        if (!ses_data->thread_loop) {
            rc = -EINVAL;
            AGM_LOGE("Could not create thread %s, error %s\n", thread_name,
                      error->message);
            g_error_free(error);
            g_free(ses_data);
            goto exit;
        }
        /* add session to sessions hash table */
        g_hash_table_insert(mdata->ses_hash_table,
                            GINT_TO_POINTER(ses_data->session_id),
                            ses_data);
    }

    cb_data = (agm_callback_data *)malloc(sizeof(agm_callback_data));
    cb_data->client_data = client_data;
    cb_data->session_id = session_id;
    cb_data->cb = cb;
    cb_data->evt_type = evt_type;

    if (subscribe_callback_event(ses_data, true, cb_data)) {
        AGM_LOGE("Unable to subscribe for callback event\n");
        g_error_free(error);
        g_free(ses_data);
        g_free(cb_data);
        rc = -EINVAL;
    }

    g_variant_unref(result);

exit:
    return rc;
}

int agm_session_register_for_events(uint32_t session_id,
                                    struct agm_event_reg_cfg *evt_reg_cfg) {
    GVariant *value_1, *value_2, *value_arr, *argument;
    GVariantBuilder builder_1;
    GVariant *result = NULL;
    GError *error = NULL;
    int rc = 0;

    g_assert(evt_reg_cfg != NULL);
    AGM_LOGD("%s\n", __func__);

    value_1 = g_variant_new_uint32(session_id);
    g_variant_builder_init(&builder_1, G_VARIANT_TYPE("(uuuyay)"));
    g_variant_builder_add(&builder_1,
                          "u",
                          (guint32)evt_reg_cfg->module_instance_id);
    g_variant_builder_add(&builder_1,
                          "u",
                          (guint32)evt_reg_cfg->event_id);
    g_variant_builder_add(&builder_1,
                          "u",
                          (guint32)evt_reg_cfg->event_config_payload_size);
    g_variant_builder_add(&builder_1, "y", (guint32)evt_reg_cfg->is_register);

    value_arr = g_variant_new_fixed_array(G_VARIANT_TYPE_BYTE,
                        (gconstpointer)&evt_reg_cfg->event_config_payload[0],
                        evt_reg_cfg->event_config_payload_size,
                        sizeof(guchar));
    g_variant_builder_add_value(&builder_1, value_arr);
    value_2 = g_variant_builder_end(&builder_1);

    argument = g_variant_new("(@u@(uuuyay))", value_1, value_2);

    if (mdata == NULL) {
        if ((rc = initialize_module_data()) != 0)
            return rc;
    }

    result = g_dbus_proxy_call_sync(mdata->proxy,
                                    "AgmSessionRegisterForEvents",
                                    argument,
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &error);

    if (result == NULL) {
        AGM_LOGE("%s: Error invoking AgmSessionRegisterForEvents: %s\n",
                  __func__, error->message);
        g_error_free(error);
        rc = -EINVAL;
        return rc;
    }

    g_variant_unref(result);
    return rc;
}

int agm_session_aif_set_cal(uint32_t session_id,
                            uint32_t aif_id,
                            struct agm_cal_config *cal_config) {
    GVariant *value_1, *value_2, *value_3, *value_4;
    GVariant *argument, *argument_2;
    GVariantBuilder builder_1;
    GVariant *result = NULL;
    GError *error = NULL;
    int rc = 0;
    gint i = 0;

    AGM_LOGD("%s :", __func__);
    g_assert(cal_config != NULL);

    value_1 = g_variant_new_uint32(session_id);
    value_2 = g_variant_new_uint32(aif_id);
    value_3 = g_variant_new_uint32(cal_config->num_ckvs);
    value_4 = g_variant_new_fixed_array(G_VARIANT_TYPE_BYTE,
                                        (gconstpointer)cal_config->kv,
                                        cal_config->num_ckvs*
                                                   sizeof(struct agm_key_value),
                                        sizeof(gchar));

    argument = g_variant_new("(@u@u@u@ay)", value_1, value_2, value_3, value_4);

    if (mdata == NULL) {
        if ((rc = initialize_module_data()) != 0)
            return rc;
    }

    result = g_dbus_proxy_call_sync(mdata->proxy,
                                    "AgmSessionAifSetCal",
                                    argument,
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &error);

    if (result == NULL) {
        AGM_LOGE("%s: Error invoking AgmSessionAifSetCal: %s\n", __func__,
                  error->message);
        g_error_free(error);
        rc = -EINVAL;
        return rc;
    }

    g_variant_unref(result);
    return rc;
}

int agm_session_aif_set_params(uint32_t session_id,
                               uint32_t aif_id,
                               void* payload,
                               size_t size) {
    GVariant *value_1, *value_2, *value_3, *value_4, *argument;
    GVariant *result = NULL;
    GError *error = NULL;
    int rc = 0;

    g_assert(payload != NULL);
    AGM_LOGD("%s\n", __func__);

    value_1 = g_variant_new_uint32(session_id);
    value_2 = g_variant_new_uint32(aif_id);
    value_3 = g_variant_new_uint32(size);
    value_4 = g_variant_new_fixed_array(G_VARIANT_TYPE_BYTE,
                                        (gconstpointer)payload,
                                        size,
                                        sizeof(gchar));

    argument = g_variant_new("(@u@u@u@ay)", value_1, value_2, value_3, value_4);

    if (mdata == NULL) {
        if ((rc = initialize_module_data()) != 0)
            return rc;
    }

    result = g_dbus_proxy_call_sync(mdata->proxy,
                                    "AgmSessionAifSetParams",
                                    argument,
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &error);

    if (result == NULL) {
        AGM_LOGE("%s: Error invoking AgmSessionAifSetParams: %s\n", __func__,
                  error->message);
        g_error_free(error);
        rc = -EINVAL;
        return rc;
    }

    g_variant_unref(result);
    return rc;
}

int agm_session_aif_get_tag_module_info_size(uint32_t session_id,
                                             uint32_t aif_id,
                                             size_t *size) {
    GVariant *value_1, *value_2, *value_3, *argument;
    GVariant *result = NULL;
    GError *error = NULL;
    int rc = 0;

    AGM_LOGD("%s\n", __func__);

    g_assert(size != NULL);

    value_1 = g_variant_new_uint32(session_id);
    value_2 = g_variant_new_uint32(aif_id);
    value_3 = g_variant_new_uint32(*size);

    argument = g_variant_new("(@u@u@u)", value_1, value_2, value_3);

    if (mdata == NULL) {
        if ((rc = initialize_module_data()) != 0)
            return rc;
    }

    result = g_dbus_proxy_call_sync(mdata->proxy,
                                    "AgmSessionAifGetTagModuleInfoSize",
                                    argument,
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &error);

    if (result == NULL) {
        AGM_LOGE("%s: Error invoking AgmSessionAifGetTagModuleInfo: %s\n",
                  __func__, error->message);
        g_error_free(error);
        rc = -EINVAL;
        return rc;
    }

    g_variant_get(result, "(u)", size);
    g_variant_unref(result);
    return rc;
}

int agm_session_aif_get_tag_module_info(uint32_t session_id,
                                        uint32_t aif_id,
                                        void *payload,
                                        size_t *size) {
    GVariant *value_1, *value_2, *value_3, *argument;
    GVariant *result = NULL;
    GVariant *val_arr = NULL;
    GError *error = NULL;
    gsize n_elements = 0;
    gsize element_size = sizeof(guchar);
    GVariant *array_v;
    GVariantIter arg_i;
    gconstpointer value;
    int rc = 0;

    AGM_LOGD("%s\n", __func__);

    if (payload == NULL)
        return agm_session_aif_get_tag_module_info_size(session_id,
                                                        aif_id,
                                                        size);

    g_assert(size != NULL);
    g_assert(payload != NULL);

    value_1 = g_variant_new_uint32(session_id);
    value_2 = g_variant_new_uint32(aif_id);
    value_3 = g_variant_new_uint32(*size);

    argument = g_variant_new("(@u@u@u)", value_1, value_2, value_3);

    if (mdata == NULL) {
        if ((rc = initialize_module_data()) != 0)
            return rc;
    }

    result = g_dbus_proxy_call_sync(mdata->proxy,
                                    "AgmSessionAifGetTagModuleInfo",
                                    argument,
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &error);

    if (result == NULL) {
        AGM_LOGE("%s: Error invoking AgmSessionAifGetTagModuleInfo: %s\n",
                  __func__, error->message);
        g_error_free(error);
        rc = -EINVAL;
        return rc;
    }

    g_variant_iter_init(&arg_i, result);
    g_variant_iter_next(&arg_i, "u", size);
    array_v = g_variant_iter_next_value(&arg_i);
    value = g_variant_get_fixed_array(array_v, &n_elements, element_size);
    memcpy(payload, value, n_elements);
    g_variant_unref(array_v);
    g_variant_unref(result);
    return rc;
}

int agm_session_aif_connect(uint32_t session_id,
                            uint32_t aif_id,
                            bool state) {
    GVariant *value_1, *value_2, *value_3, *argument;
    GVariant *result = NULL;
    GError *error = NULL;
    int rc = 0;

    AGM_LOGD("%s\n", __func__);

    value_1 = g_variant_new_uint32(session_id);
    value_2 = g_variant_new_uint32(aif_id);
    value_3 = g_variant_new_boolean(state);

    argument = g_variant_new("(@u@u@b)", value_1, value_2, value_3);

    if (mdata == NULL) {
        if ((rc = initialize_module_data()) != 0)
            return rc;
    }

    result = g_dbus_proxy_call_sync(mdata->proxy,
                                    "AgmSessionAifConnect",
                                    argument,
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &error);

    if (result == NULL) {
        AGM_LOGE("%s: Error invoking AgmSessionAifConnect: %s\n", __func__,
                  error->message);
        g_error_free(error);
        rc = -EINVAL;
        return rc;
    }

    g_variant_unref(result);
    return rc;
}

int agm_session_set_ec_ref(uint32_t capture_session_id,
                           uint32_t aif_id,
                           bool state) {
    GVariant *value_1, *value_2, *value_3, *argument;
    GVariant *result = NULL;
    GError *error = NULL;
    int rc = 0;

    AGM_LOGD("%s\n", __func__);

    value_1 = g_variant_new_uint32(capture_session_id);
    value_2 = g_variant_new_uint32(aif_id);
    value_3 = g_variant_new_boolean(state);

    argument = g_variant_new("(@u@u@b)", value_1, value_2, value_3);

    if (mdata == NULL) {
        if ((rc = initialize_module_data()) != 0)
            return rc;
    }

    result = g_dbus_proxy_call_sync(mdata->proxy,
                                    "AgmSessionSetEcRef",
                                    argument,
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &error);

    if (result == NULL) {
        AGM_LOGE("%s: Error invoking AgmSessionSetEcRef: %s\n", __func__,
                  error->message);
        g_error_free(error);
        rc = -EINVAL;
        return rc;
    }

    g_variant_unref(result);
    return rc;
}

int agm_session_get_params(uint32_t session_id, void* payload, size_t size) {
    GVariant *value_1, *value_2, *value_3, *argument;
    GVariant *val_arr = NULL, *result = NULL;
    GError *error = NULL;
    gconstpointer value;
    gsize n_elements;
    gsize element_size = sizeof(guchar);
    int rc = 0;

    g_assert(payload != NULL);
    AGM_LOGD("%s\n", __func__);

    value_1 = g_variant_new_uint32(session_id);
    value_2 = g_variant_new_uint32(size);
    value_3 = g_variant_new_fixed_array(G_VARIANT_TYPE_BYTE,
                                        (gconstpointer)payload,
                                        size,
                                        sizeof(gchar));

    argument = g_variant_new("(@u@u@ay)", value_1, value_2, value_3);

    if (mdata == NULL) {
        if ((rc = initialize_module_data()) != 0)
            return rc;
    }

    result = g_dbus_proxy_call_sync(mdata->proxy,
                                    "AgmSessionGetParams",
                                    argument,
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &error);

    if (result == NULL) {
        AGM_LOGE("%s: Error invoking AgmSessionGetParams: %s\n", __func__,
                  error->message);
        g_error_free(error);
        rc = -EINVAL;
        return rc;
    }

    val_arr = g_variant_get_child_value(result, 0);
    value = g_variant_get_fixed_array(val_arr, &n_elements, element_size);
    if (n_elements <= size) {
        memcpy(payload, value, n_elements);
    } else  {
        AGM_LOGE("Insufficient bytes size to copy bytes read\n");
        return -ENOMEM;
    }

    g_variant_unref(val_arr);
    g_variant_unref(result);
    return rc;
}

int agm_session_set_params(uint32_t session_id, void* payload, size_t size) {
    GVariant *value_1, *value_2, *value_3, *argument;
    GVariant *result = NULL;
    GError *error = NULL;
    int rc = 0;

    g_assert(payload != NULL);
    AGM_LOGD("%s\n", __func__);

    value_1 = g_variant_new_uint32(session_id);
    value_2 = g_variant_new_uint32(size);
    value_3 = g_variant_new_fixed_array(G_VARIANT_TYPE_BYTE,
                                        (gconstpointer)payload,
                                        size,
                                        sizeof(gchar));

    argument = g_variant_new("(@u@u@ay)", value_1, value_2, value_3);

    if (mdata == NULL) {
        if ((rc = initialize_module_data()) != 0)
            return rc;
    }

    result = g_dbus_proxy_call_sync(mdata->proxy,
                                    "AgmSessionSetParams",
                                    argument,
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &error);

    if (result == NULL) {
        AGM_LOGE("%s: Error invoking AgmSessionSetParams: %s\n", __func__,
                  error->message);
        g_error_free(error);
        rc = -EINVAL;
        return rc;
    }

    g_variant_unref(result);
    return rc;
}

static int agm_get_aif_info_list_size(size_t *num_aif_info) {
    GVariant *result = NULL;
    GError *error = NULL;
    int rc = 0;

    AGM_LOGD("%s\n", __func__);

    if (mdata == NULL) {
        if ((rc = initialize_module_data()) != 0)
            return rc;
    }

    result = g_dbus_proxy_call_sync(mdata->proxy,
                                    "AgmGetAifInfoListSize",
                                    NULL,
                                    G_DBUS_CALL_FLAGS_NONE,
                                    G_MAXINT,
                                    NULL,
                                    &error);

    if (result == NULL) {
        AGM_LOGE("%s: Error invoking AgmGetAifInfoListSize: %s\n", __func__,
                  error->message);
        g_error_free(error);
        rc = -EINVAL;
        return rc;
    }

    g_variant_get(result, "(u)", num_aif_info);
    g_variant_unref(result);
    return rc;
}

int agm_get_aif_info_list(struct aif_info *aif_list, size_t *num_aif_info) {
    GVariant *value_1 = NULL, *argument = NULL;
    GVariant *result = NULL, *array_v, *struct_v;
    GError *error = NULL;
    GVariantIter arg_i, struct_i, array_i;
    int rc = 0, i = 0;
    const char *name = NULL;

    g_assert(num_aif_info != NULL);
    AGM_LOGD("%s\n", __func__);

    if (*num_aif_info == 0) {
        return agm_get_aif_info_list_size(num_aif_info);
    } else {
        g_assert(aif_list != NULL);
        value_1 = g_variant_new_uint32(*num_aif_info);

        argument = g_variant_new("(@u)", value_1);

        if (mdata == NULL) {
            if ((rc = initialize_module_data()) != 0)
                return rc;
        }

        result = g_dbus_proxy_call_sync(mdata->proxy,
                                        "AgmGetAifInfoList",
                                        argument,
                                        G_DBUS_CALL_FLAGS_NONE,
                                        -1,
                                        NULL,
                                        &error);

        if (result == NULL) {
            AGM_LOGE("%s: Error invoking AgmGetAifInfoList: %s\n", __func__,
                      error->message);
            g_error_free(error);
            rc = -EINVAL;
            return rc;
        }
    }

    g_variant_iter_init(&arg_i, result);
    array_v = g_variant_iter_next_value(&arg_i);
    g_variant_iter_init(&array_i, array_v);
    for (i = 0; i < *num_aif_info; i++) {
        struct_v = g_variant_iter_next_value(&array_i);
        g_variant_iter_init(&struct_i, struct_v);
        g_variant_iter_next(&struct_i, "u", &aif_list[i].dir);
        g_variant_iter_next(&struct_i, "s", &name);
        strlcpy(aif_list[i].aif_name, name, (strlen(name) +1));
        g_variant_unref(struct_v);
    }
    g_variant_unref(array_v);
    g_variant_unref(result);

    return rc;
}
int agm_session_get_buf_info(uint32_t session_id, struct agm_buf_info *buf_info,
                             uint32_t flag)
{
    GVariant *value_1 = NULL, *value_2 = NULL, *struct_v = NULL, *argument = NULL;
    GVariant *val_arr = NULL, *result = NULL;
    GVariantIter arg_i, struct_i;
    GError *error = NULL;
    gconstpointer value;
    gsize n_elements;
    gsize element_size = sizeof(guchar);
    int rc = 0;

    AGM_LOGD("%s\n", __func__);

    if(!buf_info) {
        AGM_LOGE("%s: buf_info is NULL\n", __func__);
	return -EINVAL;
    }

    value_1 = g_variant_new_uint32(session_id);
    value_2 = g_variant_new_uint32(flag);

    argument = g_variant_new("(@u@u)", value_1, value_2);

    if (mdata == NULL) {
        if ((rc = initialize_module_data()) != 0)
            return rc;
    }

    result = g_dbus_proxy_call_sync(mdata->proxy,
                                    "AgmSessionGetBufInfo",
                                    argument,
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &error);

    if (result == NULL) {
        AGM_LOGE("%s: Error invoking AgmSessionGetBufInfo: %s\n", __func__,
                  error->message);
        g_error_free(error);
        rc = -EINVAL;
        return rc;
    }

    g_variant_iter_init(&arg_i, result);
    struct_v = g_variant_iter_next_value(&arg_i);
    g_variant_iter_init(&struct_i, struct_v);
    g_variant_iter_next(&struct_i, "i", &buf_info->data_buf_fd);
    g_variant_iter_next(&struct_i, "i", &buf_info->data_buf_size);
    g_variant_iter_next(&struct_i, "i", &buf_info->pos_buf_fd);
    g_variant_iter_next(&struct_i, "i", &buf_info->pos_buf_size);
    g_variant_unref(result);
    return rc;
}

int agm_aif_set_params(uint32_t aif_id, void* payload,
                       size_t size) {
    GVariant *value_1 = NULL, *value_2 = NULL, *value_3 = NULL, *argument = NULL;
    GVariant *result = NULL;
    GError *error = NULL;
    int rc = 0;

    g_assert(payload != NULL);
    AGM_LOGD("%s\n", __func__);

    value_1 = g_variant_new_uint32(aif_id);
    value_2 = g_variant_new_uint32(size);
    value_3 = g_variant_new_fixed_array(G_VARIANT_TYPE_BYTE,
                                        (gconstpointer)payload,
                                        size,
                                        sizeof(gchar));

    argument = g_variant_new("(@u@u@ay)", value_1, value_2, value_3);

    if (mdata == NULL) {
        if ((rc = initialize_module_data()) != 0)
            return rc;
    }

    result = g_dbus_proxy_call_sync(mdata->proxy,
                                    "AgmAifSetParams",
                                    argument,
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &error);

    if (result == NULL) {
        AGM_LOGE("%s: Error invoking AgmAifSetParams: %s\n", __func__,
                  error->message);
        g_error_free(error);
        rc = -EINVAL;
        return rc;
    }

    g_variant_unref(result);
    return rc;
}

int agm_set_params_with_tag(uint32_t session_id,
                            uint32_t aif_id,
                            struct agm_tag_config *tag_config) {
    GVariant *value_1, *value_2, *value_3, *value_4, *value_5;
    GVariant *argument, *argument_2;
    GVariantBuilder builder_1;
    GVariant *result = NULL;
    GError *error = NULL;
    int rc = 0;
    gint i = 0;

    g_assert(tag_config != NULL);
    AGM_LOGD("%s\n", __func__);

    value_1 = g_variant_new_uint32(session_id);
    value_2 = g_variant_new_uint32(aif_id);
    value_3 = g_variant_new_uint32(tag_config->tag);
    value_4 = g_variant_new_uint32(tag_config->num_tkvs);

    g_variant_builder_init(&builder_1, G_VARIANT_TYPE("a(uu)"));
    for (i = 0; i < tag_config->num_tkvs; i++) {
        g_variant_builder_open(&builder_1, G_VARIANT_TYPE("(uu)"));
        g_variant_builder_add(&builder_1, "u", (guint32)tag_config->kv[i].key);
        g_variant_builder_add(&builder_1, "u",
                                            (guint32)tag_config->kv[i].value);
        g_variant_builder_close(&builder_1);
    }

    value_5 = g_variant_builder_end(&builder_1);

    argument_2 = g_variant_new("(@u@u@a(uu))", value_3, value_4, value_5);

    argument = g_variant_new("(@u@u@(uua(uu)))", value_1, value_2, argument_2);

    if (mdata == NULL) {
        if ((rc = initialize_module_data()) != 0)
            return rc;
    }

    result = g_dbus_proxy_call_sync(mdata->proxy,
                                    "AgmSetParamsWithTag",
                                    argument,
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &error);

    if (result == NULL) {
        AGM_LOGE("%s: Error invoking AgmSetParamsWithTag: %s\n", __func__,
                  error->message);
        g_error_free(error);
        rc = -EINVAL;
        return rc;
    }

    g_variant_unref(result);
    return rc;
}

int agm_session_set_loopback(uint32_t capture_session_id,
                             uint32_t playback_session_id,
                             bool state) {
    GVariant *value_1, *value_2, *value_3, *argument;
    GVariant *result = NULL;
    GError *error = NULL;
    int rc = 0;

    AGM_LOGD("%s\n", __func__);

    value_1 = g_variant_new_uint32(capture_session_id);
    value_2 = g_variant_new_uint32(playback_session_id);
    value_3 = g_variant_new_boolean(state);

    argument = g_variant_new("(@u@u@b)", value_1, value_2, value_3);

    if (mdata == NULL) {
        if ((rc = initialize_module_data()) != 0)
            return rc;
    }

    result = g_dbus_proxy_call_sync(mdata->proxy,
                                    "AgmSessionSetLoopback",
                                    argument,
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &error);

    if (result == NULL) {
        AGM_LOGE("%s: Error invoking AgmSessionSetLoopback: %s\n", __func__,
                  error->message);
        g_error_free(error);
        rc = -EINVAL;
        return rc;
    }

    g_variant_unref(result);
    return rc;
}

int agm_session_set_metadata(uint32_t session_id,
                             uint32_t size, uint8_t *metadata) {
    GVariant *value_1, *value_2, *value_3, *argument;
    GVariant *result = NULL;
    GError *error = NULL;
    int rc = 0;

    g_assert(metadata != NULL);
    AGM_LOGD("%s\n", __func__);

    value_1 = g_variant_new_uint32(session_id);
    value_2 = g_variant_new_uint32(size);
    value_3 = g_variant_new_fixed_array(G_VARIANT_TYPE_BYTE,
                                        (gconstpointer)metadata,
                                        size,
                                        sizeof(gchar));

    argument = g_variant_new("(@u@u@ay)", value_1, value_2, value_3);

    if (mdata == NULL) {
        if ((rc = initialize_module_data()) != 0)
            return rc;
    }

    result = g_dbus_proxy_call_sync(mdata->proxy,
                                    "AgmSessionSetMetadata",
                                    argument,
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &error);

    if (result == NULL) {
        AGM_LOGE("%s: Error invoking AgmSessionSetMetadata: %s\n", __func__,
                  error->message);
        g_error_free(error);
        rc = -EINVAL;
        return rc;
    }

    g_variant_unref(result);
    return rc;
}

int agm_session_aif_set_metadata(uint32_t session_id,
                                 uint32_t aif_id,
                                 uint32_t size, uint8_t *metadata) {
    GVariant *value_1, *value_2, *value_3, *value_4, *argument;
    GVariant *result = NULL;
    GError *error = NULL;
    int rc = 0;

    g_assert(metadata != NULL);
    AGM_LOGD("%s\n", __func__);

    value_1 = g_variant_new_uint32(session_id);
    value_2 = g_variant_new_uint32(aif_id);
    value_3 = g_variant_new_uint32(size);
    value_4 = g_variant_new_fixed_array(G_VARIANT_TYPE_BYTE,
                                        (gconstpointer)metadata,
                                        size,
                                        sizeof(gchar));

    argument = g_variant_new("(@u@u@u@ay)", value_1, value_2, value_3, value_4);

    if (mdata == NULL) {
        if ((rc = initialize_module_data()) != 0)
            return rc;
    }

    result = g_dbus_proxy_call_sync(mdata->proxy,
                                    "AgmSessionAifSetMetadata",
                                    argument,
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &error);

    if (result == NULL) {
        AGM_LOGE("%s: Error invoking AgmSessionAifSetMetadata: %s\n", __func__,
                  error->message);
        g_error_free(error);
        rc = -EINVAL;
        return rc;
    }

    g_variant_unref(result);
    return rc;
}

int agm_aif_set_metadata(uint32_t aif_id,
                         uint32_t size, uint8_t *metadata) {
    GVariant *value_1, *value_2, *value_3, *argument;
    GVariant *result = NULL;
    GError *error = NULL;
    int rc = 0;

    g_assert(metadata != NULL);
    AGM_LOGD("%s\n", __func__);

    value_1 = g_variant_new_uint32(aif_id);
    value_2 = g_variant_new_uint32(size);
    value_3 = g_variant_new_fixed_array(G_VARIANT_TYPE_BYTE,
                                        (gconstpointer)metadata,
                                        size,
                                        sizeof(gchar));

    argument = g_variant_new("(@u@u@ay)", value_1, value_2, value_3);

    if (mdata == NULL) {
        if ((rc = initialize_module_data()) != 0)
            return rc;
    }

    result = g_dbus_proxy_call_sync(mdata->proxy,
                                    "AgmAifSetMetadata",
                                    argument,
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &error);

    if (result == NULL) {
        AGM_LOGE("%s: Error invoking AgmAifSetMetadata: %s\n", __func__,
                  error->message);
        g_error_free(error);
        rc = -EINVAL;
        return rc;
    }

    g_variant_unref(result);
    return rc;
}

int agm_aif_set_media_config(uint32_t aif_id,
                             struct agm_media_config *media_config) {
    GVariant *value_1, *value_2, *argument;
    GVariantBuilder builder_1;
    GVariant *result = NULL;
    GError *error = NULL;
    int rc = 0;

    g_assert(media_config != NULL);
    AGM_LOGD("%s\n", __func__);

    value_1 = g_variant_new_uint32(aif_id);

    g_variant_builder_init(&builder_1, G_VARIANT_TYPE("(uuiu)"));
    g_variant_builder_add(&builder_1, "u", (guint32)media_config->rate);
    g_variant_builder_add(&builder_1, "u", (guint32)media_config->channels);
    g_variant_builder_add(&builder_1, "i", (gint32)media_config->format);
    g_variant_builder_add(&builder_1, "u", (guint32)media_config->data_format);
    value_2 = g_variant_builder_end(&builder_1);

    argument = g_variant_new("(@u@(uuiu))", value_1, value_2);

    if (mdata == NULL) {
        if ((rc = initialize_module_data()) != 0)
            return rc;
    }

    result = g_dbus_proxy_call_sync(mdata->proxy,
                                    "AgmAifSetMediaConfig",
                                    argument,
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &error);

    if (result == NULL) {
        AGM_LOGE("%s: Error invoking AgmAifSetMediaConfig: %s\n", __func__,
                  error->message);
        g_error_free(error);
        rc = -EINVAL;
        return rc;
    }

    g_variant_unref(result);
    return rc;
}

int agm_get_buffer_timestamp(uint32_t session_id, uint64_t *timestamp) {
    GVariant *argument = NULL;
    GVariant *result = NULL;
    GError *error = NULL;
    int rc = 0;

    if (mdata == NULL) {
        if ((rc = initialize_module_data()) != 0)
            return rc;
    }

    AGM_LOGD("%s\n", __func__);

    argument = g_variant_new("(@u)", g_variant_new_uint32(session_id));

    result = g_dbus_proxy_call_sync(mdata->proxy,
                                    "AgmGetBufferTimestamp",
                                    argument,
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &error);

    if (result == NULL) {
        AGM_LOGE("%s: Error invoking AgmGetBufferTimestamp: %s\n", __func__,
                  error->message);
        g_error_free(error);
        rc = -EINVAL;
        return rc;
    }

    g_variant_get(result, "(t)", timestamp);
    g_variant_unref(result);
    return rc;
}

size_t agm_get_hw_processed_buff_cnt(uint64_t handle, enum direction dir) {
    GVariant *argument = NULL;
    GVariant *result = NULL;
    GError *error = NULL;
    agm_client_session_data *ses_data = (agm_client_session_data *) handle;
    size_t bufCount;
    int rc = 0;

    g_assert(ses_data != NULL);
    g_assert(ses_data->proxy != NULL);

    AGM_LOGD("%s\n", __func__);

    argument = g_variant_new("(@u)", g_variant_new_uint32(dir));

    result = g_dbus_proxy_call_sync(ses_data->proxy,
                                    "AgmGetHwProcessedBufCount",
                                    argument,
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &error);

    if (result == NULL) {
        AGM_LOGE("%s: Error invoking AgmGetHwProcessedBufCount: %s\n", __func__,
                  error->message);
        g_error_free(error);
        return -EINVAL;
    }

    g_variant_get(result, "(i)", &bufCount);
    g_variant_unref(result);
    return bufCount;
}

int agm_get_session_time(uint64_t handle, uint64_t *timestamp) {
    agm_client_session_data *ses_data = (agm_client_session_data *) handle;
    GVariant *result;
    GError *error = NULL;

    g_assert(ses_data != NULL);
    g_assert(ses_data->proxy != NULL);
    g_assert(timestamp != NULL);

    AGM_LOGD("%s\n", __func__);

    result = g_dbus_proxy_call_sync(ses_data->proxy,
                                    "AgmSessionGetTime",
                                    NULL,
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &error);

    if (result == NULL) {
        AGM_LOGE("%s: Error invoking AgmSessionGetTime: %s\n", __func__,
                  error->message);
        g_error_free(error);
        return -EINVAL;
    }

    g_variant_get(result, "(t)", timestamp);
    g_variant_unref(result);
    return 0;
}

int agm_session_eos(uint64_t handle) {
    agm_client_session_data *ses_data = (agm_client_session_data *) handle;
    GVariant *result;
    GError *error = NULL;

    g_assert(ses_data != NULL);
    g_assert(ses_data->proxy != NULL);

    AGM_LOGD("%s\n", __func__);

    result = g_dbus_proxy_call_sync(ses_data->proxy,
                                    "AgmSessionEos",
                                    NULL,
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &error);

    if (result == NULL) {
        AGM_LOGE("%s: Error invoking AgmSessionEos: %s\n", __func__,
                  error->message);
        g_error_free(error);
        return -EINVAL;
    }

    g_variant_unref(result);
    return 0;
}

int agm_session_set_config(uint64_t handle,
                           struct agm_session_config *session_config,
                           struct agm_media_config *media_config,
                           struct agm_buffer_config *buffer_config) {
    agm_client_session_data *ses_data = (agm_client_session_data *) handle;
    GVariant *result = NULL, *arr = NULL, *argument = NULL, *value_1, *value_2;
    GError *error = NULL;
    GVariantBuilder builder_1;

    g_assert(ses_data != NULL);
    g_assert(session_config != NULL);
    g_assert(media_config != NULL);
    g_assert(buffer_config != NULL);
    g_assert(ses_data->proxy != NULL);

    AGM_LOGD("%s\n", __func__);

    g_variant_builder_init(&builder_1, G_VARIANT_TYPE("(uuu)"));
    g_variant_builder_add(&builder_1, "u", (guint32)media_config->rate);
    g_variant_builder_add(&builder_1, "u", (guint32)media_config->channels);
    g_variant_builder_add(&builder_1, "u", (guint32)media_config->format);
    value_1 = g_variant_builder_end(&builder_1);

    g_variant_builder_init(&builder_1, G_VARIANT_TYPE("(uu)"));
    g_variant_builder_add(&builder_1, "u", (guint32)buffer_config->count);
    g_variant_builder_add(&builder_1, "u", (guint32)buffer_config->size);
    value_2 = g_variant_builder_end(&builder_1);

    arr = g_variant_new_fixed_array(G_VARIANT_TYPE_BYTE,
                                    (gconstpointer)session_config,
                                    sizeof(struct agm_session_config),
                                    sizeof(guchar));

    argument = g_variant_new("(@(uuu)@(uu)@ay)",
                   value_1,
                   value_2,
                   arr);


    result = g_dbus_proxy_call_sync(ses_data->proxy,
                                    "AgmSessionSetConfig",
                                    argument,
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &error);

    if (result == NULL) {
        AGM_LOGE("%s: Error invoking AgmSessionSetConfig: %s\n", __func__,
                  error->message);
        g_error_free(error);
        return -EINVAL;
    }

    g_variant_unref(result);
    return 0;
}

int agm_session_write(uint64_t handle, void *buf, size_t *byte_count) {
    agm_client_session_data *ses_data = (agm_client_session_data *) handle;
    GVariant *result = NULL, *arr = NULL, *argument = NULL;
    GError *error = NULL;
    gint64 start_time, end_time;
    int ret = 0;

    g_assert(ses_data != NULL);
    g_assert(ses_data->proxy != NULL);
    AGM_LOGD("%s\n", __func__);

    arr = g_variant_new_fixed_array(G_VARIANT_TYPE_BYTE,
                                    (gconstpointer)buf,
                                    *byte_count,
                                    sizeof(guchar));
    argument = g_variant_new("(@u@ay)", g_variant_new_uint32(*byte_count), arr);

    g_mutex_lock(&ses_data->mutex);
    result = g_dbus_proxy_call_sync(ses_data->proxy,
                                    "AgmSessionWrite",
                                    argument,
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &error);

    if (result == NULL) {
        AGM_LOGE("%s: Error invoking AgmSessionWrite: %s\n", __func__,
                  error->message);
        g_error_free(error);
        g_mutex_unlock(&ses_data->mutex);
        return -EINVAL;
    }

    start_time = g_get_monotonic_time();
    end_time = start_time + (AGM_DBUS_ASYNC_CALL_TIMEOUT_MS * G_TIME_SPAN_MILLISECOND);
    g_cond_wait_until(&ses_data->cond, &ses_data->mutex, end_time);
    if (g_get_monotonic_time() >= end_time){
        AGM_LOGE("%s: -ETIMEDOUT %d\n", __func__, g_get_monotonic_time());
    }

    g_mutex_unlock(&ses_data->mutex);
    g_variant_get(result, "(u)", byte_count);
    g_variant_unref(result);
    return ret;
}

int agm_session_read(uint64_t handle, void *buf, size_t *byte_count) {
    agm_client_session_data *ses_data = (agm_client_session_data *) handle;
    GVariant *result = NULL, *val_arr = NULL, *argument = NULL;
    GError *error = NULL;
    GVariantIter arg_i;
    gconstpointer value;
    gsize n_elements;
    gsize element_size = sizeof(guchar);
    gint64 start_time, end_time;

    g_assert(ses_data != NULL);
    g_assert(ses_data->proxy != NULL);
    AGM_LOGD("%s\n", __func__);

    argument = g_variant_new("(@u)", g_variant_new_uint32(*byte_count));
    ses_data->buf_size = *byte_count;
    ses_data->buf = buf;

    g_mutex_lock(&ses_data->mutex);
    result = g_dbus_proxy_call_sync(ses_data->proxy,
                                    "AgmSessionRead",
                                    argument,
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &error);

    if (result == NULL) {
        AGM_LOGE("%s: Error invoking AgmSessionRead: %s\n", __func__,
                  error->message);
        g_error_free(error);
        return -EINVAL;
    }

    start_time = g_get_monotonic_time();
    end_time = start_time + (AGM_DBUS_ASYNC_CALL_TIMEOUT_MS * G_TIME_SPAN_MILLISECOND);
    g_cond_wait_until(&ses_data->cond, &ses_data->mutex, end_time);
    if (g_get_monotonic_time() >= end_time){
        AGM_LOGE("%s: -ETIMEDOUT %d\n", __func__, g_get_monotonic_time());
    }
    *byte_count = ses_data->buf_size;

    g_mutex_unlock(&ses_data->mutex);
    g_variant_unref(result);
    return 0;
}

int agm_session_resume(uint64_t handle) {
    agm_client_session_data *ses_data = (agm_client_session_data *) handle;
    GVariant *result = NULL;
    GError *error = NULL;

    g_assert(ses_data != NULL);
    g_assert(ses_data->proxy != NULL);
    AGM_LOGD("%s\n", __func__);

    result = g_dbus_proxy_call_sync(ses_data->proxy,
                                    "AgmSessionResume",
                                    NULL,
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &error);

    if (result == NULL) {
        AGM_LOGE("%s: Error invoking AgmSessionResume: %s\n", __func__,
                  error->message);
        g_error_free(error);
        return -EINVAL;
    }

    g_variant_unref(result);
    return 0;
}

int agm_session_pause(uint64_t handle) {
    agm_client_session_data *ses_data = (agm_client_session_data *) handle;
    GVariant *result = NULL;
    GError *error = NULL;

    g_assert(ses_data != NULL);
    g_assert(ses_data->proxy != NULL);
    AGM_LOGD("%s\n", __func__);

    result = g_dbus_proxy_call_sync(ses_data->proxy,
                                    "AgmSessionPause",
                                    NULL,
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &error);

    if (result == NULL) {
        AGM_LOGE("%s: Error invoking AgmSessionPause: %s\n", __func__,
                  error->message);
        g_error_free(error);
        return -EINVAL;
    }

    g_variant_unref(result);
    return 0;
}

int agm_session_stop(uint64_t handle) {
    agm_client_session_data *ses_data = (agm_client_session_data *) handle;
    GVariant *result = NULL;
    GError *error = NULL;

    g_assert(ses_data != NULL);
    g_assert(ses_data->proxy != NULL);
    AGM_LOGD("%s\n", __func__);

    result = g_dbus_proxy_call_sync(ses_data->proxy,
                                    "AgmSessionStop",
                                    NULL,
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &error);

    if (result == NULL) {
        AGM_LOGE("%s: Error invoking AgmSessionStop: %s\n", __func__,
                  error->message);
        g_error_free(error);
        return -EINVAL;
    }

    g_variant_unref(result);
    return 0;
}

int agm_session_start(uint64_t handle) {
    agm_client_session_data *ses_data = (agm_client_session_data *) handle;
    GVariant *result = NULL;
    GError *error = NULL;

    g_assert(ses_data != NULL);
    g_assert(ses_data->proxy != NULL);
    AGM_LOGD("%s\n", __func__);

    result = g_dbus_proxy_call_sync(ses_data->proxy,
                                    "AgmSessionStart",
                                    NULL,
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &error);

    if (result == NULL) {
        AGM_LOGE("%s: Error invoking AgmSessionStart: %s\n", __func__,
                  error->message);
        g_error_free(error);
        return -EINVAL;
    }

    g_variant_unref(result);
    return 0;
}

int agm_session_prepare(uint64_t handle) {
    agm_client_session_data *ses_data = (agm_client_session_data *) handle;
    GVariant *result = NULL;
    GError *error = NULL;

    g_assert(ses_data != NULL);
    g_assert(ses_data->proxy != NULL);
    AGM_LOGD("%s\n", __func__);

    result = g_dbus_proxy_call_sync(ses_data->proxy,
                                    "AgmSessionPrepare",
                                    NULL,
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &error);

    if (result == NULL) {
        AGM_LOGE("%s: Error invoking AgmSessionPrepare: %s\n", __func__,
                  error->message);
        g_error_free(error);
        return -EINVAL;
    }

    g_variant_unref(result);
    return 0;
}

int agm_session_close(uint64_t handle) {
    agm_client_session_data *ses_data = (agm_client_session_data *) handle;
    GVariant *result = NULL;
    GError *error = NULL;
    int rc = 0;

    g_assert(ses_data != NULL);
    g_assert(ses_data->proxy != NULL);
    AGM_LOGD("%s\n", __func__);

    result = g_dbus_proxy_call_sync(ses_data->proxy,
                                    "AgmSessionClose",
                                    NULL,
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &error);

    if (result == NULL) {
        AGM_LOGE("%s: Error invoking AgmSessionClose: %s\n", __func__,
                  error->message);
        g_error_free(error);
        return -EINVAL;
    }

    free_callbacks(ses_data);

    if (ses_data->thread_loop) {
        AGM_LOGE("%s:Quitting loop", __func__);
        g_main_loop_quit(ses_data->loop);
        g_thread_join(ses_data->thread_loop);
        ses_data->thread_loop = NULL;
    }

    subscribe_ses_callback_event(ses_data, false);
    g_cond_clear(&ses_data->cond);
    g_mutex_clear(&ses_data->mutex);

    if (ses_data->ses_thread_loop) {
        AGM_LOGE("Quitting ses thread loop");
        g_main_loop_quit(ses_data->ses_loop);
        g_thread_join(ses_data->ses_thread_loop);
        ses_data->ses_thread_loop = NULL;
    }

    g_hash_table_remove(mdata->ses_hash_table,
                        GINT_TO_POINTER(ses_data->session_id));
    g_free(ses_data->obj_path);

    if (ses_data->proxy != NULL) {
        g_object_unref(ses_data->proxy);
        ses_data->proxy = NULL;
    }

    g_free(ses_data);
    g_variant_unref(result);
    return 0;
}

int agm_session_open(uint32_t session_id, enum agm_session_mode sess_mode, uint64_t *handle) {

    GVariant *value_1 = NULL, *value_2 = NULL, *argument = NULL;

    GVariant *result = NULL;
    GError *error = NULL;
    int rc = 0;
    agm_client_session_data *ses_data = NULL;
    agm_callback_data *cb_data;
    gchar thread_name[16] = "";

    g_assert(handle != NULL);
    AGM_LOGD("%s\n", __func__);

    if (mdata == NULL) {
        if ((rc = initialize_module_data()) != 0)
            return rc;
    }

    value_1 = g_variant_new_uint32(session_id);
    value_2 = g_variant_new_uint32(sess_mode);

    argument = g_variant_new("(@u@u)", value_1, value_2);


    result = g_dbus_proxy_call_sync(mdata->proxy,
                                    "AgmSessionOpen",
                                    argument,
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &error);

    if (result == NULL) {
        AGM_LOGE("%s: Error invoking AgmSessionOpen: %s\n", __func__,
                  error->message);
        g_error_free(error);
        rc = -EINVAL;
        goto exit;
    }


    if ((ses_data = (agm_client_session_data *)g_hash_table_lookup(
                                        mdata->ses_hash_table,
                                        GINT_TO_POINTER(session_id))) == NULL) {
        ses_data = (agm_client_session_data *)
                                    g_malloc0(sizeof(agm_client_session_data));

        g_variant_get(result, "(o)", &ses_data->obj_path);
        ses_data->conn = mdata->conn;
        *handle = (uint64_t)ses_data;

        ses_data->proxy = g_dbus_proxy_new_sync(ses_data->conn,
                                G_DBUS_PROXY_FLAGS_NONE,
                                NULL,
                                AGM_DBUS_CONNECTION,
                                ses_data->obj_path,
                                AGM_SESSION_IFACE,
                                NULL,
                                &error);

        if (ses_data->proxy == NULL) {
            AGM_LOGE("%s: Error in getting dbus proxy: %s", __func__,
                      error->message);
            rc = -EINVAL;
            g_error_free(error);
            goto exit;
        }

        ses_data->session_id = session_id;
        /* add session to sessions hash table */
        g_hash_table_insert(mdata->ses_hash_table,
                            GINT_TO_POINTER(ses_data->session_id),
                            ses_data);
        g_variant_unref(result);
    } else {
        *handle = (uint64_t)ses_data;
    }

    g_mutex_init(&ses_data->mutex);
    g_cond_init(&ses_data->cond);
    snprintf(thread_name, sizeof(thread_name), "ses_loop_%d", session_id);
    AGM_LOGD("create thread %s\n", thread_name);
    ses_data->ses_thread_loop = g_thread_try_new(thread_name, ses_signal_threadloop, ses_data, &error);
    if (!ses_data->ses_thread_loop) {
        rc = -EINVAL;
        AGM_LOGE("Could not create thread %s, error %s\n", thread_name,
                  error->message);
        g_error_free(error);
        g_free(ses_data);
        goto exit;
    }

    if (subscribe_ses_callback_event(ses_data, true)) {
        AGM_LOGE("Unable to subscribe for ses callback event\n");
        g_error_free(error);
        g_free(ses_data);
        rc = -EINVAL;
    }

    return rc;

exit:
    if (ses_data != NULL)
        g_free(ses_data);
    return rc;
}

int agm_init() {
    GError *error = NULL;
    int rc = 0;

    AGM_LOGD("%s\n", __func__);

    if (mdata == NULL) {
        if ((rc = initialize_module_data()) != 0)
            goto exit;
    }

    return rc;

exit:
    if (mdata)
        g_free(mdata);
    return rc;
}

int agm_deinit() {
}
