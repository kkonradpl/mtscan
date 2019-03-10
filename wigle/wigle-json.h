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

#ifndef MTSCAN_WIGLE_JSON_H_
#define MTSCAN_WIGLE_JSON_H_

typedef struct wigle_json wigle_json_t;

wigle_json_t* wigle_json_new(gint64);
wigle_data_t* wigle_json_free(wigle_json_t*);
gboolean      wigle_json_parse_chunk(wigle_json_t*, const guchar*, size_t);
gboolean      wigle_json_parse(wigle_json_t*);
wigle_data_t* wigle_json_get_data(wigle_json_t*);

#endif
