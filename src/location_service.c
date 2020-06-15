/* @@@LICENSE
*
* Copyright (c) 2014 Simon Busch <morphis@gravedo.de>
* Copyright (c) 2014 Nikolay Nizov <nizovn@gmail.com>
*
* This file is part of location-service.
*
* location-service is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* location-service is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with location-service.  If not, see <http://www.gnu.org/licenses/>.
*
* LICENSE@@@ */

#include <luna-service2/lunaservice.h>

#include "location_service.h"
#include "location_common.h"
#include "luna_service_utils.h"
#include <glib.h>
#include "utils.h"
#include <glib/gi18n.h>
#include <gio/gio.h>

extern GMainLoop *event_loop;

#define GCLUE_ACCURACY_LEVEL_HIGH GCLUE_ACCURACY_LEVEL_EXACT
#define GCLUE_ACCURACY_LEVEL_DEFAULT GCLUE_ACCURACY_LEVEL_NEIGHBORHOOD
#define GCLUE_ACCURACY_LEVEL_LOW GCLUE_ACCURACY_LEVEL_CITY

typedef enum {
	PALM_ACCURACY_LEVEL_HIGH = 1,
	PALM_ACCURACY_LEVEL_DEFAULT = 2,
	PALM_ACCURACY_LEVEL_LOW = 3,
} PalmAccuracyLevel;

typedef enum {
	CODE_Success = 0,
	CODE_Timeout = 1,
	CODE_Position_Unavailable = 2,
	CODE_Unknown = 3,
	CODE_LocationServiceOFF = 5,
	CODE_PermissionDenied = 6,
	CODE_Has_Pending_Message = 7,
	CODE_Blacklisted = 8,
} errorCode;

static void
on_client_signal (GDBusProxy *client,
                  gchar      *sender_name,
                  gchar      *signal_name,
                  GVariant   *parameters,
                  gpointer    user_data);

bool create_subscribed_client(struct location_service *service);

void luna_service_message_reply_custom_error_code(LSHandle *handle, LSMessage *message, const int error_code)
{
	bool ret;
	LSError lserror;
	char *payload;

	LSErrorInit(&lserror);

	payload = g_strdup_printf("{\"returnValue\":true, \"errorCode\":%d}", error_code);

	ret = LSMessageReply(handle, message, payload, &lserror);
	if (!ret) {
		LSErrorPrint(&lserror, stderr);
		LSErrorFree(&lserror);
	}

	g_free(payload);
}


static bool cbGetCurrentPosition(LSHandle *handle, LSMessage *message, void *user_data);
static bool cbStartTracking(LSHandle *handle, LSMessage *message, void *user_data);

static LSMethod location_service_methods[]  = {
	{ "getCurrentPosition", cbGetCurrentPosition },
	{ "startTracking", cbStartTracking },
	{ NULL, NULL }
};

static void
cb_child_watch( GPid  pid,
                gint  status,
                void *data )
{
	struct luna_service_req_data *req = data;

	if (req->subscribed) {
		luna_service_message_reply_custom_error_code(req->handle, req->message, CODE_Unknown);
		g_warning("location-getposition exited without reply: %d",status);
	}
	luna_service_req_data_free(req);
	/* Close pid */
	g_spawn_close_pid( pid );
}

static gboolean
cb_out_watch( GIOChannel   *channel,
              GIOCondition  cond,
              void *data )
{
	gchar *string;
	gsize  size;
	struct luna_service_req_data *req = data;

	if( cond == G_IO_HUP )
	{
		g_io_channel_unref( channel );
		return( FALSE );
	}

	g_io_channel_read_line( channel, &string, &size, NULL, NULL );
	LSError lserror;
	LSErrorInit(&lserror);

	if (!LSMessageReply(req->handle, req->message, string, &lserror)) {
		LSErrorPrint(&lserror, stderr);
		LSErrorFree(&lserror);
		goto cleanup;
	}
	req->subscribed = false;

cleanup:
	g_free( string );

	return( TRUE );
}

