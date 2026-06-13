/*
 * FST Manager: hostapd/wpa_supplicant control interface wrapper: Dbus based
 * implementation
 *
 * Copyright (c) 2015, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
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
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <assert.h>
#include <gio/gio.h>
#include <glib.h>

#include "utils/common.h"
#include "common/defs.h"

#define FST_MGR_COMPONENT "DBUS"
#include "fst_manager.h"

#include "fst/fst_ctrl_defs.h"
#include "fst_ctrl.h"

#define WPAS_DBUS_OBJECT_PATH_MAX 150

static const uint8_t zero_mac[ETH_ALEN] = {0};

static fst_notification_cb_func global_ntfy_cb = NULL;
static void *global_ntfy_cb_ctx = NULL;

struct wpa_ctrl {
	GDBusConnection *connection;
	guint            watch_id;
	guint            signal_id;
};

static struct wpa_ctrl *dbus_ctrl = NULL;

static void format_group_opath(const char *gname, char *buffer, size_t size)
{
	os_snprintf(buffer, size,
			WPAS_DBUS_NEW_PATH_FST "/" WPAS_DBUS_NEW_FST_GROUPS_PART "/%s",
			gname);
}

static void format_iface_opath(const char *gname, const char *ifname,
		char *buffer, size_t size)
{
	os_snprintf(buffer, size,
			WPAS_DBUS_NEW_PATH_FST "/" WPAS_DBUS_NEW_FST_GROUPS_PART "/%s"
			"/" WPAS_DBUS_NEW_FST_INTERFACES_PART "/%s",
			gname, ifname);
}

static void printerr_variant(GVariant *variant, const char *prefix)
{
	gchar *value_str;
	value_str = g_variant_print(variant, TRUE);
	fst_mgr_printf(MSG_ERROR, "%s: %s", prefix, value_str);
	g_free(value_str);
}

#ifdef DEBUG_PROPERTIES
static void print_properties(GDBusProxy *proxy)
{
  gchar **property_names;
  guint n;

  g_print ("    properties:\n");

  property_names = g_dbus_proxy_get_cached_property_names (proxy);
  for (n = 0; property_names != NULL && property_names[n] != NULL; n++)
    {
      const gchar *key = property_names[n];
      GVariant *value;
      gchar *value_str;
      value = g_dbus_proxy_get_cached_property (proxy, key);
      value_str = g_variant_print (value, TRUE);
      g_print ("      %s -> %s\n", key, value_str);
      g_variant_unref (value);
      g_free (value_str);
    }
  g_strfreev (property_names);
}
#else
#define print_properties(p)
#endif

static GVariant *variant_assure_type(GVariant *value, const GVariantType *type)
{
	if (!g_variant_is_of_type(value, type)) {
		printerr_variant(value, "Incorrect variant");
		g_variant_unref(value);
		return NULL;
	}

	return value;
}

static GVariant *fst_ctrl_parse_global_signal_array_entry(GVariant *variant,
		gsize idx, const char **name)
{
	GVariant *value = NULL, *aentry, *v = NULL;
	gsize len;

	aentry = g_variant_get_child_value(variant, idx);
	if (!aentry) {
		fst_mgr_printf(MSG_ERROR, "No array entry #%zd", idx);
		goto out;
	}

	v = g_variant_get_child_value(aentry, 0);
	if (!g_variant_is_of_type(v, G_VARIANT_TYPE_STRING)) {
		fst_mgr_printf(MSG_ERROR, "Incorrect variant type (STRING expected)");
		goto out;
	}

	*name = g_variant_get_string(v, &len);
	fst_mgr_printf(MSG_DEBUG, " name='%s'", *name);
	g_variant_unref(v);

	v = g_variant_get_child_value(aentry, 1);
	if (!g_variant_is_of_type(v, G_VARIANT_TYPE_VARIANT)) {
		fst_mgr_printf(MSG_ERROR, "Incorrect variant type (VARIANT expected)");
		goto out;
	}

	value = g_variant_get_variant(v);

out:
	if (v)
		g_variant_unref(v);
	if (aentry)
		g_variant_unref(aentry);
	return value;
}

static void fst_ctrl_on_global_signal_session_simple(GVariant *array,
		gsize n_aitems, const gchar *opath, enum fst_event_type event_type)
{
	GVariant    *aitem = NULL;
	guint32      session_id;
	const gchar *str;

	g_assert_nonnull(array);
	g_assert_nonnull(opath);
	g_assert_nonnull(global_ntfy_cb);
	g_assert_cmpint(n_aitems, ==, 1);

	aitem = fst_ctrl_parse_global_signal_array_entry(array, 0, &str);
	g_assert_nonnull(aitem);
	g_assert_cmpstr(str, ==, FST_DBUS_GLOBAL_SIG_SALL_PNAME_SESSION_ID);

	session_id = g_variant_get_uint32(aitem);

	global_ntfy_cb(global_ntfy_cb_ctx, session_id, event_type, NULL);

	g_variant_unref(aitem);
}


static void fst_ctrl_on_global_signal_session_established(GVariant *array,
		gsize n_aitems, const gchar *opath)
{
	fst_ctrl_on_global_signal_session_simple(array, n_aitems, opath,
			EVENT_FST_ESTABLISHED);
}

static void fst_ctrl_on_global_signal_session_setup(GVariant *array,
		gsize n_aitems, const gchar *opath)
{
	fst_ctrl_on_global_signal_session_simple(array, n_aitems, opath,
			EVENT_FST_SETUP);
}

static void fst_ctrl_on_global_signal_session_state(GVariant *array,
		gsize n_aitems, const gchar *opath)
{
	GVariant    *aitem = NULL;
	gsize        i;
	guint32      session_id = FST_INVALID_SESSION_ID;
	union        fst_event_extra e;

	g_assert_nonnull(array);
	g_assert_nonnull(opath);
	g_assert_nonnull(global_ntfy_cb);
	g_assert_cmpint(n_aitems, >=, 2);

	for (i = 0; i < n_aitems; i++) {
		const gchar *str;

		aitem = fst_ctrl_parse_global_signal_array_entry(array, i, &str);
		g_assert_nonnull(aitem);
		if (g_strcmp0(str, FST_DBUS_GLOBAL_SIG_SALL_PNAME_SESSION_ID) == 0)
			session_id = g_variant_get_uint32(aitem);
		else if (g_strcmp0(str, FST_DBUS_GLOBAL_SIG_SSTATE_PNAME_OLD_STATE) == 0)
			e.session_state.old_state = g_variant_get_byte(aitem);
		else if (g_strcmp0(str, FST_DBUS_GLOBAL_SIG_SSTATE_PNAME_NEW_STATE) == 0)
			e.session_state.new_state = g_variant_get_byte(aitem);
		else if (g_strcmp0(str, FST_DBUS_GLOBAL_SIG_SSTATE_PNAME_INIT_REASON) == 0)
			e.session_state.extra.to_initial.reason =
				g_variant_get_byte(aitem);
		else if (g_strcmp0(str, FST_DBUS_GLOBAL_SIG_SSTATE_PNAME_INIT_REJECT_CODE) == 0)
			e.session_state.extra.to_initial.reject_code =
				g_variant_get_byte(aitem);
		else if (g_strcmp0(str, FST_DBUS_GLOBAL_SIG_SSTATE_PNAME_INIT_INITIATOR) == 0)
			e.session_state.extra.to_initial.initiator =
				g_variant_get_byte(aitem);
		g_variant_unref(aitem);
		aitem = NULL;
	}

	if (session_id == FST_INVALID_SESSION_ID) {
		fst_mgr_printf(MSG_ERROR, "Incorrect parameters");
		goto out;
	}

	global_ntfy_cb(global_ntfy_cb_ctx, session_id,
		EVENT_FST_SESSION_STATE_CHANGED, &e);

out:
	if (aitem)
		g_variant_unref(aitem);
}

static void fst_ctrl_on_global_signal_iface_peer_state(GVariant *array,
		gsize n_aitems, const gchar *opath)
{
	GVariant     *aitem = NULL;
	gboolean      connected = -1;
	const guint8 *peer_address = NULL;
	const gchar  *ifname;
	gsize         len, i;
	union fst_event_extra evt_data = { };

	g_assert_nonnull(array);
	g_assert_nonnull(opath);
	g_assert_nonnull(global_ntfy_cb);
	g_assert_cmpint(n_aitems, ==, 3);

	for (i = 0; i < n_aitems; i++) {
		const gchar *str;

		aitem = fst_ctrl_parse_global_signal_array_entry(array, i, &str);
		g_assert_nonnull(aitem);
		if (!g_strcmp0(str, FST_DBUS_GLOBAL_SIG_PNAME_IFNAME)) {
			ifname = g_variant_get_string(aitem, &len);
			if (len >= FST_MAX_INTERFACE_SIZE) {
				fst_mgr_printf(MSG_ERROR, "Invalid ifname length (%s): %zd", ifname, len);
				goto out;
			}
		} else if (!g_strcmp0(str, FST_DBUS_GLOBAL_SIG_PNAME_CONNECTED))
			connected = g_variant_get_boolean(aitem);
		else if (!g_strcmp0(str, FST_DBUS_GLOBAL_SIG_PNAME_PEER_ADDR)) {
			peer_address =
					g_variant_get_fixed_array(aitem, &len, sizeof(guint8));
			if (len != ETH_ALEN) {
				fst_mgr_printf(MSG_ERROR, "Invalid peer_addr length: %zd", len);
				goto out;
			}
		}
		g_variant_unref(aitem);
		aitem = NULL;
	}

	if (!ifname || connected == -1 || !peer_address) {
		fst_mgr_printf(MSG_ERROR, "Invalid parameters");
		goto out;
	}

	os_memcpy(&evt_data.peer_state.addr, peer_address, ETH_ALEN);
	evt_data.peer_state.connected = connected;
	os_strlcpy(evt_data.peer_state.ifname, ifname,
		FST_MAX_INTERFACE_SIZE);

	global_ntfy_cb(global_ntfy_cb_ctx, FST_INVALID_SESSION_ID,
			   EVENT_PEER_STATE_CHANGED, &evt_data);

out:
	if (aitem)
		g_variant_unref(aitem);
}

static void fst_ctrl_on_global_signal(GDBusConnection *connection,
        const gchar *sender_name,
		const gchar *object_path,
		const gchar *interface_name,
		const gchar *signal_name,
		GVariant *parameters,
		gpointer user_data)
{
	const gchar  *opath, *signal;
	gsize         opath_length, event_length, n_aitems;
	GVariantIter  param_iter;
	GVariant     *v_opath, *v_signal, *v_array;

	g_assert_cmpstr(object_path, ==, WPAS_DBUS_NEW_PATH_FST);
	g_assert_cmpstr(interface_name, ==, WPAS_DBUS_NEW_IFACE_FST);
	g_assert_cmpstr(signal_name, ==, FST_DBUS_GLOBAL_SIG);

	if (!global_ntfy_cb)
		return;

	if (!g_variant_is_of_type(parameters, G_VARIANT_TYPE("(osa{sv})"))) {
		fst_mgr_printf(MSG_ERROR, "Invalid variant type='%s", g_variant_get_type_string(parameters));
		return;
	}

	g_variant_iter_init(&param_iter, parameters);

	v_opath = g_variant_iter_next_value(&param_iter);
	opath = g_variant_get_string(v_opath, &opath_length);

	v_signal = g_variant_iter_next_value(&param_iter);
	signal = g_variant_get_string(v_signal, &event_length);

	v_array    = g_variant_iter_next_value(&param_iter);
	n_aitems = g_variant_n_children(v_array);

	if (!g_strcmp0(signal, FST_DBUS_GLOBAL_SIG_EVT_SESSION_SETUP)) {
		fst_ctrl_on_global_signal_session_setup(v_array, n_aitems, opath);
	} else if (!g_strcmp0(signal, FST_DBUS_GLOBAL_SIG_EVT_SESSION_STATE)) {
		fst_ctrl_on_global_signal_session_state(v_array, n_aitems, opath);
	} else if (!g_strcmp0(signal, FST_DBUS_GLOBAL_SIG_EVT_SESSION_ESTAB)) {
		fst_ctrl_on_global_signal_session_established(v_array, n_aitems, opath);
	} else if (!g_strcmp0(signal, FST_DBUS_GLOBAL_SIG_EVT_IFACE_PEER_STATE)) {
		fst_ctrl_on_global_signal_iface_peer_state(v_array, n_aitems, opath);
	} else {
		fst_mgr_printf(MSG_DEBUG, " ****************** Signal Received ****************");
		fst_mgr_printf(MSG_DEBUG, " sender_name:    %s", sender_name);
		fst_mgr_printf(MSG_DEBUG, " object_path:    %s", object_path);
		fst_mgr_printf(MSG_DEBUG, " interface_name: %s", interface_name);
		fst_mgr_printf(MSG_DEBUG, " signal_name:    %s", signal);
		printerr_variant(parameters, FST_DBUS_GLOBAL_SIG);
		fst_mgr_printf(MSG_DEBUG, " ***************************************************");
		g_assert(0);
	}

	g_variant_unref(v_opath);
	g_variant_unref(v_signal);
	g_variant_unref(v_array);
}

static GDBusProxy *get_proxy(const char *path, const char *interface)
{
	GError *error;
	GDBusProxy *proxy;

	assert(path != NULL);
	assert(interface != NULL);

	error = NULL;
	proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
                                         G_DBUS_PROXY_FLAGS_NONE,
                                         NULL, /* GDBusInterfaceInfo */
                                         WPAS_DBUS_NEW_SERVICE,
                                         path,
                                         interface,
                                         NULL, /* GCancellable */
                                         &error);
	if (proxy == NULL) {
      fst_mgr_printf(MSG_ERROR, "Error creating proxy: %s", error->message);
      g_error_free(error);
    }

	return proxy;
}

