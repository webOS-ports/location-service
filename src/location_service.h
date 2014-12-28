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

struct location_service;

struct location_service* location_service_create();
void location_service_free(struct location_service *service);

#endif