void run_client(struct luna_service_req_data *req, GClueAccuracyLevel accuracy_level)
{
	GPid pid;
	gchar *arg = g_strdup_printf("%d", accuracy_level);
	gchar *argv[] = { "/usr/sbin/location-getposition" , "-a", arg, NULL };
	gint out;
	GIOChannel *out_ch;
	gboolean ret;

	/* Spawn child process */
	ret = g_spawn_async_with_pipes( NULL, argv, NULL,
	                                G_SPAWN_DO_NOT_REAP_CHILD, NULL,
	                                NULL, &pid, NULL, &out, NULL, NULL );
	g_free(arg);
	if (!ret)
	{
		g_error ("SPAWN FAILED");
		luna_service_req_data_free(req);
		return;
	}

	/* Add watch function to catch termination of the process. This function
	 * will clean any remnants of process. subscribed bool is used to know
	 * whether error LS reply is needed in cb_out_watch callback. */
	req->subscribed = true;
	g_child_watch_add( pid, (GChildWatchFunc)cb_child_watch, req);

	/* Create channels that will be used to read data from pipes. */
	out_ch = g_io_channel_unix_new( out );

	/* Add watches to channels */
	g_io_add_watch( out_ch, G_IO_IN | G_IO_HUP, (GIOFunc)cb_out_watch, req);
}

static bool cbGetCurrentPosition(LSHandle *handle, LSMessage *message, void *user_data)
{
	jvalue_ref accuracy_obj = NULL;
	jvalue_ref parsed_obj = NULL;
	const char *payload = LSMessageGetPayload(message);
	int palm_level = PALM_ACCURACY_LEVEL_DEFAULT;
	GClueAccuracyLevel geoclue_level = GCLUE_ACCURACY_LEVEL_DEFAULT;

	parsed_obj = luna_service_message_parse_and_validate(payload);
	if (jis_null(parsed_obj)) {
		luna_service_message_reply_error_bad_json(handle, message);
		goto cleanup;
	}

	if (jobject_get_exists(parsed_obj, J_CSTR_TO_BUF("accuracy"), &accuracy_obj) &&
		jis_number(accuracy_obj)) {
		jnumber_get_i32(accuracy_obj, &palm_level);
	}
	if (palm_level == PALM_ACCURACY_LEVEL_HIGH) geoclue_level = GCLUE_ACCURACY_LEVEL_HIGH;
	if (palm_level == PALM_ACCURACY_LEVEL_LOW) geoclue_level = GCLUE_ACCURACY_LEVEL_LOW;

	struct luna_service_req_data *req = luna_service_req_data_new(handle, message);
	run_client(req, geoclue_level);

cleanup:
	if (!jis_null(parsed_obj))
		j_release(&parsed_obj);

	return true;
}

static void service_free(struct location_service *service)
{
	if (service->manager)
		g_object_unref(service->manager);
	if (service->client_props)
		g_object_unref(service->client_props);
	if (service->subscribed_client)
		g_object_unref(service->subscribed_client);
	service->manager = NULL;
	service->client_props = NULL;
	service->subscribed_client = NULL;
	service->num_clients_ports1 = 0;
	service->num_clients_ports2 = 0;
	service->num_clients_palm1 = 0;
	service->num_clients_palm2 = 0;
	service->num_clients_webos1 = 0;
	service->num_clients_webos2 = 0;

}

static void cancel_func(LSHandle* sh, LSMessage* msg, struct location_service *service)
{
	if (!g_strcmp0(LSHandleGetName(sh), LSHandleGetName(service->handle_ports1))) service->num_clients_ports1--;
	if (!g_strcmp0(LSHandleGetName(sh), LSHandleGetName(service->handle_ports2))) service->num_clients_ports2--;
	if (!g_strcmp0(LSHandleGetName(sh), LSHandleGetName(service->handle_palm1))) service->num_clients_palm1--;
	if (!g_strcmp0(LSHandleGetName(sh), LSHandleGetName(service->handle_palm2))) service->num_clients_palm2--;    
	if (!g_strcmp0(LSHandleGetName(sh), LSHandleGetName(service->handle_webos1))) service->num_clients_webos1--;
	if (!g_strcmp0(LSHandleGetName(sh), LSHandleGetName(service->handle_webos2))) service->num_clients_webos2--;
	if ((service->num_clients_ports1 + service->num_clients_ports2 + service->num_clients_palm1 + service->num_clients_palm2 + service->num_clients_webos1 + service->num_clients_webos2) <= 0) {
		GError *error = NULL;
		GVariant *results = g_dbus_proxy_call_sync(service->subscribed_client,
		                   "Stop",
		                   NULL,
		                   G_DBUS_CALL_FLAGS_NONE,
		                   -1,
		                   NULL,
		                   &error);
		if (results == NULL) {
			g_critical("Failed to stop GeoClue2 client: %s", error->message);
			g_error_free(error);
		}
		else
			g_variant_unref(results);
		service_free(service);
	}
}