static GVariant *proxy_get_property(GDBusProxy *proxy, const char *property)
{
	GVariant *value;

	assert(proxy != NULL);
	assert(property != NULL);

    value = g_dbus_proxy_get_cached_property(proxy, property);
	if (!value)
		fst_mgr_printf(MSG_ERROR, "[%s:%s] No such property: %s",
			g_dbus_proxy_get_object_path(proxy),
			g_dbus_proxy_get_interface_name(proxy),
			property);

	return value;
}

static GVariant *proxy_get_typed_property(GDBusProxy *proxy,
		const char *property, const GVariantType *type)
{
	GVariant *value = proxy_get_property(proxy, property);
	if (!value)
		return NULL;

	return variant_assure_type(value, type);
}

static GVariant *get_property(const char *path, const char *interface,
		const char *property)
{
	GDBusProxy  *proxy;
    GVariant    *value;

	assert(path != NULL);
	assert(interface != NULL);
	assert(property != NULL);

	proxy = get_proxy(path, interface);
	if (!proxy)
		return NULL;

	print_properties(proxy);

    value = proxy_get_property(proxy, property);

	if (proxy)
		g_object_unref(proxy);

	return value;
}

static GVariant *get_typed_property(const char *path, const char *interface,
		const char *property, const GVariantType *type)
{
	GVariant *value = get_property(path, interface, property);
	if (!value)
		return NULL;

	return variant_assure_type(value, type);
}

