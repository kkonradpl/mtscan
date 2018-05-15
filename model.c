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

#define _GNU_SOURCE
#include <string.h>
#include <math.h>
#include "model.h"
#include "signals.h"
#include "ui-icons.h"
#include "conf.h"
#include "misc.h"

#define UNIX_TIMESTAMP() (g_get_real_time() / 1000000)
#define GPS_DOUBLE_PREC (1e-6)
#define AZI_FLOAT_PREC (1e-2)

#define MIKROTIK_LOW_SIGNAL_BUGFIX  1
#define MIKROTIK_HIGH_SIGNAL_BUGFIX 1

enum
{
    MODEL_NETWORK_UPDATE,
    MODEL_NETWORK_NEW,
    MODEL_NETWORK_NEW_HIGHLIGHT,
    MODEL_NETWORK_NEW_ALARM
};

static gint model_sort_ascii_string(GtkTreeModel*, GtkTreeIter*, GtkTreeIter*, gpointer);
static gint model_sort_rssi(GtkTreeModel*, GtkTreeIter*, GtkTreeIter*, gpointer);
static gint model_sort_double(GtkTreeModel*, GtkTreeIter*, GtkTreeIter*, gpointer);
static gint model_sort_float(GtkTreeModel*, GtkTreeIter*, GtkTreeIter*, gpointer);
static gint model_sort_version(GtkTreeModel*, GtkTreeIter*, GtkTreeIter*, gpointer);
static void model_free_foreach(gpointer, gpointer, gpointer);
static gboolean model_clear_active_foreach(gpointer, gpointer, gpointer);
static gint model_update_network(mtscan_model_t*, network_t*);

static void trim_zeros(gchar*);

mtscan_model_t*
mtscan_model_new(void)
{
    mtscan_model_t *model = g_malloc(sizeof(mtscan_model_t));
    model->store = gtk_list_store_new(COL_COUNT,
                                      G_TYPE_UCHAR,    /* COL_STATE     */
                                      G_TYPE_INT64,    /* COL_ADDRESS   */
                                      G_TYPE_INT,      /* COL_FREQUENCY */
                                      G_TYPE_STRING,   /* COL_CHANNEL   */
                                      G_TYPE_STRING,   /* COL_MODE      */
                                      G_TYPE_STRING,   /* COL_SSID      */
                                      G_TYPE_STRING,   /* COL_RADIONAME */
                                      G_TYPE_CHAR,     /* COL_MAXRSSI   */
                                      G_TYPE_CHAR,     /* COL_RSSI      */
                                      G_TYPE_CHAR,     /* COL_NOISE     */
                                      G_TYPE_BOOLEAN,  /* COL_PRIVACY   */
                                      G_TYPE_BOOLEAN,  /* COL_ROUTEROS  */
                                      G_TYPE_BOOLEAN,  /* COL_NSTREME   */
                                      G_TYPE_BOOLEAN,  /* COL_TDMA      */
                                      G_TYPE_BOOLEAN,  /* COL_WDS       */
                                      G_TYPE_BOOLEAN,  /* COL_BRIDGE    */
                                      G_TYPE_STRING,   /* COL_ROS_VER   */
                                      G_TYPE_INT64,    /* COL_FIRSTSEEN */
                                      G_TYPE_INT64,    /* COL_LASTSEEN  */
                                      G_TYPE_DOUBLE,   /* COL_LATITUDE  */
                                      G_TYPE_DOUBLE,   /* COL_LONGITUDE */
                                      G_TYPE_FLOAT,    /* COL_AZIMUTH   */
                                      G_TYPE_POINTER); /* COL_SIGNALS   */

    gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(model->store), COL_SSID, model_sort_ascii_string, GINT_TO_POINTER(COL_SSID), NULL);
    gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(model->store), COL_RADIONAME, model_sort_ascii_string, GINT_TO_POINTER(COL_RADIONAME), NULL);
    gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(model->store), COL_RSSI, model_sort_rssi, NULL, NULL);
    gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(model->store), COL_LATITUDE, model_sort_double, GINT_TO_POINTER(COL_LATITUDE), NULL);
    gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(model->store), COL_LONGITUDE, model_sort_double, GINT_TO_POINTER(COL_LONGITUDE), NULL);
    gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(model->store), COL_AZIMUTH, model_sort_float, GINT_TO_POINTER(COL_AZIMUTH), NULL);
    gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(model->store), COL_ROUTEROS_VER, model_sort_version, GINT_TO_POINTER(COL_ROUTEROS_VER), NULL);

    model->map = g_hash_table_new_full(g_int64_hash, g_int64_equal, g_free, (GDestroyNotify)gtk_tree_iter_free);
    model->active = g_hash_table_new_full(g_int64_hash, g_int64_equal, NULL, NULL);
    model->active_timeout = MODEL_DEFAULT_ACTIVE_TIMEOUT;
    model->new_timeout = MODEL_DEFAULT_NEW_TIMEOUT;
    model->disabled_sorting = FALSE;
    model->buffer = NULL;
	model->clear_active_all = FALSE;
    return model;
}


