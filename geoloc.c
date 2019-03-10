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
#include "geoloc.h"
#include "geoloc-database.h"
#include "geoloc-utils.h"
#include "wigle/wigle.h"
#include "network.h"
#include "log.h"
#include "conf.h"

typedef struct geoloc_loader_t
{
    GSList *databases;
    GSList *filenames;
    size_t count;
} geoloc_loader_t;

typedef struct geoloc
{
    gboolean            init;
    geoloc_loader_t    *loader;
    wigle_t            *wigle;
    GSList             *db_mtscan;
    geoloc_database_t  *db_wigle;
    void              (*callback)(gint64);
} geoloc_t;

static geoloc_t geoloc =
{
    .init = FALSE,
    .loader = NULL,
    .wigle = NULL,
    .db_mtscan = NULL,
    .db_wigle = NULL,
    .callback = NULL
};

static gpointer geoloc_loader(gpointer);
static gboolean geoloc_loader_callback(gpointer);
static void geoloc_loader_network_callback(network_t*, gpointer);

static void geoloc_wigle_cb(wigle_t*);
static void geoloc_wigle_cb_msg(const wigle_t*, const wigle_data_t*);

static gboolean geoloc_match_ssid(const geoloc_data_t*, const gchar*);
static gboolean geoloc_match_azimuth(const geoloc_data_t*, gfloat);
static gboolean geoloc_match_distance(const geoloc_data_t*, gfloat, gfloat*);


void
geoloc_init(const gchar* const  *filenames,
            void               (*callback)(gint64))
{
    geoloc.callback = callback;

    if(!geoloc.init)
        geoloc_reinit(filenames);
    else if(geoloc.callback)
        geoloc.callback(-1);

    geoloc.init = TRUE;
}

void
geoloc_reinit(const gchar* const *filenames)
{
    geoloc_loader_t *context;
    GSList *list = NULL;
    const gchar* const *f;

    if(filenames)
    {
        for(f = filenames; *f; f++)
            list = g_slist_append(list, g_strdup(*f));

        context = g_malloc(sizeof(geoloc_loader_t));
        context->databases = NULL;
        context->filenames = list;
        context->count = 0;

        geoloc.loader = context;
        g_thread_unref(g_thread_new("geoloc_loader", geoloc_loader, context));
    }

    if(!geoloc.db_wigle)
        geoloc.db_wigle = geoloc_database_new();

}

static gpointer
geoloc_loader(gpointer user_data)
{
    geoloc_loader_t *context = (geoloc_loader_t*)user_data;
    geoloc_database_t *database;
    const gchar *filename;
    GSList *it;
    gint count;

    for(it = context->filenames; it; it = it->next)
    {
        filename = (const gchar*)it->data;
        database = geoloc_database_new();

        count = log_read(filename,
                         geoloc_loader_network_callback,
                         database,
                         TRUE);

        if(!count)
        {
            geoloc_database_free(database);
            continue;
        }

        context->databases = g_slist_append(context->databases, database);
        context->count += geoloc_database_size(database);
    }

    g_idle_add(geoloc_loader_callback, context);
    return NULL;
}

static gboolean
geoloc_loader_callback(gpointer user_data)
{
    geoloc_loader_t *context = (geoloc_loader_t*)user_data;

    if(geoloc.loader == context)
    {
        if(geoloc.db_mtscan)
            g_slist_free_full(geoloc.db_mtscan, (GDestroyNotify)geoloc_database_free);

        g_print("geoloc_mtscan: %zu entries in local databases\n", context->count);

        geoloc.db_mtscan = context->databases;
        geoloc.loader = NULL;

        /* Update all networks */
        if(conf_get_interface_geoloc() && geoloc.callback)
            geoloc.callback(-1);
    }
    else
    {
        g_slist_free_full(context->databases, (GDestroyNotify)geoloc_database_free);
    }

    g_slist_free_full(context->filenames, g_free);
    g_free(context);

    return G_SOURCE_REMOVE;
}

static void
geoloc_loader_network_callback(network_t *network,
                               gpointer   user_data)
{
    geoloc_database_t *database = (geoloc_database_t*)user_data;
    geoloc_data_t *data;

    /* Validate network address */
    if(network->address < 0)
        return;

    /* Validate network location */
    if(isnan(network->latitude) || isnan(network->longitude))
        return;

    data = geoloc_data_new(network->ssid,
                           network->latitude,
                           network->longitude);

    geoloc_database_insert(database, network->address, data);
}

void
geoloc_wigle(const gchar *url,
             const gchar *key)
{
    if(!geoloc.wigle)
        geoloc.wigle = wigle_new(geoloc_wigle_cb, geoloc_wigle_cb_msg, url, key);
    else
        wigle_set_config(geoloc.wigle, url, key);
}

