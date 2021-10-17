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

#ifndef MTSCAN_GNSS_WSA_H_
#define MTSCAN_GNSS_WSA_H_
#include "msg.h"

typedef struct gnss_wsa gnss_wsa_t;

gnss_wsa_t*
gnss_wsa_new(void (*cb)(gnss_wsa_t*),
             void (*cb_msg)(const gnss_msg_t*),
             gint,
             guint);

void gnss_wsa_free(gnss_wsa_t*);
void gnss_wsa_cancel(gnss_wsa_t*);
gint gnss_wsa_get_sensor_id(const gnss_wsa_t*);

#endif
