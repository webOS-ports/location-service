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
#include "luna_service_utils.h"

extern GMainLoop *event_loop;

struct location_service {
	LSHandle *handle;
};

static bool cbGetCurrentPosition(LSHandle *handle, LSMessage *message, void *user_data);

static LSMethod location_service_methods[]  = {
	{ "getCurrentPosition", cbGetCurrentPosition },
	{ NULL, NULL }
};

static bool cbGetCurrentPosition(LSHandle *handle, LSMessage *message, void *user_data)
{
	struct location_service *service = user_data;
	jvalue_ref reply_obj = NULL;

	reply_obj = jobject_create();

	jobject_put(reply_obj, J_CSTR_TO_JVAL("returnValue"), jboolean_create(true));
	jobject_put(reply_obj, J_CSTR_TO_JVAL("errorCode"), jnumber_create_i32(0));
	jobject_put(reply_obj, J_CSTR_TO_JVAL("altitude"), jnumber_create_f64(0));
	jobject_put(reply_obj, J_CSTR_TO_JVAL("heading"), jnumber_create_f64(0));
	jobject_put(reply_obj, J_CSTR_TO_JVAL("horizAccuracy"), jnumber_create_f64(20));
	jobject_put(reply_obj, J_CSTR_TO_JVAL("latitude"), jnumber_create_f64(37.390196));
	jobject_put(reply_obj, J_CSTR_TO_JVAL("longitude"), jnumber_create_f64(-122.037845));
	jobject_put(reply_obj, J_CSTR_TO_JVAL("timestamp"), jnumber_create_f64(time(NULL)));
	jobject_put(reply_obj, J_CSTR_TO_JVAL("velocity"), jnumber_create_f64(0));
	jobject_put(reply_obj, J_CSTR_TO_JVAL("vertAccuracy"), jnumber_create_f64(0));

	if (!luna_service_message_validate_and_send(handle, message, reply_obj))
		goto cleanup;

cleanup:
	if (!jis_null(reply_obj))
		j_release(&reply_obj);

	return true;
}

struct location_service* location_service_create()
{
	struct location_service *service;
	LSError error;

	service = g_try_new0(struct location_service, 1);
	if (!service)
		return NULL;

	LSErrorInit(&error);

	if (!LSRegisterPubPriv("org.webosports.location", &service->handle, false, &error)) {
		g_warning("Failed to register the luna service: %s", error.message);
		LSErrorFree(&error);
		goto error;
	}

	if (!LSRegisterCategory(service->handle, "/", location_service_methods,
			NULL, NULL, &error)) {
		g_warning("Could not register service category: %s", error.message);
		LSErrorFree(&error);
		goto error;
	}

	if (!LSCategorySetData(service->handle, "/", service, &error)) {
		g_warning("Could not set data for service category: %s", error.message);
		LSErrorFree(&error);
		goto error;
	}

	if (!LSGmainAttach(service->handle, event_loop, &error)) {
		g_warning("Could not attach service handle to mainloop: %s", error.message);
		LSErrorFree(&error);
		goto error;
	}

	return service;

error:
	if (service->handle != NULL) {
		LSUnregister(service->handle, &error);
		LSErrorFree(&error);
	}

	g_free(service);

	return NULL;
}

void location_service_free(struct location_service *service)
{
	LSError error;

	LSErrorInit(&error);

	if (service->handle != NULL && LSUnregister(service->handle, &error) < 0) {
		g_warning("Could not unregister service: %s", error.message);
		LSErrorFree(&error);
	}

	g_free(service);
}

// vim:ts=4:sw=4:noexpandtab
