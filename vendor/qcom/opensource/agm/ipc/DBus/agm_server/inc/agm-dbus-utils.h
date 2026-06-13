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

#include <dbus/dbus.h>
#include <glib.h>
#include <gio/gio.h>

typedef void (*agm_dbus_receive_cb_t)(DBusConnection *conn,
                                      DBusMessage *msg,
                                      void *userdata);

typedef struct {
    const char *method_name;
    const char *method_signature;
    agm_dbus_receive_cb_t cb_func;
} agm_dbus_method;

typedef struct {
    const char *method_name;
    const char *method_signature;
} agm_dbus_signal;

typedef struct {
    const char *name;
    agm_dbus_method *methods;
    int method_count;
    agm_dbus_signal *signals;
    int signal_count;
} agm_dbus_interface_info;

typedef struct {
    DBusConnection *conn;
    guint watch_id;
    int watch_fd;
    DBusWatch *watch;
} agm_dbus_watch_data;

typedef struct {
    DBusConnection *conn;
    guint timeout_id;
    DBusTimeout *timeout;
} agm_dbus_timeout_data;

typedef struct {
    DBusConnection *conn;
    GHashTable *objects;
} agm_dbus_connection;

/* Creates a new agm_dbus_connection object and returns it */
agm_dbus_connection *agm_dbus_new_connection();

/* Frees agm_dbus_connection object */
void agm_dbus_connection_free(agm_dbus_connection *conn);

/* Registers dbus interface with the object */
int agm_dbus_add_interface(agm_dbus_connection *conn,
                           const char *dbus_obj_path,
                           agm_dbus_interface_info *interface_info,
                           void *userdata);

/* Removes dbus interface from the object */
int agm_dbus_remove_interface(agm_dbus_connection *conn,
                              const char *dbus_obj_path,
                              const char *interface);

/* Sends error to dbus client */
void agm_dbus_send_error(agm_dbus_connection *conn,
                         DBusMessage *msg,
                         char *dbus_error,
                         char *error);

/* Sends a signal to dbus client */
void agm_dbus_send_signal(agm_dbus_connection *conn, DBusMessage *msg);

/* Sets the watch and timeout functions of a DBusConnection to integrate the
   connection. Returns 0 on success */
int agm_setup_dbus_with_main_loop(agm_dbus_connection *conn);
