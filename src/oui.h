/*
 *  MTscan - MikroTik RouterOS wireless scanner
 *  Copyright (c) 2015-2024  Konrad Kosmatka
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

#ifndef MTSCAN_OUI_H_
#define MTSCAN_OUI_H_

gboolean oui_init(const gchar*);
const gchar* oui_lookup(gint64);
void oui_destroy();

#endif