static void
geoloc_wigle_cb(wigle_t *src)
{
    if(geoloc.wigle == src)
        geoloc.wigle = NULL;

    wigle_free(src);
}

static void
geoloc_wigle_cb_msg(const wigle_t      *src,
                    const wigle_data_t *wigle_data)
{
    geoloc_data_t* data;
    gint64 bssid;

    if(geoloc.wigle != src)
        return;

    bssid = wigle_data_get_bssid(wigle_data);

    if(wigle_data_get_error(wigle_data) != WIGLE_ERROR_NONE)
    {
        /* Remove network placeholder from cache on failure */
        geoloc_database_remove(geoloc.db_wigle, bssid);
        return;
    }

    if(!wigle_data_get_match(wigle_data))
        return;

    if(isnan(wigle_data_get_lat(wigle_data)) ||
       isnan(wigle_data_get_lon(wigle_data)))
        return;

    data = geoloc_data_new(wigle_data_get_ssid(wigle_data),
                           wigle_data_get_lat(wigle_data),
                           wigle_data_get_lon(wigle_data));

    geoloc_database_insert(geoloc.db_wigle, bssid, data);

    /* Update single network */
    if(conf_get_interface_geoloc() && geoloc.callback)
        geoloc.callback(bssid);
}

const geoloc_data_t*
geoloc_match(gint64       bssid,
             const gchar* ssid,
             gfloat       azimuth,
             gboolean     query_wigle,
             gfloat      *distance_out)
{
    const geoloc_data_t *data;
    geoloc_database_t *database;
    GSList *it;
    gboolean ssid_match = FALSE;

    /* We need to check each local database from MTscan logs,
     * starting from the first, which has the highest priority */
    if(conf_get_preferences_location_mtscan() &&
       geoloc.db_mtscan)
    {
        for(it = geoloc.db_mtscan; it; it = it->next)
        {
            database = (geoloc_database_t*)it->data;
            data = geoloc_database_lookup(database, bssid);

            if(data &&
               geoloc_data_is_vaild(data) &&
               geoloc_match_ssid(data, ssid))
            {
                ssid_match = TRUE;
                if(geoloc_match_azimuth(data, azimuth) &&
                   geoloc_match_distance(data, conf_get_preferences_location_max_distance(), distance_out))
                {
                    return data;
                }
            }
        }
    }

    /* Check WiGLE cache */
    if(conf_get_preferences_location_wigle() &&
       geoloc.db_wigle)
    {
        data = geoloc_database_lookup(geoloc.db_wigle, bssid);

        if(data &&
           geoloc_data_is_vaild(data) &&
           geoloc_match_ssid(data, ssid) &&
           geoloc_match_azimuth(data, azimuth) &&
           geoloc_match_distance(data, conf_get_preferences_location_max_distance(), distance_out))
        {
            return data;
        }

        if(query_wigle &&
           geoloc.wigle &&
           !data &&
           !ssid_match)
        {
            /* No data in cache, dispatch WiGLE lookup */
            wigle_lookup(geoloc.wigle, bssid);

            /* Create empty cache entry as placeholder */
            geoloc_database_insert(geoloc.db_wigle, bssid, geoloc_data_new(NULL, NAN, NAN));
        }
    }

    return NULL;
}

static gboolean
geoloc_match_ssid(const geoloc_data_t *data,
                  const gchar         *ssid)
{
    const gchar *g_ssid;
    g_ssid = geoloc_data_get_ssid(data);

    if(ssid && g_ssid &&
       *ssid && *g_ssid &&
       strcmp(ssid, g_ssid) != 0)
        return FALSE;

    return TRUE;
}

static gboolean
geoloc_match_azimuth(const geoloc_data_t *data,
                     gfloat               azimuth)
{
    gfloat g_azimuth;

    if(isnan(azimuth))
        return TRUE;

    g_azimuth = (gfloat)geoloc_utils_azimuth(conf_get_preferences_location_latitude(),
                                             conf_get_preferences_location_longitude(),
                                             geoloc_data_get_lat(data),
                                             geoloc_data_get_lon(data));

    return geoloc_utils_azimuth_match(g_azimuth, azimuth, conf_get_preferences_location_azimuth_error());
}

static gboolean
geoloc_match_distance(const geoloc_data_t *data,
                      gfloat               distance_limit,
                      gfloat              *distance_out)
{
    gfloat g_distance;
    gboolean ret;

    g_distance = (gfloat)geoloc_utils_distance(conf_get_preferences_location_latitude(),
                                               conf_get_preferences_location_longitude(),
                                               geoloc_data_get_lat(data),
                                               geoloc_data_get_lon(data));

    ret = (g_distance <= distance_limit);

    if(ret && distance_out)
        *distance_out = g_distance;

    return ret;
}
