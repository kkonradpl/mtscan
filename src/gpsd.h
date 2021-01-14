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

#ifndef MTSCAN_GPSD_H_
#define MTSCAN_GPSD_H_

typedef struct gpsd_conn gpsd_conn_t;
typedef struct gpsd_msg  gpsd_msg_t;
typedef struct gpsd_data gpsd_data_t;

typedef enum gpsd_msg_type
{
    GPSD_MSG_INFO,
    GPSD_MSG_DATA
} gpsd_msg_type_t;

typedef enum gpsd_msg_info
{
    GPSD_INFO_RESOLVING,
    GPSD_INFO_ERR_RESOLV,
    GPSD_INFO_CONNECTING,
    GPSD_INFO_ERR_CONN,
    GPSD_INFO_ERR_TIMEOUT,
    GPSD_INFO_ERR_MISMATCH,
    GPSD_INFO_CONNECTED,
    GPSD_INFO_DISCONNECTED
} gpsd_msg_info_t;

typedef enum gpsd_mode
{
    GPSD_MODE_INVALID = 0,
    GPSD_MODE_NONE = 1,
    GPSD_MODE_2D = 2,
    GPSD_MODE_3D = 3
} gpsd_mode_t;

gpsd_conn_t*
gpsd_conn_new(void        (*cb)(gpsd_conn_t*),
              void        (*cb_msg)(const gpsd_conn_t*, gpsd_msg_type_t, gconstpointer),
              const gchar  *hostname,
              gint          port,
              guint         reconnect);

void gpsd_conn_free(gpsd_conn_t*);
void gpsd_conn_cancel(gpsd_conn_t*);
const gchar* gpsd_conn_get_hostname(const gpsd_conn_t*);
const gchar* gpsd_conn_get_port(const gpsd_conn_t*);

const gchar* gpsd_data_get_device(const gpsd_data_t*);
gpsd_mode_t  gpsd_data_get_mode(const gpsd_data_t*);
gint64       gpsd_data_get_time(const gpsd_data_t*);
gdouble      gpsd_data_get_ept(const gpsd_data_t*);
gdouble      gpsd_data_get_lat(const gpsd_data_t*);
gdouble      gpsd_data_get_lon(const gpsd_data_t*);
gdouble      gpsd_data_get_alt(const gpsd_data_t*);
gdouble      gpsd_data_get_epx(const gpsd_data_t*);
gdouble      gpsd_data_get_epy(const gpsd_data_t*);
gdouble      gpsd_data_get_epv(const gpsd_data_t*);
gdouble      gpsd_data_get_track(const gpsd_data_t*);
gdouble      gpsd_data_get_speed(const gpsd_data_t*);
gdouble      gpsd_data_get_climb(const gpsd_data_t*);
gdouble      gpsd_data_get_eps(const gpsd_data_t*);
gdouble      gpsd_data_get_epc(const gpsd_data_t*);

#endif
