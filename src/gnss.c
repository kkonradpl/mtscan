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

#include <gtk/gtk.h>
#include <math.h>
#include "conf.h"
#include "gnss.h"
#include "gnss/gpsd.h"
#include "gnss/data.h"
#ifdef G_OS_WIN32
#include "gnss/wsa.h"
#endif

#define GNSS_RECONNECT_SEC    2
#define GNSS_DATA_TIMEOUT_SEC 5

typedef enum gnss_source
{
    GNSS_SOURCE_NONE,
    GNSS_SOURCE_GPSD,
#ifdef G_OS_WIN32
    GNSS_SOURCE_WSA
#endif
} gnss_source_t;

#define GNSS_ERROR_NONE 0
#define GNSS_ERROR_RESOLVING 1
#define GNSS_ERROR_CONNECTING 2
#define GNSS_ERROR_ACCESS 3

typedef struct gnss_context
{
    gpointer            conn;
    gnss_source_t       source;
    gint                error;
    gboolean            ready;
    gboolean            data_valid;
    mtscan_gnss_data_t  data;
    guint               timeout_id;
    void              (*update_cb)(mtscan_gnss_state_t, const mtscan_gnss_data_t*, gpointer);
    gpointer            update_cb_user_data;
} gnss_context_t;

static gnss_context_t gnss =
{
    .conn = NULL,
    .source = GNSS_SOURCE_NONE,
    .error = GNSS_ERROR_NONE,
    .ready = FALSE,
    .data_valid = FALSE,
    .data = {0},
    .timeout_id = 0,
    .update_cb = NULL,
    .update_cb_user_data = NULL
};

static void gnss_null(void);
static void gnss_update(void);

static gboolean gnss_cb_timeout(gpointer);
static void gnss_cb_msg(const gnss_msg_t*);
static void gnss_cb_msg_info(gnss_msg_info_t);
static void gnss_cb_msg_data(const gnss_data_t*);
static void gnss_cb_gpsd(gnss_gpsd_t*);
#ifdef G_OS_WIN32
static void gnss_cb_wsa(gnss_wsa_t*);
#endif

void
gnss_start(gint         source,
           const gchar *host,
           gint         port,
           gint         sensor_id)
{
    gnss_stop();

    if(source == CONF_PREFERENCES_GNSS_SOURCE_GPSD)
    {
        gnss.conn = gnss_gpsd_new(gnss_cb_gpsd, gnss_cb_msg, host, port, GNSS_RECONNECT_SEC);
        gnss.source = GNSS_SOURCE_GPSD;
    }
#ifdef G_OS_WIN32
    else if(source == CONF_PREFERENCES_GNSS_SOURCE_WSA)
    {
        gnss.conn = gnss_wsa_new(gnss_cb_wsa, gnss_cb_msg, sensor_id, GNSS_RECONNECT_SEC);
        gnss.source = GNSS_SOURCE_WSA;
    }
#endif

    gnss_update();
}

void
gnss_stop(void)
{
    if(gnss.conn)
    {
        if(gnss.source == GNSS_SOURCE_GPSD)
            gnss_gpsd_cancel((gnss_gpsd_t*)gnss.conn);
#ifdef G_OS_WIN32
        else if(gnss.source == GNSS_SOURCE_WSA)
            gnss_wsa_cancel((gnss_wsa_t*)gnss.conn);
#endif
        gnss_null();
    }
}

void
gnss_set_callback(void     (*cb)(mtscan_gnss_state_t, const mtscan_gnss_data_t*, gpointer),
                 gpointer   user_data)
{
    gnss.update_cb = cb;
    gnss.update_cb_user_data = user_data;
    gnss_update();
}

mtscan_gnss_state_t
gnss_get_data(const mtscan_gnss_data_t **gnss_out)
{
    if(gnss_out)
        *gnss_out = &gnss.data;

    if(!gnss.conn)
        return GNSS_OFF;

    if(gnss.error == GNSS_ERROR_ACCESS)
        return GNSS_ACCESS_DENIED;

    if(!gnss.ready)
        return GNSS_OPENING;

    if(!gnss.data_valid)
        return GNSS_AWAITING;

    if(!gnss.data.fix)
        return GNSS_NO_FIX;

    return GNSS_OK;
}

