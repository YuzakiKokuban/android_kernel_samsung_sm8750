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

#define LOG_TAG "agm_dbus_utils"

#include <stdio.h>
#include <errno.h>
#include <malloc.h>

#include "agm-dbus-utils.h"
#include "utils.h"

#define DISPATCH_TIMEOUT  0

typedef struct {
    const char *name;
    GHashTable *methods; /* Key -> method_name Value -> agm_dbus_method */
    GHashTable *signals; /* Key -> method_name Value -> agm_dbus_signal */
    void *userdata;
} agm_dbus_interface;

typedef struct {
    const char *obj_path;
    GHashTable *interfaces; /* Key -> interface name Value -> agm_dbus_interface */
} agm_dbus_object;

static DBusHandlerResult server_message_handler(DBusConnection *connection,
                                                DBusMessage *message,
                                                void *userdata) {
    DBusHandlerResult result;
    DBusMessage *reply = NULL;
    DBusError err;
    agm_dbus_connection *conn = (agm_dbus_connection *)userdata;
    const char *dbus_obj_path = NULL;
    const char *dbus_interface = NULL;
    const char *dbus_method = NULL;
    agm_dbus_object *object = NULL;
    agm_dbus_interface *interface = NULL;
    agm_dbus_method *method = NULL;

    if (dbus_message_get_type(message) != DBUS_MESSAGE_TYPE_METHOD_CALL)
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    dbus_interface = dbus_message_get_interface(message);
    dbus_method = dbus_message_get_member(message);
    dbus_obj_path = dbus_message_get_path(message);

    if ((object = (agm_dbus_object *)g_hash_table_lookup
                                     (conn->objects, dbus_obj_path)) == NULL) {
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    } else {
        if ((interface = (agm_dbus_interface *)
             g_hash_table_lookup(object->interfaces, dbus_interface)) == NULL) {
            return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        } else {
            if ((method = (agm_dbus_method *)
                g_hash_table_lookup(interface->methods, dbus_method)) == NULL) {
                return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
            } else {
                method->cb_func(connection, message, interface->userdata);
                return DBUS_HANDLER_RESULT_HANDLED;
            }
        }
    }
}

static DBusObjectPathVTable vtable = {
    .unregister_function = NULL,
    .message_function = server_message_handler,
    .dbus_internal_pad1 = NULL,
    .dbus_internal_pad2 = NULL,
    .dbus_internal_pad3 = NULL,
    .dbus_internal_pad4 = NULL
};

static void agm_handle_dispatch_status(DBusConnection *conn,
                                       DBusDispatchStatus status,
                                       void *userdata) {
    if (status == DBUS_DISPATCH_DATA_REMAINS) {
        while (dbus_connection_get_dispatch_status(conn) ==
                                                    DBUS_DISPATCH_DATA_REMAINS)
            dbus_connection_dispatch(conn);
    }
}

static void dispatch_status(DBusConnection *conn,
                            DBusDispatchStatus status,
                            void *userdata)
{
    if (!dbus_connection_get_is_connected(conn))
        return;

    agm_handle_dispatch_status(conn, status, userdata);
}

static void agm_free_dbus_watch_data(void *userdata) {
    agm_dbus_watch_data *watch_data = (agm_dbus_watch_data *)userdata;

    if (watch_data == NULL) {
        AGM_LOGD("%s: Watch data is NULL\n", __func__);
        return;
    }

    if (watch_data->watch_id > 0) {
        g_source_remove(watch_data->watch_id);
        watch_data->watch_id = 0;
    }

    if (watch_data->conn)
        dbus_connection_unref(watch_data->conn);

    if (watch_data)
        free(watch_data);
}

static void agm_remove_dbus_watch_cb(DBusWatch *watch, void *userdata) {
    if (dbus_watch_get_enabled(watch))
        return;

    /* Triggers agm_free_dbus_watch_data which is set in agm_add_dbus_watch_cb*/
    dbus_watch_set_data(watch, NULL, NULL);
}