static gint
model_sort_ascii_string(GtkTreeModel *model,
                        GtkTreeIter  *a,
                        GtkTreeIter  *b,
                        gpointer      data)
{
    gint column = GPOINTER_TO_INT(data);
    gchar *v1, *v2;
    gint ret;

    gtk_tree_model_get(model, a,
                       column, &v1,
                       -1);

    gtk_tree_model_get(model, b,
                       column, &v2,
                       -1);

    /* Don't care about UTF-8 chars now,
       as these are escaped by RouterOS */
    ret = g_ascii_strcasecmp(v1, v2);

    g_free(v1);
    g_free(v2);
    return ret;
}

static gint
model_sort_rssi(GtkTreeModel *model,
                GtkTreeIter  *a,
                GtkTreeIter  *b,
                gpointer      data)
{
    guint8 state, state2;
    gint8 rssi, rssi2;
    gint64 lastseen, lastseen2;

    gtk_tree_model_get(model, a,
                       COL_STATE, &state,
                       COL_RSSI, &rssi,
                       COL_LASTSEEN, &lastseen,
                       -1);

    gtk_tree_model_get(model, b,
                       COL_STATE, &state2,
                       COL_RSSI, &rssi2,
                       COL_LASTSEEN, &lastseen2,
                       -1);

    if(state == MODEL_STATE_INACTIVE && state2 == MODEL_STATE_INACTIVE)
    {
        if(lastseen - lastseen2 == 0)
            return rssi - rssi2;
        else
            return lastseen - lastseen2;
    }
    else if(state != MODEL_STATE_INACTIVE && state2 == MODEL_STATE_INACTIVE)
    {
        return 1;
    }
    else if(state == MODEL_STATE_INACTIVE && state2 != MODEL_STATE_INACTIVE)
    {
        return -1;
    }
    else
    {
        return rssi - rssi2;
    }
}

static gint
model_sort_double(GtkTreeModel *model,
                  GtkTreeIter  *a,
                  GtkTreeIter  *b,
                  gpointer      data)
{
    gint column = GPOINTER_TO_INT(data);
    gdouble v1, v2, diff;
    gboolean v1_isnan, v2_isnan;

    gtk_tree_model_get(model, a,
                       column, &v1,
                       -1);

    gtk_tree_model_get(model, b,
                       column, &v2,
                       -1);

    v1_isnan = isnan(v1);
    v2_isnan = isnan(v2);

    if(v1_isnan && v2_isnan)
        return 0;

    if(!v1_isnan && v2_isnan)
        return -1;

    if(v1_isnan && !v2_isnan)
        return 1;

    diff = v1 - v2;
    if(fabs(diff) < GPS_DOUBLE_PREC)
        return 0;

    if(diff < 0)
        return -1;

    return 1;
}

static gint
model_sort_float(GtkTreeModel *model,
                 GtkTreeIter  *a,
                 GtkTreeIter  *b,
                 gpointer      data)
{
    gint column = GPOINTER_TO_INT(data);
    gfloat v1, v2, diff;
    gboolean v1_isnan, v2_isnan;

    gtk_tree_model_get(model, a,
                       column, &v1,
                       -1);

    gtk_tree_model_get(model, b,
                       column, &v2,
                       -1);

    v1_isnan = isnan(v1);
    v2_isnan = isnan(v2);

    if(v1_isnan && v2_isnan)
        return 0;

    if(!v1_isnan && v2_isnan)
        return -1;

    if(v1_isnan && !v2_isnan)
        return 1;

    diff = v1 - v2;
    if(fabs(diff) < AZI_FLOAT_PREC)
        return 0;

    if(diff < 0)
        return -1;

    return 1;
}

