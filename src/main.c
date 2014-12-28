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
#include <glib.h>
#include <stdlib.h>

#include "location_service.h"

#define VERSION						"0.1"

GMainLoop *event_loop;
static gboolean option_version = FALSE;
static gboolean option_debug = FALSE;

static GOptionEntry options[] = {
	{ "version", 'v', 0, G_OPTION_ARG_NONE, &option_version,
				"Show version information and exit" },
	{ "debug", 'd', G_OPTION_FLAG_REVERSE,
				G_OPTION_ARG_NONE, &option_debug,
				"Output debug information" },
	{ NULL },
};

static void log_handler(const gchar *log_domain, GLogLevelFlags log_level,
						const gchar *message, gpointer user_data)
{
	g_print("%s\n", message);
}

int main(int argc, char **argv)
{
	GOptionContext *context;
	GError *err = NULL;
	struct location_service *service;

	g_log_set_handler (NULL, G_LOG_LEVEL_MASK, log_handler, NULL);

	g_message("Location Service %s", VERSION);

	context = g_option_context_new(NULL);
	g_option_context_add_main_entries(context, options, NULL);

	if (g_option_context_parse(context, &argc, &argv, &err) == FALSE) {
		if (err != NULL) {
			g_printerr("%s\n", err->message);
			g_error_free(err);
			exit(1);
		}

		g_printerr("An unknown error occurred\n");
		exit(1);
	}

	g_option_context_free(context);

	if (option_version == TRUE) {
		printf("%s\n", VERSION);
		exit(0);
	}

	event_loop = g_main_loop_new(NULL, FALSE);

	service = g_try_new0(struct location_service, 1);
	if (!service)
		goto exit;
	if (!location_service_register(service, &service->handle_ports, "org.webosports.location"))
		goto exit;
	if (!location_service_register(service, &service->handle_palm, "com.palm.location"))
		goto exit;

	g_main_loop_run(event_loop);

exit:
	if (service) {
		location_service_unregister(service->handle_ports);
		location_service_unregister(service->handle_palm);
		g_free(service);
	}

	g_main_loop_unref(event_loop);

	return 0;
}

// vim:ts=4:sw=4:noexpandtab
