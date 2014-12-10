/* @@@LICENSE
*
* Copyright (c) 2013 Simon Busch <morphis@gravedo.de>
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* LICENSE@@@ */

#include <luna-service2/lunaservice.h>
#include <glib.h>

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

	service = location_service_create();
	if (!service)
		goto exit;

	g_main_loop_run(event_loop);

exit:
	if (service)
		location_service_free(service);

	g_main_loop_unref(event_loop);

	return 0;
}

// vim:ts=4:sw=4:noexpandtab
