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
#include "wigle-data.h"

typedef struct wigle_data
{
    gint64         bssid;
    wigle_error_t  error;
    gint8          match;
    gchar         *message;
    gchar         *ssid;
    gdouble        lat;
    gdouble        lon;
} wigle_data_t;


wigle_data_t*
wigle_data_new()
{
    wigle_data_t *data;
    data = g_malloc(sizeof(wigle_data_t));

    data->error = WIGLE_ERROR_NONE;
    data->bssid = -1;
    data->match = -1;
    data->message = NULL;
    data->ssid = NULL;
    data->lat = NAN;
    data->lon = NAN;

    return data;
}

void
wigle_data_free(wigle_data_t *data)
{
    if(data)
    {
        g_free(data->message);
        g_free(data->ssid);
        g_free(data);
    }
}

gint64
wigle_data_get_bssid(const wigle_data_t *data)
{
    return data->bssid;
}

void
wigle_data_set_bssid(wigle_data_t *data,
                     gint64        bssid)
{
    data->bssid = bssid;
}

wigle_error_t
wigle_data_get_error(const wigle_data_t *data)
{
    if(data->error == WIGLE_ERROR_NONE && data->match < 0)
        return WIGLE_ERROR_UNKNOWN;

    return data->error;
}

void
wigle_data_set_error(wigle_data_t  *data,
                     wigle_error_t  error)
{
    data->error = error;
}

gboolean
wigle_data_get_match(const wigle_data_t *data)
{
    return data->match > 0;
}

void
wigle_data_set_match(wigle_data_t *data,
                     gboolean      match)
{
    data->match = (gint8)match;
}


const gchar*
wigle_data_get_message(const wigle_data_t *data)
{
    return (data->message ? data->message : "");
}

void
wigle_data_set_message(wigle_data_t *data,
                       gchar        *message)
{
    g_free(data->message);
    data->message = message;
}

const gchar*
wigle_data_get_ssid(const wigle_data_t *data)
{
    return (data->ssid ? data->ssid : "");
}

void
wigle_data_set_ssid(wigle_data_t *data,
                    gchar        *ssid)
{
    g_free(data->ssid);
    data->ssid = ssid;
}

gdouble
wigle_data_get_lat(const wigle_data_t *data)
{
    return data->lat;
}

void
wigle_data_set_lat(wigle_data_t *data,
                   gdouble       lat)
{
    data->lat = lat;
}

gdouble
wigle_data_get_lon(const wigle_data_t *data)
{
    return data->lon;
}

void
wigle_data_set_lon(wigle_data_t *data,
                   gdouble       lon)
{
    data->lon = lon;
}
