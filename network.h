/*
 *  MTscan - MikroTik RouterOS wireless scanner
 *  Copyright (c) 2015-2017  Konrad Kosmatka
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
    gboolean privacy;
    gboolean routeros;
    gboolean nstreme;
    gboolean tdma;
    gboolean wds;
    gboolean bridge;
} network_flags_t;

typedef struct network
{
    gint64 address;
    gint frequency;
    gchar *channel;
    gchar *mode;
    gchar *ssid;
    gchar *radioname;
    gint8 rssi;
    gint8 noise;
    network_flags_t flags;
    gchar *routeros_ver;
    gint64 firstseen;
    gint64 lastseen;
    gdouble latitude;
    gdouble longitude;
    gfloat azimuth;
    signals_t *signals;
} network_t;

void network_init(network_t*);
void network_free(network_t*);
void network_free_null(network_t*);

#endif

