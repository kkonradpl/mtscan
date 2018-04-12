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

#include <gtk/gtk.h>
#include <math.h>
#include "gps.h"
#include "gpsd.h"

#define GPS_RECONNECT_SEC    1
#define GPS_DATA_TIMEOUT_SEC 5

typedef struct gps_context
{
    gpsd_conn_t       *conn;
    gboolean           connected;
    guint              timeout_id;
    mtscan_gps_data_t  data;
    void             (*update_cb)(mtscan_gps_state_t, const mtscan_gps_data_t*, gpointer);
    gpointer           update_cb_user_data;
} gps_context_t;

static gps_context_t gps =
{
    .conn = NULL,
    .connected = FALSE,
    .timeout_id = 0,
    .data = {0},
    .update_cb = NULL,
    .update_cb_user_data = NULL
};

static void gps_null(void);
static void gps_update(void);
static void gps_cb(gpsd_conn_t*);
static void gps_cb_msg(const gpsd_conn_t*, gpsd_msg_type_t, gconstpointer);
static void gps_cb_msg_info(gpsd_msg_info_t);
static void gps_cb_msg_data(const gpsd_data_t*);

static gboolean gps_cb_timeout(gpointer);

void
gps_start(const gchar *host,
          gint         port)
{
    gps_stop();
    gps.conn = gpsd_conn_new(gps_cb, gps_cb_msg, host, port, GPS_RECONNECT_SEC);
    gps_update();
}

void
gps_stop(void)
{
    if(gps.conn)
    {
        gpsd_conn_cancel(gps.conn);
        gps_null();
    }
}

void
gps_set_callback(void     (*cb)(mtscan_gps_state_t, const mtscan_gps_data_t*, gpointer),
                 gpointer   user_data)
{
    gps.update_cb = cb;
    gps.update_cb_user_data = user_data;
    gps_update();
}

mtscan_gps_state_t
gps_get_data(const mtscan_gps_data_t **gps_out)
{
    if(gps_out)
        *gps_out = &gps.data;

    if(!gps.conn)
        return GPS_OFF;

    if(!gps.connected)
        return GPS_OPENING;

    if(!gps.data.valid)
        return GPS_AWAITING;

    if(!gps.data.fix ||
       isnan(gps.data.lat) ||
       isnan(gps.data.lon))
        return GPS_NO_FIX;

    return GPS_OK;
}

static void
gps_null(void)
{
    if(gps.timeout_id)
    {
        g_source_remove(gps.timeout_id);
        gps.timeout_id = 0;
    }

    gps.connected = FALSE;
    gps.conn = NULL;
    gps_update();
}

static void
gps_update(void)
{
    mtscan_gps_state_t state;
    const mtscan_gps_data_t *data;
    if(gps.update_cb)
    {
        state = gps_get_data(&data);
        gps.update_cb(state, data, gps.update_cb_user_data);
    }
}

static void
gps_cb(gpsd_conn_t *src)
{
    if(gps.conn == src)
        gps_null();

    gpsd_conn_free(src);
}

static void
gps_cb_msg(const gpsd_conn_t *src,
           gpsd_msg_type_t    type,
           gconstpointer      data)
{
    if(gps.conn != src)
        return;

    switch(type)
    {
        case GPSD_MSG_INFO:
            gps_cb_msg_info((gpsd_msg_info_t)data);
            break;

        case GPSD_MSG_DATA:
            gps_cb_msg_data((const gpsd_data_t*)data);
            break;
    }
}

static void
gps_cb_msg_info(gpsd_msg_info_t type)
{
    switch(type)
    {
        case GPSD_INFO_RESOLVING:
            break;

        case GPSD_INFO_ERR_RESOLV:
            break;

        case GPSD_INFO_CONNECTING:
            break;

        case GPSD_INFO_ERR_CONN:
            break;

        case GPSD_INFO_ERR_TIMEOUT:
            break;

        case GPSD_INFO_ERR_MISMATCH:
            break;

        case GPSD_INFO_CONNECTED:
            gps.connected = TRUE;
            gps.data.valid = FALSE;
            gps_update();
            break;

        case GPSD_INFO_DISCONNECTED:
            gps.connected = FALSE;
            gps_update();
            break;
    }
}

static void
gps_cb_msg_data(const gpsd_data_t *data)
{
    gint mode;
    mode = gpsd_data_get_mode(data);

    gps.data.fix = (mode != GPSD_MODE_INVALID && mode != GPSD_MODE_NONE);
    gps.data.lat = gpsd_data_get_lat(data);
    gps.data.lon = gpsd_data_get_lon(data);
    gps.data.alt = gpsd_data_get_alt(data);
    gps.data.epx = gpsd_data_get_epx(data);
    gps.data.epy = gpsd_data_get_epy(data);
    gps.data.epv = gpsd_data_get_epv(data);
    gps.data.valid = TRUE;
    gps_update();

    if(gps.timeout_id)
        g_source_remove(gps.timeout_id);
    gps.timeout_id = g_timeout_add(GPS_DATA_TIMEOUT_SEC * 1000, gps_cb_timeout, NULL);
}

static gboolean
gps_cb_timeout(gpointer user_data)
{
    gps.data.valid = FALSE;
    gps_update();
    gps.timeout_id = 0;
    return G_SOURCE_REMOVE;
}