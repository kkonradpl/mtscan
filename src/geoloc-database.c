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
#include "geoloc-database.h"

geoloc_database_t*
geoloc_database_new()
{
    return g_hash_table_new_full(g_int64_hash,
                                 g_int64_equal,
                                 g_free,
                                 (GDestroyNotify)geoloc_data_free);
}

guint
geoloc_database_size(geoloc_database_t *database)
{
    return g_hash_table_size(database);
}

geoloc_data_t*
geoloc_database_lookup(geoloc_database_t *database,
                       gint64             bssid)
{
    return g_hash_table_lookup(database, &bssid);
}

void
geoloc_database_insert(geoloc_database_t *database,
                       gint64             bssid,
                       geoloc_data_t     *data)
{
    gint64 *key = g_malloc(sizeof(gint64));
    *key = bssid;
    g_hash_table_insert(database, key, data);
}

void
geoloc_database_remove(geoloc_database_t *database,
                       gint64             bssid)
{
    g_hash_table_remove(database, &bssid);
}

void
geoloc_database_free(geoloc_database_t *database)
{
    g_hash_table_destroy(database);
}
