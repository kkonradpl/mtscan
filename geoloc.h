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

#ifndef MTSCAN_GEOLOC_H_
#define MTSCAN_GEOLOC_H_
#include "geoloc-data.h"

void geoloc_init(const gchar* const*,  void(*)(gint64));
void geoloc_reinit(const gchar* const*);
void geoloc_wigle(const gchar*, const gchar*);

const geoloc_data_t* geoloc_match(gint64, const gchar*, gfloat, gboolean, gfloat*);

#endif