static gint
model_sort_version(GtkTreeModel *model,
                   GtkTreeIter  *a,
                   GtkTreeIter  *b,
                   gpointer      data)
{
    gint column = GPOINTER_TO_INT(data);
    gchar *v1, *v2;
    gint ret;

    gtk_tree_model_get(model, a,
                       column, &v1,
                       -1);

    gtk_tree_model_get(model, b,
                       column, &v2,
                       -1);
#ifndef G_OS_WIN32
    ret = strverscmp(v1, v2);
#else
    ret = strcasecmp(v1, v2);
#endif
    g_free(v1);
    g_free(v2);
    return ret;
}

void
mtscan_model_free(mtscan_model_t *model)
{
    g_hash_table_foreach(model->map, model_free_foreach, model);
    g_hash_table_destroy(model->map);
    g_hash_table_destroy(model->active);
    g_object_unref(model->store);
    g_free(model);
}

static void
model_free_foreach(gpointer key,
                   gpointer value,
                   gpointer data)
{
    mtscan_model_t *model = (mtscan_model_t*)data;
    GtkTreeModel *store = (GtkTreeModel*)model->store;
    GtkTreeIter *iter = (GtkTreeIter*)value;
    signals_t *signals;

    gtk_tree_model_get(store, iter,
                       COL_SIGNALS, &signals, -1);

    signals_free(signals);
}

void
mtscan_model_clear(mtscan_model_t *model)
{
    mtscan_model_buffer_clear(model);
    g_hash_table_remove_all(model->active);
    g_hash_table_foreach(model->map, model_free_foreach, model);
    g_hash_table_remove_all(model->map);
    gtk_list_store_clear(GTK_LIST_STORE(model->store));
}

void
mtscan_model_clear_active(mtscan_model_t *model)
{
    model->clear_active_all = TRUE;
	g_hash_table_foreach_remove(model->active, model_clear_active_foreach, model);
	model->clear_active_all = FALSE;
}

static gboolean
model_clear_active_foreach(gpointer key,
                           gpointer value,
                           gpointer data)
{
    mtscan_model_t *model = (mtscan_model_t*)data;
    GtkTreeModel *store = (GtkTreeModel*)model->store;
    GtkTreeIter *iter = (GtkTreeIter*)value;
    gint64 firstseen, lastseen;
    gboolean privacy;
    gint8 rssi;
    guint8 state;

    gtk_tree_model_get(store, iter,
                       COL_STATE, &state,
                       COL_PRIVACY, &privacy,
                       COL_RSSI, &rssi,
                       COL_FIRSTSEEN, &firstseen,
                       COL_LASTSEEN, &lastseen,
                       -1);

    if(model->clear_active_all ||
       UNIX_TIMESTAMP() > lastseen+model->active_timeout)
    {
        gtk_list_store_set(GTK_LIST_STORE(store), iter,
                           COL_STATE, MODEL_STATE_INACTIVE,
                           -1);
        model->clear_active_changed = TRUE;
        return TRUE;
    }

    if(state == MODEL_STATE_NEW &&
       UNIX_TIMESTAMP() > firstseen+model->new_timeout)
    {
        gtk_list_store_set(GTK_LIST_STORE(store), iter,
                           COL_STATE, MODEL_STATE_ACTIVE,
                           -1);
        model->clear_active_changed = TRUE;
    }

    return FALSE;
}

void
mtscan_model_remove(mtscan_model_t *model,
                    GtkTreeIter    *iter)
{
    gint64 address;
    signals_t *signals;

    gtk_tree_model_get(GTK_TREE_MODEL(model->store), iter,
                       COL_ADDRESS, &address,
                       COL_SIGNALS, &signals,
                       -1);

    g_hash_table_remove(model->active, &address);
    g_hash_table_remove(model->map, &address);
    signals_free(signals);
    gtk_list_store_remove(model->store, iter);
}

void
mtscan_model_buffer_add(mtscan_model_t *model,
                        network_t      *net)
{
    if(conf_get_preferences_blacklist_enabled() &&
       conf_get_preferences_blacklist(net->address))
    {
        network_free(net);
        g_free(net);
        return;
    }

    model->buffer = g_slist_prepend(model->buffer, (gpointer)net);
}