static bool cbStartTracking(LSHandle *handle, LSMessage *message, void *user_data)
{
	struct location_service *service = user_data;

	bool subscribed = luna_service_check_for_subscription_and_process(handle, message);
	if (!subscribed) goto reply;

	bool is_ports1 = !g_strcmp0(LSHandleGetName(handle), LSHandleGetName(service->handle_ports1));
	bool is_ports2 = !g_strcmp0(LSHandleGetName(handle), LSHandleGetName(service->handle_ports2));
	bool is_palm1 = !g_strcmp0(LSHandleGetName(handle), LSHandleGetName(service->handle_palm1));
	bool is_palm2 = !g_strcmp0(LSHandleGetName(handle), LSHandleGetName(service->handle_palm2));
	bool is_webos1 = !g_strcmp0(LSHandleGetName(handle), LSHandleGetName(service->handle_webos1));
	bool is_webos2 = !g_strcmp0(LSHandleGetName(handle), LSHandleGetName(service->handle_webos2));

	if (service->subscribed_client) {
		if (is_ports1) service->num_clients_ports1++;
		if (is_ports2) service->num_clients_ports2++;
		if (is_palm1) service->num_clients_palm1++;
		if (is_palm2) service->num_clients_palm2++;
		if (is_webos1) service->num_clients_webos1++;
		if (is_webos2) service->num_clients_webos2++;
		goto reply;
	}

	if (!create_subscribed_client(service)) {
		luna_service_message_reply_custom_error_code(handle, message, CODE_Unknown);
		service_free(service);
		return true;
	}
	GError *error = NULL;
	GVariant *results = g_dbus_proxy_call_sync(service->subscribed_client,
	                   "Start",
	                   NULL,
	                   G_DBUS_CALL_FLAGS_NONE,
	                   -1,
	                   NULL,
	                   &error);
	if (results == NULL) {
		g_critical("Failed to start GeoClue2 client: %s", error->message);
		luna_service_message_reply_custom_error_code(handle, message, CODE_Unknown);
		g_error_free(error);
		service_free(service);
		return true;
	}
	if (is_ports1) service->num_clients_ports1++;
	if (is_ports2) service->num_clients_ports2++;
	if (is_palm1) service->num_clients_palm1++;
	if (is_palm2) service->num_clients_palm2++;
	if (is_webos1) service->num_clients_webos1++;
	if (is_webos2) service->num_clients_webos2++;

	g_variant_unref(results);
reply:
	luna_service_message_reply_success(handle, message);
	return true;
}

static void
on_client_signal (GDBusProxy *client,
                  gchar      *sender_name,
                  gchar      *signal_name,
                  GVariant   *parameters,
                  gpointer    user_data)
{
	char *location_path;
	struct location_service *service = user_data;

	if (g_strcmp0 (signal_name, "LocationUpdated") != 0)
		return;

	g_assert (g_variant_n_children (parameters) > 1);
	g_variant_get_child (parameters, 1, "&o", &location_path);
	GError *error = NULL;

	GDBusProxy *location = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
	                          G_DBUS_PROXY_FLAGS_NONE,
	                          NULL,
	                          "org.freedesktop.GeoClue2",
	                          location_path,
	                          "org.freedesktop.GeoClue2.Location",
	                          NULL,
	                          &error);
	if (error != NULL) {
		g_critical ("Failed to connect to GeoClue2 service: %s", error->message);
		g_error_free(error);
		return;
	}

	jvalue_ref reply_obj = NULL;
	reply_obj = jobject_create();
	location_to_reply(location, &reply_obj);
	g_object_unref (location);
	if (service->num_clients_ports1)
		luna_service_post_subscription(service->handle_ports1, "/", "startTracking", reply_obj);
	if (service->num_clients_ports2)
		luna_service_post_subscription(service->handle_ports2, "/", "startTracking", reply_obj);
	if (service->num_clients_palm1)
		luna_service_post_subscription(service->handle_palm1, "/", "startTracking", reply_obj);
	if (service->num_clients_palm2)
		luna_service_post_subscription(service->handle_palm2, "/", "startTracking", reply_obj);
	if (service->num_clients_webos1)
		luna_service_post_subscription(service->handle_webos1, "/", "startTracking", reply_obj);
	if (service->num_clients_webos2)
		luna_service_post_subscription(service->handle_webos2, "/", "startTracking", reply_obj);

	if (!jis_null(reply_obj))
		j_release(&reply_obj);
}

