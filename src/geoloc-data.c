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
#include <glib.h>
#include <math.h>

typedef struct geoloc_data
{
    gchar *ssid;
    gdouble lat;
    gdouble lon;
} geoloc_data_t;

geoloc_data_t*
geoloc_data_new(const gchar *ssid,
                gdouble      lat,
                gdouble      lon)
{
    geoloc_data_t *data;
    data = g_malloc(sizeof(geoloc_data_t));

    data->ssid = (ssid && *ssid) ? g_strdup(ssid) : NULL;
    data->lat = lat;
    data->lon = lon;

    return data;
}

void
geoloc_data_free(geoloc_data_t *data)
{
    g_free(data->ssid);
    g_free(data);
}

const gchar*
geoloc_data_get_ssid(const geoloc_data_t *data)
{
    return (data->ssid ? data->ssid : "");
}

gdouble
geoloc_data_get_lat(const geoloc_data_t *data)
{
    return data->lat;
}

gdouble
geoloc_data_get_lon(const geoloc_data_t *data)
{
    return data->lon;
}

gboolean
geoloc_data_is_vaild(const geoloc_data_t *data)
{
    return !isnan(data->lat) && !isnan(data->lon);
}