static void
gnss_null(void)
{
    if(gnss.timeout_id)
    {
        g_source_remove(gnss.timeout_id);
        gnss.timeout_id = 0;
    }

    gnss.ready = FALSE;
    gnss.conn = NULL;
    gnss.source = GNSS_SOURCE_NONE;
    gnss_update();
}

static void
gnss_update(void)
{
    mtscan_gnss_state_t state;
    const mtscan_gnss_data_t *data;
    if(gnss.update_cb)
    {
        state = gnss_get_data(&data);
        gnss.update_cb(state, data, gnss.update_cb_user_data);
    }
}


static gboolean
gnss_cb_timeout(gpointer user_data)
{
    gnss.data_valid = FALSE;
    gnss_update();
    gnss.timeout_id = 0;
    return G_SOURCE_REMOVE;
}

static void
gnss_cb_msg(const gnss_msg_t *msg)
{
    gconstpointer src = gnss_msg_get_src(msg);
    gnss_msg_type_t type = gnss_msg_get_type(msg);
    gconstpointer data = gnss_msg_get_data(msg);

    if(gnss.conn != src)
        return;

    switch(type)
    {
        case GNSS_MSG_INFO:
            gnss_cb_msg_info((gnss_msg_info_t)data);
            break;

        case GNSS_MSG_DATA:
            gnss_cb_msg_data((const gnss_data_t*)data);
            break;
    }
}

static void
gnss_cb_msg_info(gnss_msg_info_t type)
{
    switch(type)
    {
        case GNSS_INFO_OPENING:
            gnss.error = GNSS_ERROR_NONE;
            break;

        case GNSS_INFO_RESOLVING:
        case GNSS_INFO_CONNECTING:
            break;

        case GNSS_INFO_READY:
            gnss.ready = TRUE;
            gnss.data_valid = FALSE;
            gnss.error = 0;
            gnss_update();
            break;

        case GNSS_INFO_CLOSED:
            gnss.ready = FALSE;
            gnss_update();
            break;

        case GNSS_INFO_ERR_RESOLVING:
            gnss.error = GNSS_ERROR_RESOLVING;
            break;

        case GNSS_INFO_ERR_CONNECTING:
            gnss.error = GNSS_ERROR_CONNECTING;
            break;

        case GNSS_INFO_ERR_UNAVAILABLE:
            break;

        case GNSS_INFO_ERR_ACCESS:
            gnss.error = GNSS_ERROR_ACCESS;
            break;

        case GNSS_INFO_ERR_TIMEOUT:
            break;

        case GNSS_INFO_ERR_MISMATCH:
            break;
    }
}

static void
gnss_cb_msg_data(const gnss_data_t *data)
{
    gint mode = gnss_data_get_mode(data);
    gdouble epx = gnss_data_get_epx(data);
    gdouble epy = gnss_data_get_epy(data);

    if ((mode == GNSS_MODE_2D || mode == GNSS_MODE_3D) &&
        (isnan(epx) || isnan(epy)))
    {
        /* Discard incomplete data */
        return;
    }

    gnss.data_valid = (mode == GNSS_MODE_3D);
    gnss.data.fix = (mode != GNSS_MODE_INVALID && mode != GNSS_MODE_NONE);
    gnss.data.lat = gnss_data_get_lat(data);
    gnss.data.lon = gnss_data_get_lon(data);
    gnss.data.alt = gnss_data_get_alt(data);
    gnss.data.epx = epx;
    gnss.data.epy = epy;
    gnss.data.epv = gnss_data_get_epv(data);
    gnss_update();

    if(gnss.timeout_id)
        g_source_remove(gnss.timeout_id);
    gnss.timeout_id = g_timeout_add(GNSS_DATA_TIMEOUT_SEC * 1000, gnss_cb_timeout, NULL);
}

static void
gnss_cb_gpsd(gnss_gpsd_t *src)
{
    if(gnss.conn == src)
        gnss_null();

    gnss_gpsd_free(src);
}

#ifdef G_OS_WIN32
static void
gnss_cb_wsa(gnss_wsa_t *src)
{
    if(gnss.conn == src)
        gnss_null();

    gnss_wsa_free(src);
}
#endif