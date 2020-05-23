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

#ifndef LOCATION_SERVICE_H_
#define LOCATION_SERVICE_H_

#include <glib/gi18n.h>
#include <gio/gio.h>

struct location_service {
	LSHandle *handle_ports1;
	LSHandle *handle_ports2;
	LSHandle *handle_palm1;
	LSHandle *handle_palm2;
	LSHandle *handle_webos1;
	LSHandle *handle_webos2;
	GDBusProxy *manager;
	GDBusProxy *client_props;
	GDBusProxy *subscribed_client;
	int num_clients_ports1;
	int num_clients_ports2;
	int num_clients_palm1;
	int num_clients_palm2;
	int num_clients_webos1;
	int num_clients_webos2;
};

bool location_service_register(struct location_service *service, LSHandle **handle, const char *name);
void location_service_unregister(LSHandle *handle);

#endif
