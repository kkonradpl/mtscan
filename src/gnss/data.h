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

#ifndef MTSCAN_GNSS_DATA_H_
#define MTSCAN_GNSS_DATA_H_

typedef struct gnss_data gnss_data_t;

/* The enumeration of gnss_mode follows the gpsd way */
typedef enum gnss_mode
{
    GNSS_MODE_INVALID = 0,
    GNSS_MODE_NONE = 1,
    GNSS_MODE_2D = 2,
    GNSS_MODE_3D = 3
} gnss_mode_t;

gnss_data_t* gnss_data_new(void);
void gnss_data_free(gnss_data_t*);

const gchar* gnss_data_get_device(const gnss_data_t*);
gnss_mode_t  gnss_data_get_mode(const gnss_data_t*);
gint64       gnss_data_get_time(const gnss_data_t*);
gdouble      gnss_data_get_ept(const gnss_data_t*);
gdouble      gnss_data_get_lat(const gnss_data_t*);
gdouble      gnss_data_get_lon(const gnss_data_t*);
gdouble      gnss_data_get_alt(const gnss_data_t*);
gdouble      gnss_data_get_epx(const gnss_data_t*);
gdouble      gnss_data_get_epy(const gnss_data_t*);
gdouble      gnss_data_get_epv(const gnss_data_t*);
gdouble      gnss_data_get_track(const gnss_data_t*);
gdouble      gnss_data_get_speed(const gnss_data_t*);
gdouble      gnss_data_get_climb(const gnss_data_t*);
gdouble      gnss_data_get_eps(const gnss_data_t*);
gdouble      gnss_data_get_epc(const gnss_data_t*);

void gnss_data_set_device(gnss_data_t*, const gchar*, size_t);
void gnss_data_set_mode(gnss_data_t*, gnss_mode_t);
void gnss_data_set_time(gnss_data_t*, gint64);
void gnss_data_set_ept(gnss_data_t*, gdouble);
void gnss_data_set_lat(gnss_data_t*, gdouble);
void gnss_data_set_lon(gnss_data_t*, gdouble);
void gnss_data_set_alt(gnss_data_t*, gdouble);
void gnss_data_set_epx(gnss_data_t*, gdouble);
void gnss_data_set_epy(gnss_data_t*, gdouble);
void gnss_data_set_epv(gnss_data_t*, gdouble);
void gnss_data_set_track(gnss_data_t*, gdouble);
void gnss_data_set_speed(gnss_data_t*, gdouble);
void gnss_data_set_climb(gnss_data_t*, gdouble);
void gnss_data_set_eps(gnss_data_t*, gdouble);
void gnss_data_set_epc(gnss_data_t*, gdouble);

#endif
