/*
 *  MTscan - MikroTik RouterOS wireless scanner
 *  Copyright (c) 2015-2019  Konrad Kosmatka
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */

#ifndef MTSCAN_GEOLOC_DATABASE_H_
#define MTSCAN_GEOLOC_DATABASE_H_

#include "geoloc-data.h"

typedef GHashTable geoloc_database_t;

geoloc_database_t* geoloc_database_new();
guint geoloc_database_size(geoloc_database_t*);
geoloc_data_t* geoloc_database_lookup(geoloc_database_t*, gint64);
void geoloc_database_insert(geoloc_database_t*, gint64, geoloc_data_t*);
void geoloc_database_remove(geoloc_database_t*, gint64);
void geoloc_database_free(geoloc_database_t*);

#endif
