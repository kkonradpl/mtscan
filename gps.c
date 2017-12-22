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

#include <gtk/gtk.h>
#include <math.h>
#ifndef G_OS_WIN32
#include <gps.h>
#endif
#include "gps.h"

#define GPS_DATA_TIMEOUT_SEC 5
#define GPS_WAITING_LOOP_MSEC 100
#define GPS_RECONNECT_DELAY_MSEC 1000

#define UNIX_TIMESTAMP() (g_get_real_time() / 1000000)

static mtscan_gps_t *gps_conn = NULL;
static mtscan_gps_data_t *gps = NULL;

static gpointer gps_thread(gpointer);
static gboolean gps_closed(gpointer);
static gboolean gps_update(gpointer);
static void gps_null(void);

void
gps_start(const gchar *host,
          gint         port)
{
    gps_stop();
    gps_conn = g_new(mtscan_gps_t, 1);
    gps_conn->host = g_strdup(host);
    gps_conn->port = g_strdup_printf("%d", port);
    gps_conn->connected = FALSE;
    gps_conn->canceled = FALSE;
    g_thread_unref(g_thread_new("gps-thread", gps_thread, gps_conn));
}

void
gps_stop(void)
{
    if(gps_conn)
        gps_conn->canceled = TRUE;
    gps_null();
}

gint
gps_get_data(const mtscan_gps_data_t **gps_out)
{
    *gps_out = gps;
    if(!gps_conn)
        return GPS_OFF;
    if(!gps_conn->connected)
        return GPS_OPENING;
    if(!gps || UNIX_TIMESTAMP() >= (gps->timestamp + GPS_DATA_TIMEOUT_SEC))
        return GPS_WAITING_FOR_DATA;
    if(!gps->fix || isnan(gps->lat) || isnan(gps->lon))
        return GPS_INVALID;
    return GPS_OK;
}

static gpointer
gps_thread(gpointer data)
{
    mtscan_gps_t *gps_conn = (mtscan_gps_t*)data;
    mtscan_gps_data_t *new_data;
#ifndef G_OS_WIN32
    struct gps_data_t gps_data;

gps_thread_reconnect:
    if(gps_conn->canceled)
        goto gps_thread_cleanup;

    if(gps_open(gps_conn->host, gps_conn->port, &gps_data) != 0)
    {
        g_usleep(GPS_RECONNECT_DELAY_MSEC*1000);
        goto gps_thread_reconnect;
    }

    if(gps_conn->canceled)
        goto gps_thread_cleanup_close;

    gps_stream(&gps_data, WATCH_ENABLE | WATCH_JSON, NULL);
    gps_conn->connected = TRUE;

    while(!gps_conn->canceled)
    {
        if(gps_waiting(&gps_data, GPS_WAITING_LOOP_MSEC*1000))
        {
            if(gps_read(&gps_data) < 0)
            {
                gps_close(&gps_data);
                gps_conn->connected = FALSE;

                g_usleep(GPS_RECONNECT_DELAY_MSEC*1000);
                goto gps_thread_reconnect;
            }

            new_data = g_new(mtscan_gps_data_t, 1);
            new_data->lat = gps_data.fix.latitude;
            new_data->lon = gps_data.fix.longitude;
            new_data->hdop = gps_data.dop.hdop;
            new_data->sat = gps_data.satellites_used;
            new_data->timestamp = UNIX_TIMESTAMP();
            new_data->source = gps_conn;

            if(gps_data.status > STATUS_NO_FIX &&
               gps_data.fix.mode > MODE_NO_FIX &&
               !isnan(gps_data.dop.hdop) &&
               !isnan(gps_data.fix.latitude) &&
               !isnan(gps_data.fix.longitude))
               {
                    new_data->fix = TRUE;
               }

            g_idle_add(gps_update, new_data);
        }
    }

gps_thread_cleanup_close:
    gps_stream(&gps_data, WATCH_DISABLE, NULL);
    gps_close(&gps_data);
gps_thread_cleanup:
#endif
    g_idle_add(gps_closed, gps_conn);
    return NULL;
}

static gboolean
gps_closed(gpointer data)
{
    mtscan_gps_t *current_conn = (mtscan_gps_t*)data;

    if(current_conn == gps_conn)
        gps_null();

    g_free(current_conn->host);
    g_free(current_conn->port);
    g_free(current_conn);
    return FALSE;
}

static gboolean
gps_update(gpointer data)
{
    mtscan_gps_data_t *new_data = (mtscan_gps_data_t*)data;
    if(new_data->source == gps_conn)
    {
        g_free(gps);
        gps = new_data;
    }
    else
    {
        g_free(new_data);
    }
    return FALSE;
}

static void
gps_null(void)
{
    g_free(gps);
    gps = NULL;
    gps_conn = NULL;
}