void
mtscan_model_buffer_clear(mtscan_model_t *model)
{
    GSList *current = model->buffer;
    while(current)
    {
        network_t *net = (network_t*)(current->data);
        network_free(net);
        g_free(net);
        current = current->next;
    }
    g_slist_free(model->buffer);
    model->buffer = NULL;
}

gint
mtscan_model_buffer_and_inactive_update(mtscan_model_t *model)
{
    GSList *current;
    gint state = MODEL_UPDATE_NONE;
    gint status;

    if(model->buffer)
    {
        model->buffer = g_slist_reverse(model->buffer);
        current = model->buffer;

        while(current)
        {
            network_t* net = (network_t*)(current->data);

            status = model_update_network(model, net);
            if(status == MODEL_NETWORK_NEW_ALARM)
                state |= MODEL_UPDATE_NEW_ALARM;
            else if(status == MODEL_NETWORK_NEW_HIGHLIGHT)
                state |= MODEL_UPDATE_NEW_HIGHLIGHT;
            else if(status == MODEL_NETWORK_NEW)
                state |= MODEL_UPDATE_NEW;
            else if(status == MODEL_NETWORK_UPDATE)
                state |= MODEL_UPDATE;

            network_free(net);
            g_free(net);
            current = current->next;
        }
        g_slist_free(model->buffer);
        model->buffer = NULL;
    }

    model->clear_active_changed = FALSE;
	g_hash_table_foreach_remove(model->active, model_clear_active_foreach, model);
    if(model->clear_active_changed && state == MODEL_UPDATE_NONE)
        state = MODEL_UPDATE_ONLY_INACTIVE;

    return state;
}

