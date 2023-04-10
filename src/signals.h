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

#ifndef MTSCAN_SIGNALS_H_
#define MTSCAN_SIGNALS_H_
#include <glib.h>

typedef struct signals_node
{
    struct signals_node *next;
    gint64 timestamp;
    gdouble latitude;
    gdouble longitude;
    gfloat altitude;
    gfloat accuracy;
    gint8 rssi;
    gfloat azimuth;
} signals_node_t;

typedef struct signals
{
    signals_node_t *head;
    signals_node_t *tail;
} signals_t;

signals_t* signals_new(void);
signals_node_t* signals_node_new0(void);
signals_node_t* signals_node_new(gint64, gint8, gdouble, gdouble, gfloat, gfloat, gfloat);
void signals_append(signals_t*, signals_node_t*);
void signals_merge(signals_t*, signals_t*);
void signals_free(signals_t*);

#endif
