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

#ifndef LOCATION_COMMON_H_
#define LOCATION_COMMON_H_

#include <pbnjson.h>
#include <glib/gi18n.h>
#include <gio/gio.h>

typedef enum {
	GCLUE_ACCURACY_LEVEL_COUNTRY = 1,
	GCLUE_ACCURACY_LEVEL_CITY = 4,
	GCLUE_ACCURACY_LEVEL_NEIGHBORHOOD = 5,
	GCLUE_ACCURACY_LEVEL_STREET = 6,
	GCLUE_ACCURACY_LEVEL_EXACT = 8,
} GClueAccuracyLevel;

void location_to_reply(GDBusProxy *location, jvalue_ref *reply_obj);

#endif
