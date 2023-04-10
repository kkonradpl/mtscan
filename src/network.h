/*
 *  MTscan - MikroTik RouterOS wireless scanner
 *  Copyright (c) 2015-2023  Konrad Kosmatka
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

#ifndef MTSCAN_NETWORK_H_
#define MTSCAN_NETWORK_H_
#include <gtk/gtk.h>
#include "signals.h"

typedef struct network_flags
{
    gint privacy;
    gint routeros;
    gint nstreme;
    gint tdma;
    gint wds;
    gint bridge;
} network_flags_t;

typedef struct network
{
    gint64 address;
    gint frequency;
    gchar *channel;
    gchar *mode;
    guint8 streams;
    gchar *ssid;
    gchar *radioname;
    gint8 rssi;
    gint8 noise;
    network_flags_t flags;
    gchar *routeros_ver;
    gint ubnt_airmax;
    gint ubnt_ptp;
    gint ubnt_ptmp;
    gint ubnt_mixed;
    gint wps;
    gchar *wps_manufacturer;
    gchar *wps_model_name;
    gchar *wps_model_number;
    gchar *wps_serial_number;
    gchar *wps_device_name;
    gint64 firstseen;
    gint64 lastseen;
    gdouble latitude;
    gdouble longitude;
    gfloat altitude;
    gfloat accuracy;
    gfloat azimuth;
    gfloat distance;
    signals_t *signals;
} network_t;

void network_init(network_t*);
void network_to_utf8(network_t*, const gchar*);
void network_free(network_t*);
void network_free_null(network_t*);

#endif