static gboolean agm_cond_watch_cb(GIOChannel *channel,
                                  GIOCondition io_condition,
                                  gpointer userdata) {
    agm_dbus_watch_data *watch_data = (agm_dbus_watch_data *)userdata;
    unsigned int flags = 0;

    flags |= (io_condition & G_IO_IN  ? DBUS_WATCH_READABLE : 0) |
             (io_condition & G_IO_OUT ? DBUS_WATCH_WRITABLE : 0) |
             (io_condition & G_IO_HUP ? DBUS_WATCH_HANGUP : 0) |
             (io_condition & G_IO_ERR ? DBUS_WATCH_ERROR : 0);

    /* Notifies the D-Bus library when a previously-added watch is ready for
       reading or writing or has an exception */
    if (dbus_watch_handle(watch_data->watch, flags) == false)
        AGM_LOGE("dbus_watch_handle() failed\n");

    agm_handle_dispatch_status(watch_data->conn,
                        dbus_connection_get_dispatch_status(watch_data->conn),
                        (void *)userdata);
    return true;
}

static dbus_bool_t agm_add_dbus_watch_cb(DBusWatch *watch, void *userdata) {
    agm_dbus_connection *conn = (agm_dbus_connection *) userdata;
    agm_dbus_watch_data *watch_data = NULL;
    dbus_bool_t status;
    GIOChannel *channel;
    int watch_fd;
    unsigned int watch_flags;
    GIOCondition io_condition;

    if (conn->conn == NULL) {
        AGM_LOGE("%s: Connection is NULL\n", __func__);
        return -1;
    }

    status = dbus_watch_get_enabled(watch);

    if (!status)
        return true;

    watch_data = (agm_dbus_watch_data *)calloc(1, sizeof(agm_dbus_watch_data));

    if (watch_data == NULL) {
        AGM_LOGE("%s: Couldn't allocate memory for watch_data\n", __func__);
        return false;
    }

    /* Gets unix file descriptor for DBusWatch object we want to watch */
    watch_data->watch_fd = dbus_watch_get_unix_fd(watch);
    /* Creates a new GIOChannel given a file descriptor */
    channel = g_io_channel_unix_new(watch_data->watch_fd);
    watch_data->watch = watch;
    watch_data->conn = dbus_connection_ref(conn->conn);

    /* Sets data which can be retrieved with dbus_watch_get_data() */
    dbus_watch_set_data(watch_data->watch, watch_data, agm_free_dbus_watch_data);
    /* Dbus flags indicating the conditions to be monitored on the dbus watch */
    watch_flags = dbus_watch_get_flags(watch_data->watch);

    if (watch_flags & DBUS_WATCH_READABLE) {
        io_condition = G_IO_IN;
        io_condition = (GIOCondition)(io_condition | (G_IO_HUP | G_IO_ERR));
    } else if (watch_flags & DBUS_WATCH_WRITABLE) {
        io_condition = G_IO_OUT;
        io_condition = (GIOCondition) (io_condition | (G_IO_HUP | G_IO_ERR));
    } else {
        io_condition = (GIOCondition) (G_IO_HUP | G_IO_ERR);
    }

    /* Adds IOChannel to default main loop and calls the provided function
       whenever condition is satisfied */
    watch_data->watch_id = g_io_add_watch_full(channel,
                                               G_PRIORITY_DEFAULT,
                                               io_condition,
                                               agm_cond_watch_cb,
                                               watch_data,
                                               agm_free_dbus_watch_data);
    g_io_channel_unref(channel);
    return true;
}

static void agm_toggle_dbus_watch_cb(DBusWatch *watch, void *userdata) {
    if (dbus_watch_get_enabled(watch))
        agm_add_dbus_watch_cb(watch, userdata);
    else
        agm_remove_dbus_watch_cb(watch, userdata);
}

static void agm_free_dbus_timeout_data(void *userdata) {
    agm_dbus_timeout_data *timeout_data = (agm_dbus_timeout_data *)userdata;

    if (timeout_data == NULL) {
        AGM_LOGE("%s: Timeout data is NULL\n", __func__);
        return;
    }

    if (timeout_data->timeout_id > 0) {
        /* Removes the source with the given ID from the default main context.*/
        g_source_remove(timeout_data->timeout_id);
        timeout_data->timeout_id = 0;
    }

    dbus_connection_unref(timeout_data->conn);
    free(timeout_data);
}

