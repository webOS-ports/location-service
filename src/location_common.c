/* @@@LICENSE
*
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

#include "location_common.h"

void location_to_reply(GDBusProxy *location, jvalue_ref *reply_obj)
{
	GVariant *value;
	gdouble latitude, longitude, accuracy, altitude;

	value = g_dbus_proxy_get_cached_property (location, "Latitude");
	latitude = g_variant_get_double (value);
	g_variant_unref(value);
	value = g_dbus_proxy_get_cached_property (location, "Longitude");
	longitude = g_variant_get_double (value);
	g_variant_unref(value);
	value = g_dbus_proxy_get_cached_property (location, "Accuracy");
	accuracy = g_variant_get_double (value);
	g_variant_unref(value);
        value = g_dbus_proxy_get_cached_property (location, "Altitude");
        altitude = g_variant_get_double (value);
	g_variant_unref(value);
        if (altitude == -G_MAXDOUBLE) altitude = -1;

	jobject_put(*reply_obj, J_CSTR_TO_JVAL("returnValue"), jboolean_create(true));
	jobject_put(*reply_obj, J_CSTR_TO_JVAL("errorCode"), jnumber_create_i32(0));
	jobject_put(*reply_obj, J_CSTR_TO_JVAL("altitude"), jnumber_create_f64(altitude));
	jobject_put(*reply_obj, J_CSTR_TO_JVAL("heading"), jnumber_create_f64(-1));
	jobject_put(*reply_obj, J_CSTR_TO_JVAL("horizAccuracy"), jnumber_create_f64(accuracy));
	jobject_put(*reply_obj, J_CSTR_TO_JVAL("latitude"), jnumber_create_f64(latitude));
	jobject_put(*reply_obj, J_CSTR_TO_JVAL("longitude"), jnumber_create_f64(longitude));
	jobject_put(*reply_obj, J_CSTR_TO_JVAL("timestamp"), jnumber_create_f64(time(NULL)));
	jobject_put(*reply_obj, J_CSTR_TO_JVAL("velocity"), jnumber_create_f64(-1));
	jobject_put(*reply_obj, J_CSTR_TO_JVAL("vertAccuracy"), jnumber_create_f64(-1));
}
