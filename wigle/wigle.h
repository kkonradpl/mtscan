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

#ifndef MTSCAN_WIGLE_H_
#define MTSCAN_WIGLE_H_

#include "wigle-data.h"

typedef struct wigle wigle_t;

wigle_t* wigle_new(void        (*cb)(wigle_t*),
                   void        (*cb_msg)(const wigle_t*, const wigle_data_t*),
                   const gchar  *addr,
                   const gchar  *key);

void wigle_set_config(wigle_t*, const gchar*, const gchar*);
void wigle_lookup(wigle_t*, gint64);
void wigle_free(wigle_t*);
void wigle_cancel(wigle_t*);


#endif