static gboolean agm_timeout_dispatch_cb(gpointer userdata) {
    agm_dbus_timeout_data *timeout_data = (agm_dbus_timeout_data *)userdata;

    timeout_data->timeout_id = 0;

    /* if not enabled should not be polled by the main loop */
    if (!dbus_timeout_get_enabled(timeout_data->timeout))
        return FALSE;

    dbus_timeout_handle(timeout_data->timeout);

    return FALSE;
}

static void agm_remove_dbus_timeout_cb(DBusTimeout *timeout, void *userdata) {
    /* This will trigger agm_free_dbus_timeout_data() */
    dbus_timeout_set_data(timeout, NULL, NULL);
}

static dbus_bool_t agm_add_dbus_timeout_cb(DBusTimeout *timeout,
                                           void *userdata) {
    int interval;
    agm_dbus_connection *conn = (agm_dbus_connection *) userdata;
    agm_dbus_timeout_data *timeout_data = NULL;
    dbus_bool_t status;

    if (conn->conn == NULL) {
        AGM_LOGE("%s: Connection is NULL\n", __func__);
        return -1;
    }

    status = dbus_timeout_get_enabled(timeout);

    if (!status)
        return true;

    timeout_data = (agm_dbus_timeout_data *)
                                    calloc(1, sizeof(agm_dbus_timeout_data));

    if (timeout_data == NULL) {
        AGM_LOGE("%s: Couldn't allocate memory for timeout_data\n", __func__);
        return false;
    }

    interval = dbus_timeout_get_interval(timeout);
    timeout_data->conn = dbus_connection_ref(conn->conn);
    timeout_data->timeout = timeout;

    /* Sets data which can be retrieved with dbus_timeout_get_data() */
    dbus_timeout_set_data(timeout_data->timeout,
                          timeout_data,
                          agm_free_dbus_timeout_data);
    /* Sets a function to be called at regular intervals
       with the given priority */
    timeout_data->timeout_id = g_timeout_add_full(G_PRIORITY_DEFAULT,
                                                  interval,
                                                  agm_timeout_dispatch_cb,
                                                  timeout_data,
                                                  agm_free_dbus_timeout_data);

    return true;
}

static void agm_toggle_dbus_timeout_cb(DBusTimeout *timeout, void *userdata) {
    if (dbus_timeout_get_enabled(timeout))
        agm_add_dbus_timeout_cb(timeout, userdata);
    else
        agm_remove_dbus_timeout_cb(timeout, userdata);
}

int agm_setup_dbus_with_main_loop(agm_dbus_connection *conn) {
    if (conn == NULL) {
        AGM_LOGE("%s: Connection object is NULL\n", __func__);
        return -1;
    }

    if (!dbus_connection_set_watch_functions(conn->conn,
                                             agm_add_dbus_watch_cb,
                                             agm_remove_dbus_watch_cb,
                                             agm_toggle_dbus_watch_cb,
                                             conn,
                                             NULL)) {
        AGM_LOGE("%s: dbus_connection_set_watch_functions failed\n", __func__);
        return -1;
    }

    if (!dbus_connection_set_timeout_functions(conn->conn,
                                               agm_add_dbus_timeout_cb,
                                               agm_remove_dbus_timeout_cb,
                                               agm_toggle_dbus_timeout_cb,
                                               conn,
                                               NULL)) {
        AGM_LOGE("%s: dbus_connection_set_timeout_functions failed\n", __func__);
        return -1;
    }

    dbus_connection_set_dispatch_status_function(conn->conn,
                                                 agm_handle_dispatch_status,
                                                 conn,
                                                 NULL);

    return 0;
}

static void unregister_object(agm_dbus_connection *conn,
                              agm_dbus_object *object) {
    dbus_connection_unregister_object_path(conn->conn, object->obj_path);
}

void agm_dbus_send_signal(agm_dbus_connection *conn, DBusMessage *signal_msg) {
    DBusMessage *signal_copy;

    signal_copy = dbus_message_copy(signal_msg);
    dbus_connection_send(conn->conn, signal_copy, NULL);
    dbus_message_unref(signal_copy);
}

