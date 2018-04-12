/*
 *  MTscan - MikroTik RouterOS wireless scanner
 *  Copyright (c) 2015-2018  Konrad Kosmatka
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

#ifndef MTSCAN_GPS_H_
#define MTSCAN_GPS_H_

typedef enum mtscan_gps_state
{
    GPS_OFF,
    GPS_OPENING,
    GPS_AWAITING,
    GPS_NO_FIX,
    GPS_OK
} mtscan_gps_state_t;

typedef struct mtscan_gps_data
{
    gboolean valid;
    gboolean fix;
    gdouble lat;
    gdouble lon;
    gdouble alt;
    gdouble epx;
    gdouble epy;
    gdouble epv;
} mtscan_gps_data_t;

void gps_start(const gchar*, gint);
void gps_stop(void);
void gps_set_callback(void (*cb)(mtscan_gps_state_t, const mtscan_gps_data_t*, gpointer), gpointer);
mtscan_gps_state_t gps_get_data(const mtscan_gps_data_t**);

#endif