gint
model_update_network(mtscan_model_t *model,
                     network_t      *net)
{
    GtkTreeIter *iter_ptr;
    GtkTreeIter iter;
    gint64 *address;
    gchar *current_ssid;
    gchar *current_radioname;
    gint8 current_maxrssi;
    guint8 current_state;
    gboolean new_network_found;

    if(g_hash_table_lookup_extended(model->map, &net->address, (gpointer*)&address, (gpointer*)&iter_ptr))
    {
        /* Update a network, check current values first */
        gtk_tree_model_get(GTK_TREE_MODEL(model->store), iter_ptr,
                           COL_STATE, &current_state,
                           COL_SSID, &current_ssid,
                           COL_RADIONAME, &current_radioname,
                           COL_MAXRSSI, &current_maxrssi,
                           COL_SIGNALS, &net->signals,
                           -1);

        /* Update state to active (keep MODEL_STATE_NEW untouched) */
        if(current_state == MODEL_STATE_INACTIVE)
            current_state = MODEL_STATE_ACTIVE;

        /* Preserve hidden SSIDs */
        if((net->ssid && !net->ssid[0]) ||
           !net->ssid)
        {
            g_free(net->ssid);
            net->ssid = current_ssid;
        }
        else
        {
            g_free(current_ssid);
        }

        /* ... and Radio Names */
        if((net->radioname && !net->radioname[0]) ||
           !net->radioname)
        {
            g_free(net->radioname);
            net->radioname = current_radioname;
        }
        else
        {
            g_free(current_radioname);
        }

#if MIKROTIK_LOW_SIGNAL_BUGFIX
        if(net->rssi >= -12 && net->rssi <= -10 && current_maxrssi <= -15)
            net->rssi = current_maxrssi;
#endif
#if MIKROTIK_HIGH_SIGNAL_BUGFIX
        if(net->rssi >= 10)
            net->rssi = current_maxrssi;
#endif

        if(conf_get_preferences_signals())
            signals_append(net->signals, signals_node_new(net->firstseen, net->rssi, net->latitude, net->longitude, net->azimuth));

        /* At new signal peak, update additionally COL_MAXRSSI, COL_LATITUDE, COL_LONGITUDE and COL_AZIMUTH */
        if(net->rssi > current_maxrssi)
        {
            gtk_list_store_set(model->store, iter_ptr,
                               COL_STATE, current_state,
                               COL_FREQUENCY, net->frequency,
                               COL_CHANNEL, (net->channel ? net->channel : ""),
                               COL_MODE, (net->mode ? net->mode : ""),
                               COL_SSID, (net->ssid ? net->ssid : ""),
                               COL_RADIONAME, (net->radioname ? net->radioname : ""),
                               COL_MAXRSSI, net->rssi,
                               COL_RSSI, net->rssi,
                               COL_NOISE, net->noise,
                               COL_PRIVACY, net->flags.privacy,
                               COL_ROUTEROS, net->flags.routeros,
                               COL_NSTREME, net->flags.nstreme,
                               COL_TDMA, net->flags.tdma,
                               COL_WDS, net->flags.wds,
                               COL_BRIDGE, net->flags.bridge,
                               COL_ROUTEROS_VER, (net->routeros_ver ? net->routeros_ver : ""),
                               COL_LASTSEEN, net->firstseen,
                               COL_LATITUDE, net->latitude,
                               COL_LONGITUDE, net->longitude,
                               COL_AZIMUTH, net->azimuth,
                               -1);
        }
        else
        {
            gtk_list_store_set(model->store, iter_ptr,
                               COL_STATE, current_state,
                               COL_FREQUENCY, net->frequency,
                               COL_CHANNEL, (net->channel ? net->channel : ""),
                               COL_MODE, (net->mode ? net->mode : ""),
                               COL_SSID, (net->ssid ? net->ssid : ""),
                               COL_RADIONAME, (net->radioname ? net->radioname : ""),
                               COL_RSSI, net->rssi,
                               COL_NOISE, net->noise,
                               COL_PRIVACY, net->flags.privacy,
                               COL_ROUTEROS, net->flags.routeros,
                               COL_NSTREME, net->flags.nstreme,
                               COL_TDMA, net->flags.tdma,
                               COL_WDS, net->flags.wds,
                               COL_BRIDGE, net->flags.bridge,
                               COL_ROUTEROS_VER, (net->routeros_ver ? net->routeros_ver : ""),
                               COL_LASTSEEN, net->firstseen,
                               -1);
        }

        /* Add address to the active network list */
        g_hash_table_insert(model->active, address, iter_ptr);
        new_network_found = MODEL_NETWORK_UPDATE;
    }
    else
    {
        /* Add a new network */
        net->signals = signals_new();
        if(conf_get_preferences_signals())
            signals_append(net->signals, signals_node_new(net->firstseen, net->rssi, net->latitude, net->longitude, net->azimuth));

        gtk_list_store_insert_with_values(model->store, &iter, -1,
                                          COL_STATE, MODEL_STATE_NEW,
                                          COL_ADDRESS, net->address,
                                          COL_FREQUENCY, net->frequency,
                                          COL_CHANNEL, (net->channel ? net->channel : ""),
                                          COL_MODE, (net->mode ? net->mode : ""),
                                          COL_SSID, (net->ssid ? net->ssid : ""),
                                          COL_RADIONAME, (net->radioname ? net->radioname : ""),
                                          COL_MAXRSSI, net->rssi,
                                          COL_RSSI, net->rssi,
                                          COL_NOISE, net->noise,
                                          COL_PRIVACY, net->flags.privacy,
                                          COL_ROUTEROS, net->flags.routeros,
                                          COL_NSTREME, net->flags.nstreme,
                                          COL_TDMA, net->flags.tdma,
                                          COL_WDS, net->flags.wds,
                                          COL_BRIDGE, net->flags.bridge,
                                          COL_ROUTEROS_VER, (net->routeros_ver ? net->routeros_ver : ""),
                                          COL_FIRSTSEEN, net->firstseen,
                                          COL_LASTSEEN, net->firstseen,
                                          COL_LATITUDE, net->latitude,
                                          COL_LONGITUDE, net->longitude,
                                          COL_AZIMUTH, net->azimuth,
                                          COL_SIGNALS, net->signals,
                                          -1);

        iter_ptr = gtk_tree_iter_copy(&iter);
        address = gint64dup(&net->address);

        g_hash_table_insert(model->map, address, iter_ptr);
        g_hash_table_insert(model->active, address, iter_ptr);
        if(conf_get_preferences_alarmlist_enabled() && conf_get_preferences_alarmlist(*address))
            new_network_found = MODEL_NETWORK_NEW_ALARM;
        else if(conf_get_preferences_highlightlist_enabled() && conf_get_preferences_highlightlist(*address))
            new_network_found = MODEL_NETWORK_NEW_HIGHLIGHT;
        else
            new_network_found = MODEL_NETWORK_NEW;
    }

    /* Signals are stored in GtkListStore just as pointer,
       so set it to NULL before freeing the struct */
    net->signals = NULL;
    return new_network_found;
}