bool create_subscribed_client(struct location_service *service)
{
	GVariant *results = NULL;
	const char *client_path;
	GError *error = NULL;
	GDBusProxy *manager = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
	                          G_DBUS_PROXY_FLAGS_NONE,
	                          NULL,
	                          "org.freedesktop.GeoClue2",
	                          "/org/freedesktop/GeoClue2/Manager",
	                          "org.freedesktop.GeoClue2.Manager",
	                          NULL,
	                          &error);
	if (error != NULL) {
		g_critical ("Failed to connect to GeoClue2 service: %s", error->message);
		goto error;
	}

	results = g_dbus_proxy_call_sync (manager,
	                   "GetClient",
	                   NULL,
	                   G_DBUS_CALL_FLAGS_NONE,
	                   -1,
	                   NULL,
	                   &error);

	if (results == NULL) {
		g_critical ("Failed to connect to GeoClue2 service: %s", error->message);
		goto error;
	}

	g_assert (g_variant_n_children (results) > 0);
	g_variant_get_child (results, 0, "&o", &client_path);

	GDBusProxy *client_props = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
	                          G_DBUS_PROXY_FLAGS_NONE,
	                          NULL,
	                          "org.freedesktop.GeoClue2",
	                          client_path,
	                          "org.freedesktop.DBus.Properties",
	                          NULL,
	                          &error);

	g_variant_unref (results);
	if (error != NULL) {
		g_critical ("Failed to connect to GeoClue2 service: %s", error->message);
		goto error;
	}

	GVariant *desktop_id = g_variant_new ("s", "location-service");

	results = g_dbus_proxy_call_sync (client_props,
	                   "Set",
	                   g_variant_new ("(ssv)",
	                                  "org.freedesktop.GeoClue2.Client",
	                                  "DesktopId",
	                                  desktop_id),
	                   G_DBUS_CALL_FLAGS_NONE,
	                   -1,
	                   NULL,
	                   &error);

	if (results == NULL) {
		g_critical ("Failed to start GeoClue2 client: %s", error->message);
		goto error;
	}
	g_variant_unref (results);

	GVariant *level = g_variant_new ("u", GCLUE_ACCURACY_LEVEL_DEFAULT);

	results = g_dbus_proxy_call_sync (client_props,
	                   "Set",
	                   g_variant_new ("(ssv)",
	                                  "org.freedesktop.GeoClue2.Client",
	                                  "RequestedAccuracyLevel",
	                                  level),
	                   G_DBUS_CALL_FLAGS_NONE,
	                   -1,
	                   NULL,
	                   &error);

	if (results == NULL) {
		g_critical ("Failed to start GeoClue2 client: %s", error->message);
		goto error;
	}
	g_variant_unref (results);

	GDBusProxy *client = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
	                          G_DBUS_PROXY_FLAGS_NONE,
	                          NULL,
	                          "org.freedesktop.GeoClue2",
	                          g_dbus_proxy_get_object_path (client_props),
	                          "org.freedesktop.GeoClue2.Client",
	                          NULL,
	                          &error);
	if (error != NULL) {
		g_critical ("Failed to connect to GeoClue2 service: %s", error->message);
		goto error;
	}

	service->manager = manager;
	service->client_props = client_props;
	service->subscribed_client = client;
	g_signal_connect (client, "g-signal",
	                  G_CALLBACK (on_client_signal), service);
	return true;
error:
	g_error_free (error);
	return false;
}

bool location_service_register(struct location_service *service, LSHandle **handle, const char *name)
{
	LSError error;
	LSErrorInit(&error);

	if (!LSRegister(name, handle, &error)) {
		g_warning("Failed to register the luna service: %s", error.message);
		LSErrorFree(&error);
		goto error;
	}

	if (!LSRegisterCategory(*handle, "/", location_service_methods,
	                        NULL, NULL, &error)) {
		g_warning("Could not register service category: %s", error.message);
		LSErrorFree(&error);
		goto error;
	}

	if (!LSCategorySetData(*handle, "/", service, &error)) {
		g_warning("Could not set data for service category: %s", error.message);
		LSErrorFree(&error);
		goto error;
	}

	if (!LSGmainAttach(*handle, event_loop, &error)) {
		g_warning("Could not attach service handle to mainloop: %s", error.message);
		LSErrorFree(&error);
		goto error;
	}

	if (!LSSubscriptionSetCancelFunction(*handle, (LSFilterFunc) cancel_func, service, &error)) {
		g_warning("Could not register cancel subscription callback: %s", error.message);
		LSErrorFree(&error);
		goto error;
	}

	return true;

error:
	if (*handle != NULL) {
		LSUnregister(*handle, &error);
		LSErrorFree(&error);
	}

	return false;
}

void location_service_unregister(LSHandle *handle)
{
	LSError error;

	LSErrorInit(&error);

	if (handle != NULL && LSUnregister(handle, &error) < 0) {
		g_warning("Could not unregister service: %s", error.message);
		LSErrorFree(&error);
	}
}

// vim:ts=4:sw=4:noexpandtab
