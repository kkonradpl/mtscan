/*
 *  MTscan - MikroTik RouterOS wireless scanner
 *  Copyright (c) 2015-2020  Konrad Kosmatka
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
#include "log.h"
#include "misc.h"
#include "network.h"

static void conf_extlist_callback(network_t*, gpointer);

gboolean
conf_extlist_load(GTree       *list,
                  const gchar *filename)
{
    gint status;

    if(!filename || !strlen(filename))
    {
        fprintf(stderr, "conf_extlist_load: no file given\n");
        return FALSE;
    }

    status = log_read(filename, conf_extlist_callback, list, TRUE);

    if(status == LOG_READ_ERROR_OPEN)
        fprintf(stderr, "conf_extlist_load: cannot open file: %s\n", filename);
    else if(status == LOG_READ_ERROR_READ)
        fprintf(stderr, "conf_extlist_load: cannot read file: %s\n", filename);
    else if(status == LOG_READ_ERROR_PARSE)
        fprintf(stderr, "conf_extlist_load: cannot parse file: %s\n", filename);

    return status > 0;
}

static void
conf_extlist_callback(network_t *network,
                      gpointer   user_data)
{
    GTree *output = (GTree*)user_data;

    /* Validate network address */
    if(network->address < 0)
        return;

    g_tree_insert(output, gint64dup(&network->address), GINT_TO_POINTER(TRUE));
}