void
mtscan_model_add(mtscan_model_t *model,
                 network_t      *net,
                 gboolean        merge)
{
    GtkTreeIter *iter_merge;
    GtkTreeIter iter;
    gint8 current_maxrssi;
    gint64 current_firstseen;
    gint64 current_lastseen;
    signals_t *current_signals;
    gint64 *address;

    if(merge && (iter_merge = g_hash_table_lookup(model->map, &net->address)))
    {
        /* Merge a network, check current values first */
        gtk_tree_model_get(GTK_TREE_MODEL(model->store), iter_merge,
                           COL_MAXRSSI, &current_maxrssi,
                           COL_FIRSTSEEN, &current_firstseen,
                           COL_LASTSEEN, &current_lastseen,
                           COL_SIGNALS, &current_signals,
                           -1);

        /* Merge signal samples */
        signals_merge(current_signals, net->signals);

        /* Update the first seen date, if required */
        if(net->firstseen < current_firstseen)
        {
            gtk_list_store_set(model->store, iter_merge,
                               COL_FIRSTSEEN, net->firstseen,
                               -1);
        }

        /* Update the last seen date along with other values, if required */
        if(net->lastseen > current_lastseen)
        {
            gtk_list_store_set(model->store, iter_merge,
                               COL_FREQUENCY, net->frequency,
                               COL_CHANNEL, (net->channel ? net->channel : ""),
                               COL_MODE, (net->mode ? net->mode : ""),
                               COL_SSID, (net->ssid ? net->ssid : ""),
                               COL_RADIONAME, (net->radioname ? net->radioname : ""),
                               COL_PRIVACY, net->flags.privacy,
                               COL_ROUTEROS, net->flags.routeros,
                               COL_NSTREME, net->flags.nstreme,
                               COL_TDMA, net->flags.tdma,
                               COL_WDS, net->flags.wds,
                               COL_BRIDGE, net->flags.bridge,
                               COL_ROUTEROS_VER, (net->routeros_ver ? net->routeros_ver : ""),
                               COL_LASTSEEN, net->lastseen,
                               -1);
        }

        /* Update the max signal level together with its coordinates, if required */
        if(net->rssi > current_maxrssi)
        {
            gtk_list_store_set(model->store, iter_merge,
                               COL_MAXRSSI, net->rssi,
                               COL_LATITUDE, net->latitude,
                               COL_LONGITUDE, net->longitude,
                               COL_AZIMUTH, net->azimuth,
                               -1);
        }
    }
    else
    {
        /* Add a new network */
        gtk_list_store_insert_with_values(model->store, &iter, -1,
                                          COL_STATE, MODEL_STATE_INACTIVE,
                                          COL_ADDRESS, net->address,
                                          COL_FREQUENCY, net->frequency,
                                          COL_CHANNEL, (net->channel ? net->channel : ""),
                                          COL_MODE, (net->mode ? net->mode : ""),
                                          COL_SSID, (net->ssid ? net->ssid : ""),
                                          COL_RADIONAME, (net->radioname ? net->radioname : ""),
                                          COL_MAXRSSI, net->rssi,
                                          COL_RSSI, MODEL_NO_SIGNAL,
                                          COL_NOISE, net->noise,
                                          COL_PRIVACY, net->flags.privacy,
                                          COL_ROUTEROS, net->flags.routeros,
                                          COL_NSTREME, net->flags.nstreme,
                                          COL_TDMA, net->flags.tdma,
                                          COL_WDS, net->flags.wds,
                                          COL_BRIDGE, net->flags.bridge,
                                          COL_ROUTEROS_VER, (net->routeros_ver ? net->routeros_ver : ""),
                                          COL_FIRSTSEEN, net->firstseen,
                                          COL_LASTSEEN, net->lastseen,
                                          COL_LATITUDE, net->latitude,
                                          COL_LONGITUDE, net->longitude,
                                          COL_AZIMUTH, net->azimuth,
                                          COL_SIGNALS, net->signals,
                                          -1);


        address = gint64dup(&net->address);
        g_hash_table_insert(model->map, address, gtk_tree_iter_copy(&iter));

        /* Signals are stored in GtkListStore just as pointer,
           so set it to NULL before freeing the struct */
        net->signals = NULL;
    }
}