void agm_dbus_send_error(agm_dbus_connection *conn,
                         DBusMessage *msg,
                         char *dbus_error,
                         char *error) {
    DBusMessage *reply = NULL;

    reply = dbus_message_new_error(msg, dbus_error, error);
    dbus_connection_send(conn->conn, reply, NULL);
    dbus_message_unref(reply);
    return;
}

void agm_free_signal(gpointer data) {
    agm_dbus_signal *signal = (agm_dbus_signal *)data;

    if(signal == NULL)
        return;

    free(signal);
    signal = NULL;
}

void agm_free_method(gpointer data) {
    agm_dbus_method *method = (agm_dbus_method *)data;

    if(method == NULL)
        return;

    free(method);
    method = NULL;
}


void agm_free_interface(gpointer data) {
    agm_dbus_interface *interface = (agm_dbus_interface *)data;

    if(interface == NULL)
        return;

    g_hash_table_remove_all(interface->methods);
    g_hash_table_unref(interface->methods);

    g_hash_table_remove_all(interface->signals);
    g_hash_table_unref(interface->signals);

    free(interface);
    interface = NULL;
}

void agm_free_object(gpointer data) {
    agm_dbus_object *object = (agm_dbus_object *)data;

    if(object == NULL)
        return;

    g_hash_table_remove_all(object->interfaces);
    g_hash_table_unref(object->interfaces);

    free(object);
    object = NULL;
}

int agm_dbus_remove_interface(agm_dbus_connection *conn,
                              const char *dbus_obj_path,
                              const char *interface_path) {
    agm_dbus_object *object = NULL;
    agm_dbus_interface *interface = NULL;

    if (conn == NULL || dbus_obj_path == NULL || interface_path == NULL)
        return -1;

    if ((object = (agm_dbus_object *)
                    g_hash_table_lookup(conn->objects, dbus_obj_path)) == NULL)
        return -1;

    if ((interface = (agm_dbus_interface *)
               g_hash_table_lookup(object->interfaces, interface_path)) == NULL)
        return -1;

    g_hash_table_remove(object->interfaces,interface_path);
    interface = NULL;

    if (g_hash_table_size(object->interfaces) == 0) {
        unregister_object(conn, object);
        g_hash_table_remove(conn->objects, (void *)object->obj_path);
        object = NULL;
    }

    return 0;
}

