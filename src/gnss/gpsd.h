/*
 *  MTscan - MikroTik RouterOS wireless scanner
 *  Copyright (c) 2015-2021  Konrad Kosmatka
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

#ifndef MTSCAN_GNSS_GPSD_H_
#define MTSCAN_GNSS_GPSD_H_
#include "msg.h"

typedef struct gnss_gpsd gnss_gpsd_t;

gnss_gpsd_t*
gnss_gpsd_new(void        (*cb_gpsd)(gnss_gpsd_t*),
              void        (*cb_msg)(const gnss_msg_t*),
              const gchar  *hostname,
              gint          port,
              guint         reconnect);

void gnss_gpsd_free(gnss_gpsd_t*);
void gnss_gpsd_cancel(gnss_gpsd_t*);
const gchar* gnss_gpsd_get_hostname(const gnss_gpsd_t*);
const gchar* gnss_gpsd_get_port(const gnss_gpsd_t*);

#endif
