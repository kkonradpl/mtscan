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

#include <glib.h>
#include <math.h>
#include "data.h"

typedef struct gnss_data
{
    gchar *device;
    gnss_mode_t mode;
    gint64 time;
    gdouble ept;
    gdouble lat;
    gdouble lon;
    gdouble alt;
    gdouble epx;
    gdouble epy;
    gdouble epv;
    gdouble track;
    gdouble speed;
    gdouble climb;
    gdouble eps;
    gdouble epc;
} gnss_data_t;


gnss_data_t*
gnss_data_new(void)
{
    gnss_data_t *data;
    data = g_malloc(sizeof(gnss_data_t));

    data->device = NULL;
    data->time = g_get_real_time() / 1000000;
    data->mode = GNSS_MODE_INVALID;
    data->ept = NAN;
    data->lat = NAN;
    data->lon = NAN;
    data->alt = NAN;
    data->epx = NAN;
    data->epy = NAN;
    data->epv = NAN;
    data->track = NAN;
    data->speed = NAN;
    data->climb = NAN;
    data->eps = NAN;
    data->epc = NAN;
    return data;
}

void
gnss_data_free(gnss_data_t *data)
{
    if(data)
    {
        g_free(data->device);
        g_free(data);
    }
}

const gchar*
gnss_data_get_device(const gnss_data_t *data)
{
    static const gchar *unknown = "";
    return (data->device ? data->device : unknown);
}

gnss_mode_t
gnss_data_get_mode(const gnss_data_t *data)
{
    if((data->mode != GNSS_MODE_INVALID && data->mode != GNSS_MODE_NONE) &&
       (isnan(data->lat) || isnan(data->lon)))
    {
        return GNSS_MODE_INVALID;
    }

    return data->mode;
}

gint64
gnss_data_get_time(const gnss_data_t *data)
{
    return data->time;
}

gdouble
gnss_data_get_ept(const gnss_data_t *data)
{
    return data->ept;
}

gdouble
gnss_data_get_lat(const gnss_data_t *data)
{
    return data->lat;
}

gdouble
gnss_data_get_lon(const gnss_data_t *data)
{
    return data->lon;
}

gdouble
gnss_data_get_alt(const gnss_data_t *data)
{
    return data->alt;
}

gdouble
gnss_data_get_epx(const gnss_data_t *data)
{
    return data->epx;
}

gdouble
gnss_data_get_epy(const gnss_data_t *data)
{
    return data->epy;
}

gdouble
gnss_data_get_epv(const gnss_data_t *data)
{
    return data->epv;
}

gdouble
gnss_data_get_track(const gnss_data_t *data)
{
    return data->track;
}

gdouble
gnss_data_get_speed(const gnss_data_t *data)
{
    return data->speed;
}

gdouble
gnss_data_get_climb(const gnss_data_t *data)
{
    return data->climb;
}

gdouble
gnss_data_get_eps(const gnss_data_t *data)
{
    return data->eps;
}

gdouble
gnss_data_get_epc(const gnss_data_t *data)
{
    return data->epc;
}

void
gnss_data_set_device(gnss_data_t *data,
                     const gchar *value,
                     size_t       length)
{
    g_free(data->device);
    data->device = g_strndup(value, length);
}

void
gnss_data_set_mode(gnss_data_t *data,
                   gnss_mode_t  value)
{
    data->mode = value;
}

void
gnss_data_set_time(gnss_data_t *data,
                   gint64       value)
{
    data->time = value;
}

void
gnss_data_set_ept(gnss_data_t *data,
                  gdouble      value)
{
    data->ept = value;
}

void
gnss_data_set_lat(gnss_data_t *data,
                  gdouble      value)
{
    data->lat = value;
}

void
gnss_data_set_lon(gnss_data_t *data,
                  gdouble      value)
{
    data->lon = value;
}

void
gnss_data_set_alt(gnss_data_t *data,
                  gdouble      value)
{
    data->alt = value;
}

void
gnss_data_set_epx(gnss_data_t *data,
                  gdouble      value)
{
    data->epx = value;
}

void
gnss_data_set_epy(gnss_data_t *data,
                  gdouble      value)
{
    data->epy = value;
}

void
gnss_data_set_epv(gnss_data_t *data,
                  gdouble      value)
{
    data->epv = value;
}

void
gnss_data_set_track(gnss_data_t *data,
                    gdouble      value)
{
    data->track = value;
}

void
gnss_data_set_speed(gnss_data_t *data,
                    gdouble      value)
{
    data->speed = value;
}

void
gnss_data_set_climb(gnss_data_t *data,
                    gdouble      value)
{
    data->climb = value;
}

void
gnss_data_set_eps(gnss_data_t *data,
                  gdouble      value)
{
    data->eps = value;
}

void
gnss_data_set_epc(gnss_data_t *data,
                  gdouble      value)
{
    data->epc = value;
}
