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

#ifndef MTSCAN_GNSS_H_
#define MTSCAN_GNSS_H_

typedef enum mtscan_gnss_state
{
    GNSS_OFF,
    GNSS_OPENING,
    GNSS_ACCESS_DENIED,
    GNSS_AWAITING,
    GNSS_NO_FIX,
    GNSS_OK
} mtscan_gnss_state_t;

typedef struct mtscan_gnss_data
{
    gboolean fix;
    gdouble lat;
    gdouble lon;
    gdouble alt;
    gdouble epx;
    gdouble epy;
    gdouble epv;
} mtscan_gnss_data_t;

void gnss_start(gint, const gchar*, gint, gint);
void gnss_stop(void);
void gnss_set_callback(void (*cb)(mtscan_gnss_state_t, const mtscan_gnss_data_t*, gpointer), gpointer);
mtscan_gnss_state_t gnss_get_data(const mtscan_gnss_data_t**);

#endif