int agm_dbus_add_interface(agm_dbus_connection *conn,
                           const char *dbus_obj_path,
                           agm_dbus_interface_info *interface_info,
                           void *userdata) {
    DBusError err;
    int i = 0;
    agm_dbus_object *object = NULL;
    agm_dbus_interface *interface = NULL;
    agm_dbus_method *method = NULL;
    agm_dbus_signal *signal = NULL;

    if ((conn == NULL) || conn->conn == NULL || conn->objects == NULL) {
        AGM_LOGE("Connection not initialized\n");
        return -EINVAL;
    }

    if (dbus_obj_path == NULL) {
        AGM_LOGE("Invalid object path\n");
        return -EINVAL;
    }

    if (interface_info == NULL) {
        AGM_LOGE("Invalid interface\n");
        return -EINVAL;
    }

    dbus_error_init(&err);

    if ((object = (agm_dbus_object *)
                   g_hash_table_lookup(conn->objects, dbus_obj_path)) == NULL) {
        object = (agm_dbus_object *)malloc(sizeof(agm_dbus_object));
        object->obj_path = dbus_obj_path;
        object->interfaces = g_hash_table_new_full(g_str_hash,
                                                   g_str_equal,
                                                   NULL,
                                                   agm_free_interface);

        interface = (agm_dbus_interface *)malloc(sizeof(agm_dbus_interface));
        interface->name = interface_info->name;
        interface->methods = g_hash_table_new_full(g_str_hash,
                                                   g_str_equal,
                                                   NULL,
                                                   agm_free_method);

        for (i = 0; i < interface_info->method_count; i++) {
             method = (agm_dbus_method *)malloc(sizeof(agm_dbus_method));
             method->method_name = interface_info->methods[i].method_name;
             method->method_signature =
                                    interface_info->methods[i].method_signature;
             method->cb_func = interface_info->methods[i].cb_func;
             g_hash_table_insert(interface->methods,
                                 g_strdup(method->method_name),
                                 method);
        }

        interface->signals = g_hash_table_new_full(g_str_hash,
                                                   g_str_equal,
                                                   NULL,
                                                   agm_free_signal);
        for (i = 0; i < interface_info->signal_count; i++) {
             signal = (agm_dbus_signal *)malloc(sizeof(agm_dbus_signal));
             signal->method_name = interface_info->signals[i].method_name;
             signal->method_signature =
                                    interface_info->signals[i].method_signature;
             g_hash_table_insert(interface->signals,
                                 g_strdup(signal->method_name),
                                 signal);
        }

        interface->userdata = userdata;
        g_hash_table_insert(object->interfaces,
                            g_strdup(interface->name),
                            interface);

        g_hash_table_insert(conn->objects, g_strdup(object->obj_path), object);
        if (!dbus_connection_register_object_path(conn->conn,
                                                  dbus_obj_path,
                                                  &vtable,
                                                  conn)) {
            AGM_LOGE("Failed to register a object path for agm\n");
            dbus_error_free(&err);
            return -EINVAL;
        }
        AGM_LOGE("Registered object %s", dbus_obj_path);
    } else {
        if ((interface = (agm_dbus_interface *)
                                g_hash_table_lookup(object->interfaces,
                                               interface_info->name)) == NULL) {
            interface = (agm_dbus_interface *)
                                    malloc(sizeof(agm_dbus_interface));
            interface->name = interface_info->name;
            interface->signals = NULL;
            interface->methods = NULL;
            if (interface_info->methods != NULL) {
                interface->methods = g_hash_table_new_full(g_str_hash,
                                                           g_str_equal,
                                                           NULL,
                                                           agm_free_method);

                for (i = 0; i < interface_info->method_count; i++) {
                    method = (agm_dbus_method *)malloc(sizeof(agm_dbus_method));
                    method->method_name =
                                    interface_info->methods[i].method_name;
                    method->method_signature =
                                    interface_info->methods[i].method_signature;
                    method->cb_func = interface_info->methods[i].cb_func;
                    g_hash_table_insert(interface->methods,
                                        g_strdup(method->method_name),
                                        method);
                }
            }

            if (interface_info->signals != NULL) {
                interface->signals = g_hash_table_new_full(g_str_hash,
                                                           g_str_equal,
                                                           NULL,
                                                           agm_free_signal);
                for (i = 0; i < interface_info->signal_count; i++) {
                    signal = (agm_dbus_signal *)malloc(sizeof(agm_dbus_signal));
                    signal->method_name =
                                    interface_info->signals[i].method_name;
                    signal->method_signature =
                                    interface_info->signals[i].method_signature;
                    g_hash_table_insert(interface->signals,
                                        g_strdup(signal->method_name),
                                        signal);
                }
            }

            interface->userdata = userdata;
            g_hash_table_insert(object->interfaces,
                                g_strdup(interface->name),
                                interface);
        }
    }

    dbus_error_free(&err);
    return 0;
}

void agm_dbus_connection_free(agm_dbus_connection *conn) {
    if (conn == NULL) {
        AGM_LOGE("Connection is NULL\n");
        return;
    }

    g_hash_table_remove_all(conn->objects);
    g_hash_table_unref(conn->objects);

    free(conn);
    conn = NULL;
}

agm_dbus_connection *agm_dbus_new_connection() {
    int rc = 0;
    DBusError err;
    agm_dbus_connection *conn = NULL;

    conn = (agm_dbus_connection *)malloc(sizeof(agm_dbus_connection));
    conn->objects = NULL;

    dbus_error_init(&err);
    conn->conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);

    if (conn->conn == NULL) {
        AGM_LOGE("Failed to request name on bus: %s\n", err.message);
        free(conn);
        dbus_error_free(&err);
        return NULL;
    }

    conn->objects = g_hash_table_new_full(g_str_hash,
                                          g_str_equal,
                                          NULL,
                                          agm_free_object);
    dbus_error_free(&err);
    return conn;
}