static gchar *get_session_path(u32 session_id)
{
	gchar        *res = NULL;
	const gchar **strv = NULL;
	gsize         i, len  = 0;
	GVariant     *value, *pvalue;

	value = get_typed_property(
			WPAS_DBUS_NEW_PATH_FST,
			WPAS_DBUS_NEW_IFACE_FST,
			FST_DBUS_GLOBAL_PROP_SESSIONS,
			G_VARIANT_TYPE_OBJECT_PATH_ARRAY);
	if (!value)
		goto out;

	strv = g_variant_get_objv(value, &len);
	if (!strv)
		goto out;

	for (i = 0; !res && i < len; i++) {
		pvalue = get_typed_property(strv[i],
				WPAS_DBUS_NEW_IFACE_FST_SESSION,
				FST_DBUS_SESSION_PROP_ID,
				G_VARIANT_TYPE_UINT32);
		if (pvalue) {
			if (g_variant_get_uint32(pvalue) == session_id)
				res = g_strdup(strv[i]);
			g_variant_unref(pvalue);
		}
	}

out:
	if (value)
		g_variant_unref(value);

	return res;
}

static int session_call_method(u32 session_id, const char *method,
		GVariant *parameters)
{
	int         res = -EINVAL;
	gchar      *session_path = NULL;
	GDBusProxy *proxy = NULL;
	GError     *error = NULL;
	GVariant   *value = NULL;

	session_path = get_session_path(session_id);
	if (!session_path) {
		fst_mgr_printf(MSG_ERROR, "Cannot find session %u", session_id);
		goto out;
	}

	proxy = get_proxy(session_path, WPAS_DBUS_NEW_IFACE_FST_SESSION);
	if (!proxy)
		goto out;

	value = g_dbus_proxy_call_sync(proxy, method,  parameters,
				G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
	if (error) {
      fst_mgr_printf(MSG_ERROR, "Error calling method: %s", error->message);
      g_error_free(error);
	  goto out;
    }

	res = 0;

out:
	if (session_path)
		g_free(session_path);
	if (value)
		g_variant_unref(value);
	if (proxy)
		g_object_unref(proxy);

	return res;
}

int fst_set_notify_cb(fst_notification_cb_func ntfy_cb, void *ntfy_cb_ctx)
{
	global_ntfy_cb = ntfy_cb;
	global_ntfy_cb_ctx = ntfy_cb_ctx;
	return 0;
}

void fst_free(void *p)
{
	free(p);
}

int fst_get_groups(struct fst_group_info **groups)
{
	int           res = -EINVAL;
	const gchar **strv = NULL;
	gsize         i, len = 0;
	GVariant     *value;

	*groups = NULL;

	value = get_typed_property(WPAS_DBUS_NEW_PATH_FST,
			WPAS_DBUS_NEW_IFACE_FST,
			FST_DBUS_GLOBAL_PROP_GROUPS,
			G_VARIANT_TYPE_OBJECT_PATH_ARRAY);
	if (!value)
		goto out;

	strv = g_variant_get_objv(value, &len);
	if (!strv)
		goto out;

	*groups = malloc(sizeof(struct fst_group_info) * len);
	if (!*groups) {
		fst_mgr_printf(MSG_ERROR, "Cannot allocate array to store groups");
		res = -ENOMEM;
		goto out;
	}

	for (i = 0; i < len; i++) {
		struct fst_group_info *info = *groups + i;
		const char  *group_id = strv[i] +
				sizeof(WPAS_DBUS_NEW_PATH_FST) +
				sizeof(WPAS_DBUS_NEW_FST_GROUPS_PART);
		fst_mgr_printf(MSG_DEBUG, " path#%zd: %s (group=%s)", i, strv[i], group_id);
		os_strlcpy(info->id, group_id, sizeof(info->id));
	}

	res = len;

out:
	if (value)
		g_variant_unref(value);

	if (res < 0 && *groups) {
		fst_free(*groups);
		*groups = NULL;
	}

	return res;
}

int fst_get_group_ifaces(const struct fst_group_info *group,
		struct fst_iface_info **ifaces)
{
	int           res = -EINVAL;
	int           gid_len;
	GDBusProxy   *proxy = NULL;
	const gchar **strv = NULL;
	gsize         i, len  = 0;
	GVariant     *value, *pvalue = NULL;
	char          group_path[WPAS_DBUS_OBJECT_PATH_MAX];

	format_group_opath(group->id, group_path, sizeof(group_path));

	value = get_typed_property(group_path,
			WPAS_DBUS_NEW_IFACE_FST_GROUP,
			FST_DBUS_GROUP_PROP_IFACES,
			G_VARIANT_TYPE_OBJECT_PATH_ARRAY);
	if (!value)
		goto out;

	strv = g_variant_get_objv(value, &len);
	if (!strv)
		goto out;

	*ifaces = malloc(sizeof(struct fst_iface_info) * len);
	if (!*ifaces) {
		fst_mgr_printf(MSG_ERROR, "Cannot allocate array to store ifaces");
		res = -ENOMEM;
		goto out;
	}

	gid_len = os_strlen(group->id) + 1;
	for (i = 0; i < len; i++) {
		struct fst_iface_info *info = *ifaces + i;
		const char *ifname = strv[i] +
				sizeof(WPAS_DBUS_NEW_PATH_FST) +
				sizeof(WPAS_DBUS_NEW_FST_GROUPS_PART) +
				gid_len +
				sizeof(WPAS_DBUS_NEW_FST_INTERFACES_PART);

		fst_mgr_printf(MSG_DEBUG, " path#%zd: %s (ifname=%s)", i, strv[i], ifname);

		proxy = get_proxy(strv[i], WPAS_DBUS_NEW_IFACE_FST_INTERFACE);
		if (!proxy)
			goto out;

		os_strlcpy(info->name, ifname, sizeof(info->name));

		pvalue = proxy_get_typed_property(proxy,
				FST_DBUS_IFACE_PROP_LLT,
				G_VARIANT_TYPE_UINT32);
		if (!pvalue)
			goto out;

		info->llt = g_variant_get_uint32(pvalue);
		g_variant_unref(pvalue);

		pvalue = proxy_get_typed_property(proxy,
				FST_DBUS_IFACE_PROP_PRIORITY,
				G_VARIANT_TYPE_BYTE);
		if (!pvalue)
			goto out;

		info->priority = g_variant_get_byte(pvalue);
		g_variant_unref(pvalue);
		pvalue = NULL;
		g_object_unref(proxy);
		proxy = NULL;
	}

	res = len;

out:
	if (pvalue)
		g_variant_unref(pvalue);
	if (value)
		g_variant_unref(value);
	if (proxy)
		g_object_unref(proxy);
	if (res < 0 && *ifaces) {
		fst_free(*ifaces);
		*ifaces = NULL;
	}

	return res;
}

int fst_get_sessions(const struct fst_group_info *group, u32 **sessions)
{
	int           res = -EINVAL;
	const gchar **strv = NULL;
	gsize         i, len  = 0;
	GVariant     *value, *pvalue = NULL;
	char          group_path[WPAS_DBUS_OBJECT_PATH_MAX];

	format_group_opath(group->id, group_path, sizeof(group_path));

	value = get_typed_property(group_path,
			WPAS_DBUS_NEW_IFACE_FST_GROUP,
			FST_DBUS_GROUP_PROP_SESSIONS,
			G_VARIANT_TYPE_OBJECT_PATH_ARRAY);
	if (!value)
		goto out;

	strv = g_variant_get_objv(value, &len);
	if (!strv)
		goto out;

	*sessions = malloc(sizeof(u32) * len);
	if (!*sessions) {
		fst_mgr_printf(MSG_ERROR, "Cannot allocate array to store sessions");
		res = -ENOMEM;
		goto out;
	}

	for (i = 0; i < len; i++) {
		u32 *info = *sessions + i;
		pvalue = get_typed_property(strv[i],
				WPAS_DBUS_NEW_IFACE_FST_SESSION,
				FST_DBUS_SESSION_PROP_ID,
				G_VARIANT_TYPE_UINT32);
		if (!pvalue)
			goto out;

		*info = g_variant_get_uint32(pvalue);
		g_variant_unref(pvalue);
		pvalue = NULL;
	}

	res = len;

out:
	if (pvalue)
		g_variant_unref(pvalue);
	if (value)
		g_variant_unref(value);
	if (res < 0 && *sessions) {
		fst_free(*sessions);
		*sessions = NULL;
	}

	return res;
}

int fst_get_iface_peers(const struct fst_group_info *group,
			struct fst_iface_info *iface, uint8_t **peers)
{
	int           res = -EINVAL;
	GVariant     *value;
	char          iface_path[WPAS_DBUS_OBJECT_PATH_MAX];
	const gchar  *v;
	gsize         len;

	*peers = NULL;

	format_iface_opath(group->id, iface->name, iface_path, sizeof(iface_path));

	value = get_typed_property(iface_path,
			WPAS_DBUS_NEW_IFACE_FST_INTERFACE,
			FST_DBUS_IFACE_PROP_PEER_ADDR,
			G_VARIANT_TYPE_BYTESTRING);
	if (!value)
		goto out;

	v = (const gchar*)g_variant_get_fixed_array(value, &len, sizeof(u8));

	if (len != ETH_ALEN) {
		fst_mgr_printf(MSG_ERROR, "Incorrect peer address array len: %zd", len);
		goto out;
	}

	if (!os_memcmp(v, zero_mac, len)) {
		res = 0;
		goto out;
	}

	*peers = os_zalloc(ETH_ALEN);
	if (!*peers)  {
		fst_mgr_printf(MSG_ERROR, "Cannot allocate array to store peer address");
		res = -ENOMEM;
		goto out;
	}

	os_memcpy(*peers, v, ETH_ALEN);

	res = 1;

out:
	if (value)
		g_variant_unref(value);
	if (res < 0 && *peers) {
		fst_free(*peers);
		*peers = NULL;
	}

	return res;
}

int fst_get_peer_mbies(const struct fst_group_info *group,
	struct fst_iface_info *iface, const uint8_t *peer, char **mbies)
{
	int           res = -EINVAL;
	GVariant     *value;
	char          iface_path[WPAS_DBUS_OBJECT_PATH_MAX];
	const gchar  *v;
	gsize         len;

	*mbies = NULL;

	format_iface_opath(group->id, iface->name, iface_path, sizeof(iface_path));

	value = get_typed_property(iface_path,
			WPAS_DBUS_NEW_IFACE_FST_INTERFACE,
			FST_DBUS_IFACE_PROP_PEER_MBIEs,
			G_VARIANT_TYPE_BYTESTRING);
	if (!value)
		goto out;

	v = (const gchar*)g_variant_get_fixed_array(value, &len, sizeof(u8));

	if (!len) {
		fst_mgr_printf(MSG_WARNING, "MBIEs of zero len received");
		res = len;
		goto out;
	}

	*mbies = os_zalloc((len * 2 + 1)* sizeof(char));
	if (!*mbies)  {
		fst_mgr_printf(MSG_ERROR, "Cannot allocate array to store MBIEs");
		res = -ENOMEM;
		goto out;
	}

	for (res = 0; res < len; res++)
		sprintf(*mbies + 2 * res, "%02x", v[res]);

out:
	if (value)
		g_variant_unref(value);
	if (res < 0 && *mbies) {
		fst_free(*mbies);
		*mbies = NULL;
	}

	return res;
}

int fst_session_get_info(u32 session_id, struct fst_session_info *info)
{
	int         res = -EINVAL;
	gchar      *session_path = NULL;
	GDBusProxy *proxy = NULL;
	GVariant   *value = NULL;
	const gchar*str;
	gsize       len;

	session_path = get_session_path(session_id);
	if (!session_path) {
		fst_mgr_printf(MSG_ERROR, "Cannot find session %u", session_id);
		goto out;
	}

	proxy = get_proxy(session_path, WPAS_DBUS_NEW_IFACE_FST_SESSION);
	if (!proxy)
		goto out;

	value = proxy_get_typed_property(proxy, FST_DBUS_SESSION_PROP_OWN_ADDR,
				G_VARIANT_TYPE_BYTESTRING);
	if (!value)
		goto out;

	str = g_variant_get_fixed_array(value, &len, sizeof(u8));
	if (len != ETH_ALEN) {
		fst_mgr_printf(MSG_ERROR, "Invalid mac address length %zd", len);
		goto out;
	}
	os_memcpy(info->own_addr, str, ETH_ALEN);
	g_variant_unref(value);

	value = proxy_get_typed_property(proxy, FST_DBUS_SESSION_PROP_PEER_ADDR,
				G_VARIANT_TYPE_BYTESTRING);
	if (!value)
		goto out;

	str = g_variant_get_fixed_array(value, &len, sizeof(u8));
	if (len != ETH_ALEN) {
		fst_mgr_printf(MSG_ERROR, "Invalid mac address length %zd", len);
		goto out;
	}
	os_memcpy(info->peer_addr, str, ETH_ALEN);
	g_variant_unref(value);

	value = proxy_get_typed_property(proxy, FST_DBUS_SESSION_PROP_OLD_IFACE,
				G_VARIANT_TYPE_STRING);
	if (!value)
		goto out;

	str = g_variant_get_string(value, &len);
	if (len < 0 || len >= sizeof(info->old_ifname)) {
		fst_mgr_printf(MSG_ERROR, "Invalid ifname length %zd", len);
		goto out;
	}
	if (len)
		os_strlcpy(info->old_ifname, str, sizeof(info->old_ifname));
	else
		os_memset(info->old_ifname, 0, sizeof(info->old_ifname));
	g_variant_unref(value);

	value = proxy_get_typed_property(proxy, FST_DBUS_SESSION_PROP_NEW_IFACE,
				G_VARIANT_TYPE_STRING);
	if (!value)
		goto out;

	str = g_variant_get_string(value, &len);
	if (len < 0 || len >= sizeof(info->new_ifname)) {
		fst_mgr_printf(MSG_ERROR, "Invalid ifname length %zd", len);
		goto out;
	}
	if (len)
		os_strlcpy(info->new_ifname, str, sizeof(info->new_ifname));
	else
		os_memset(info->old_ifname, 0, sizeof(info->old_ifname));
	g_variant_unref(value);

	value = proxy_get_typed_property(proxy, FST_DBUS_SESSION_PROP_LLT,
				G_VARIANT_TYPE_UINT32);
	if (!value)
		goto out;

	info->llt = g_variant_get_uint32(value);
	g_variant_unref(value);

	value = proxy_get_typed_property(proxy, FST_DBUS_SESSION_PROP_STATE,
				G_VARIANT_TYPE_UINT32);
	if (!value)
		goto out;

	info->state = (enum fst_session_state)g_variant_get_uint32(value);
	g_variant_unref(value);

	value = proxy_get_typed_property(proxy, FST_DBUS_SESSION_PROP_ID,
				G_VARIANT_TYPE_UINT32);
	if (!value)
		goto out;

	info->session_id = g_variant_get_uint32(value);
	g_variant_unref(value);

	value = NULL;
	res   = 0;

out:
	if (session_path)
		g_free(session_path);
	if (value)
		g_variant_unref(value);
	if (proxy)
		g_object_unref(proxy);

	return res;
}

int fst_session_add(const char * group_id, u32 * session_id)
{
	int         res = -EINVAL;
	GDBusProxy *proxy;
    GVariant   *value, *sub;
	GError     *error = NULL;
	char        group_path[WPAS_DBUS_OBJECT_PATH_MAX];

	assert(group_id != NULL);
	assert(session_id != NULL);

	format_group_opath(group_id, group_path, sizeof(group_path));

	proxy = get_proxy(group_path, WPAS_DBUS_NEW_IFACE_FST_GROUP);
	if (!proxy)
		goto out;

	value = g_dbus_proxy_call_sync(proxy, FST_DBUS_GROUP_MTHD_ADD_SESSION,
				NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
	if (error) {
      fst_mgr_printf(MSG_ERROR, "Error calling method: %s", error->message);
      g_error_free(error);
	  goto out;
    }

	if (!value) {
		fst_mgr_printf(MSG_ERROR, "Cannot get result");
		goto out;
	}

	sub = g_variant_get_child_value(value, 0);
	if (!sub) {
		printerr_variant(value, "Incorrect variant");
		goto out;
	}

	if (!g_variant_is_of_type(sub, G_VARIANT_TYPE_UINT32)) {
		printerr_variant(sub, "Incorrect variant");
		goto out;
	}

	*session_id = g_variant_get_uint32(sub);
	res = 0;

out:
	if (sub)
		g_variant_unref(sub);
	if (value)
		g_variant_unref(value);
	if (proxy)
		g_object_unref(proxy);

	return res;
}

int fst_session_remove(u32 session_id)
{
	return session_call_method(session_id, FST_DBUS_SESSION_MTHD_REMOVE, NULL);
}

int fst_session_set(u32 session_id, const char *pname, const char * pval)
{
	return session_call_method(session_id, FST_DBUS_SESSION_MTHD_SET,
			g_variant_new("(ss)", pname, pval));
}

int fst_session_initiate(u32 session_id)
{
	return session_call_method(session_id, FST_DBUS_SESSION_MTHD_INITIATE,
			NULL);
}

int fst_session_respond(u32 session_id, const char *response_status)
{
	return session_call_method(session_id, FST_DBUS_SESSION_MTHD_RESPOND,
			g_variant_new("(s)", response_status));
}

int fst_session_transfer(u32 session_id)
{
	return session_call_method(session_id, FST_DBUS_SESSION_MTHD_TRANSFER,
			NULL);
}

int fst_session_teardown(u32 session_id)
{
	return session_call_method(session_id, FST_DBUS_SESSION_MTHD_TEARDOWN,
			NULL);
}

static void wpa_supplicant_appeared_clb(GDBusConnection *connection,
		const gchar *name,
		const gchar *name_owner,
		gpointer user_data)
{
	g_assert_nonnull(dbus_ctrl);
	g_assert_nonnull(dbus_ctrl->connection);
	g_assert_cmpint(dbus_ctrl->signal_id, ==, 0);

	if (fst_manager_init()) {
		fst_mgr_printf(MSG_ERROR, "cannot init fst manager");
		goto error_fst_manager_init;
	}

	dbus_ctrl->signal_id = g_dbus_connection_signal_subscribe(
			dbus_ctrl->connection,
			WPAS_DBUS_NEW_SERVICE,
			WPAS_DBUS_NEW_IFACE_FST,
			FST_DBUS_GLOBAL_SIG,
			WPAS_DBUS_NEW_PATH_FST,
			NULL,
			G_DBUS_SIGNAL_FLAGS_NONE,
			fst_ctrl_on_global_signal,
			NULL,
			NULL);

	if (!dbus_ctrl->signal_id) {
		fst_mgr_printf(MSG_ERROR, "cannot subscribe to global signal");
		goto error_signal_subscribe;
	}

	return;

error_signal_subscribe:
	fst_manager_deinit();
error_fst_manager_init:
       return;
}

static void wpa_supplicant_vanished_clb(GDBusConnection *connection,
		const gchar *name,
		gpointer user_data)
{
	g_assert_nonnull(dbus_ctrl);
	g_assert_nonnull(dbus_ctrl->connection);

	if (dbus_ctrl->signal_id) {
		g_dbus_connection_signal_unsubscribe(dbus_ctrl->connection,
				dbus_ctrl->signal_id);
		fst_manager_deinit();
		dbus_ctrl->signal_id = 0;
	}
}

Boolean fst_ctrl_create(void)
{
	GError *error = NULL;

	assert(dbus_ctrl == NULL);

	dbus_ctrl = os_zalloc(sizeof(*dbus_ctrl));
	if (!dbus_ctrl) {
		fst_mgr_printf(MSG_ERROR, "Cannot allocate dBus ctrl object");
		goto error_alloc;
	}

	dbus_ctrl->connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
	if (!dbus_ctrl->connection) {
		fst_mgr_printf(MSG_ERROR, "Cannot get connection: %s", error->message);
		goto error_g_bus_get_sync;
	}

	dbus_ctrl->watch_id = g_bus_watch_name_on_connection(dbus_ctrl->connection,
			WPAS_DBUS_NEW_SERVICE,
			G_BUS_NAME_WATCHER_FLAGS_NONE,
			wpa_supplicant_appeared_clb,
			wpa_supplicant_vanished_clb,
			NULL,
			NULL);
	if (!dbus_ctrl->watch_id) {
		fst_mgr_printf(MSG_ERROR, "Cannot setup watch");
		goto error_setup_watch;
	}

	return TRUE;

error_setup_watch:
	g_object_unref(dbus_ctrl->connection);
error_g_bus_get_sync:
	os_free(dbus_ctrl);
	dbus_ctrl = NULL;
	if (error)
		g_error_free(error);
error_alloc:
	return FALSE;
}

void fst_ctrl_free(void)
{
	g_assert_nonnull(dbus_ctrl);
	wpa_supplicant_vanished_clb(dbus_ctrl->connection,
		WPAS_DBUS_NEW_SERVICE,
		dbus_ctrl);
	g_object_unref(dbus_ctrl->connection);
	os_free(dbus_ctrl);
	dbus_ctrl = NULL;
}
