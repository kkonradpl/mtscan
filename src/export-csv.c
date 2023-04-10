/*
 *  MTscan - MikroTik RouterOS wireless scanner
 *  Copyright (c) 2023  Konrad Kosmatka
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

#include <string.h>
#include <glib/gstdio.h>
#include <math.h>
#include "export-csv.h"
#include "model.h"
#include "mtscan.h"

#define HEADER_LINE "WigleWifi-1.4,appRelease=2.53,model=" APP_NAME ",release=" APP_VERSION ",device=unknown,display=unknown,board=unknown,brand=MikroTik\n"
#define COLUMNS_LINE "MAC,SSID,AuthMode,FirstSeen,Channel,RSSI,CurrentLatitude,CurrentLongitude,AltitudeMeters,AccuracyMeters,Type\n"

typedef struct mtscan_export
{
    FILE *fp;
    mtscan_model_t *model;
    gboolean ret;
} mtscan_export_t;


static gboolean export_write(mtscan_export_t*, const gchar*);
static gboolean export_foreach(GtkTreeModel*, GtkTreePath*, GtkTreeIter*, gpointer);
static gint frequency_to_channel(gint freq);

gboolean
export_csv(const gchar    *filename,
           mtscan_model_t *model)
{
    mtscan_export_t e;

    e.fp = g_fopen(filename, "w");
    e.model = model;
    e.ret = TRUE;

    if (e.fp == NULL)
        return FALSE;

    if (!export_write(&e, HEADER_LINE))
        e.ret = FALSE;
    else if (!export_write(&e, COLUMNS_LINE))
        e.ret = FALSE;
    else
        gtk_tree_model_foreach(GTK_TREE_MODEL(model->store), export_foreach, &e);

    fclose(e.fp);
    return e.ret;
}

static gboolean
export_write(mtscan_export_t *e,
             const gchar     *str)
{
    size_t length = strlen(str);
    return (fwrite(str, sizeof(char), length, e->fp) == length);
}

static gboolean
export_foreach(GtkTreeModel *store,
               GtkTreePath  *path,
               GtkTreeIter  *iter,
               gpointer      data)
{
    mtscan_export_t *e = (mtscan_export_t*)data;
    network_t net;
    gchar *timestamp;
    gchar *entry;
    GDateTime *date;

    mtscan_model_get(e->model, iter, &net);
    net.signals = NULL;

    if (isnan(net.latitude) ||
        isnan(net.longitude) ||
        isnan(net.altitude) ||
        isnan(net.accuracy))
    {
        /* Skip entry with invalid location */
        network_free(&net);
        return FALSE;
    }

    date = g_date_time_new_from_unix_utc(net.firstseen);
    timestamp = g_date_time_format(date, "%Y-%m-%d %H:%M:%S");
    g_date_time_unref(date);

    entry = g_strdup_printf("%s,%s,%s,%s,%d,%d,%s,%s,%s,%s,WIFI\n",
                             model_format_address(net.address, TRUE),
                             net.ssid,
                             net.flags.privacy ? "?" : "", /* TODO */
                             timestamp,
                             frequency_to_channel(net.frequency),
                             net.rssi,
                             model_format_latitude(net.latitude, FALSE),
                             model_format_longitude(net.longitude, FALSE),
                             model_format_altitude(net.altitude),
                             model_format_accuracy(net.accuracy));

    g_free(timestamp);

    e->ret = export_write(e, entry);
    g_free(entry);
    network_free(&net);

    return (e->ret == FALSE);
}

static gint
frequency_to_channel(gint freq)
{
    freq /= 1000;

    if (freq >= 2412 && freq <= 2472)
    {
        return (freq - 2407) / 5;
    }
    else if (freq == 2484)
    {
        return 14;
    }
    else if (freq >= 5180 && freq <= 5825)
    {
        return (freq - 5000) / 5;
    }

    return 0;
}
