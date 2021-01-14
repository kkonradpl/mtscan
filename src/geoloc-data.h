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

#ifndef MTSCAN_GEOLOC_DATA_H_
#define MTSCAN_GEOLOC_DATA_H_

typedef struct geoloc_data geoloc_data_t;

geoloc_data_t* geoloc_data_new(const gchar*, gdouble, gdouble);
void geoloc_data_free(geoloc_data_t*);

const gchar* geoloc_data_get_ssid(const geoloc_data_t*);
gdouble geoloc_data_get_lat(const geoloc_data_t*);
gdouble geoloc_data_get_lon(const geoloc_data_t*);
gboolean geoloc_data_is_vaild(const geoloc_data_t*);

#endif