void
mtscan_model_set_active_timeout(mtscan_model_t *model,
                                gint            timeout)
{
    model->active_timeout = timeout;
}

void
mtscan_model_set_new_timeout(mtscan_model_t *model,
                             gint            timeout)
{
    model->new_timeout = timeout;
}

void
mtscan_model_disable_sorting(mtscan_model_t *model)
{
    if(gtk_tree_sortable_get_sort_column_id(GTK_TREE_SORTABLE(model->store), &model->last_sort_column, &model->last_sort_order))
        gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(model->store), GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID, model->last_sort_order);
    model->disabled_sorting = TRUE;
}

void
mtscan_model_enable_sorting(mtscan_model_t *model)
{
    if(model->disabled_sorting)
    {
        gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(model->store), model->last_sort_column, model->last_sort_order);
        model->disabled_sorting = FALSE;
    }
}

const gchar*
model_format_address(gint64   address,
                     gboolean separated)
{
    static gchar str[18];

    if(!separated)
    {
        g_snprintf(str, sizeof(str), "%012" G_GINT64_MODIFIER "X", address);
    }
    else
    {
        g_snprintf(str, sizeof(str),
                   "%02X:%02X:%02X:%02X:%02X:%02X",
                   (gint)((address >> 40) & 0xFF),
                   (gint)((address >> 32) & 0xFF),
                   (gint)((address >> 24) & 0xFF),
                   (gint)((address >> 16) & 0xFF),
                   (gint)((address >>  8) & 0xFF),
                   (gint)((address      ) & 0xFF));
    }
    return str;
}

const gchar*
model_format_frequency(gint value)
{
    static gchar output[9];
    gint frac, i;

    if((frac = value % 1000))
    {
        g_snprintf(output, sizeof(output), "%d.%03d", value/1000, frac);
        for(i=strlen(output)-1; i>=0 && output[i] == '0'; i--);
        output[i+1] = '\0';
        return output;
    }

    g_snprintf(output, sizeof(output), "%d", value/1000);
    return output;
}

const gchar*
model_format_date(gint64 value)
{
    static gchar output[20];
    struct tm *tm;
    static gint day_now, month_now, year_now;
#ifdef G_OS_WIN32
    static __time64_t cached = 0;
    __time64_t ts_val, ts_now;

    ts_val = (__time64_t)value;
    ts_now = _time64(NULL);
    if(ts_now != cached)
    {
        tm = _localtime64(&ts_now);
        day_now = tm->tm_mday;
        month_now = tm->tm_mon;
        year_now = tm->tm_year;
        cached = ts_now;
    }
    tm = _localtime64(&ts_val);
#else
    static time_t cached = 0;
    time_t ts_val, ts_now;

    ts_val = (time_t)value;
    ts_now = time(NULL);
    if(ts_now != cached)
    {
        tm = localtime(&ts_now);
        day_now = tm->tm_mday;
        month_now = tm->tm_mon;
        year_now = tm->tm_year;
        cached = ts_now;
    }
    tm = localtime(&ts_val);
#endif

    if(tm->tm_mday == day_now &&
       tm->tm_mon == month_now &&
       tm->tm_year == year_now)
    {
        strftime(output, sizeof(output), "%H:%M:%S", tm);
    }
    else
    {
        strftime(output, sizeof(output), "%Y-%m-%d %H:%M:%S", tm);
    }

    return output;
}

const gchar*
model_format_gps(gdouble  value,
                 gboolean trim)
{
    static gchar output[12];
    if(!isnan(value))
    {
        g_ascii_formatd(output, sizeof(output), "%.6f", value);
        if(trim)
            trim_zeros(output);
    }
    else
    {
        *output = '\0';
    }
    return output;
}

const gchar*
model_format_azimuth(gfloat   value,
                     gboolean trim)
{
    static gchar output[12];
    if(!isnan(value))
    {
        g_ascii_formatd(output, sizeof(output), "%.2f", value);
        if(trim)
            trim_zeros(output);
    }
    else
    {
        *output = '\0';
    }
    return output;
}

static void
trim_zeros(gchar *string)
{
    gint i;
    for(i=strlen(string)-1; i>=0 && string[i] == '0'; i--);
    if(i >= 0 && string[i] == '.')
        i++;
    string[i+1] = '\0';
}
