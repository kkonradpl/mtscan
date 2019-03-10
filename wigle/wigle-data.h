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

#ifndef MTSCAN_WIGLE_DATA_H_
#define MTSCAN_WIGLE_DATA_H_

typedef struct wigle_data wigle_data_t;

typedef enum wigle_error
{
    WIGLE_ERROR_NONE           =  0,
    WIGLE_ERROR_UNKNOWN        = -1,
    WIGLE_ERROR_CONNECT        = -2,
    WIGLE_ERROR_TIMEOUT        = -3,
    WIGLE_ERROR_INCOMPLETE     = -4,
    WIGLE_ERROR_MISMATCH       = -5,
    WIGLE_ERROR_LIMIT_EXCEEDED = -6
} wigle_error_t;

wigle_data_t* wigle_data_new();
void          wigle_data_free(wigle_data_t*);

gint64        wigle_data_get_bssid(const wigle_data_t*);
void          wigle_data_set_bssid(wigle_data_t*, gint64);

wigle_error_t wigle_data_get_error(const wigle_data_t*);
void          wigle_data_set_error(wigle_data_t*, wigle_error_t);

gboolean      wigle_data_get_match(const wigle_data_t*);
void          wigle_data_set_match(wigle_data_t*, gboolean);

const gchar*  wigle_data_get_message(const wigle_data_t*);
void          wigle_data_set_message(wigle_data_t*, gchar*);

const gchar*  wigle_data_get_ssid(const wigle_data_t*);
void          wigle_data_set_ssid(wigle_data_t*, gchar*);

gdouble       wigle_data_get_lat(const wigle_data_t*);
void          wigle_data_set_lat(wigle_data_t*, gdouble);

gdouble       wigle_data_get_lon(const wigle_data_t*);
void          wigle_data_set_lon(wigle_data_t*, gdouble);

#endif
