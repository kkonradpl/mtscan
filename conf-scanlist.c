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
#include "conf-scanlist.h"

typedef struct conf_scanlist
{
    gchar *name;
    gchar *data;
    gboolean main;
} conf_scanlist_t;


conf_scanlist_t*
conf_scanlist_new(gchar    *name,
                  gchar    *data,
                  gboolean  main)
{
    conf_scanlist_t* sl = g_malloc(sizeof(conf_scanlist_t));
    sl->name = name;
    sl->data = data;
    sl->main = main;
    return sl;
}

void
conf_scanlist_free(conf_scanlist_t *sl)
{
    if(sl)
    {
        g_free(sl->name);
        g_free(sl->data);
        g_free(sl);
    }
}

const gchar*
conf_scanlist_get_name(const conf_scanlist_t *sl)
{
    return sl->name;
}

const gchar*
conf_scanlist_get_data(const conf_scanlist_t *sl)
{
    return sl->data;
}

gboolean
conf_scanlist_get_main(const conf_scanlist_t *sl)
{
    return sl->main;
}

GtkListStore*
conf_scanlist_list_new(void)
{
    return gtk_list_store_new(CONF_SCANLIST_COLS,
                              G_TYPE_STRING,    /* CONF_SCANLIST_COL_NAME */
                              G_TYPE_STRING,    /* CONF_SCANLIST_COL_DATA */
                              G_TYPE_BOOLEAN);  /* CONF_SCANLIST_COL_MAIN */
}

GtkTreeIter
conf_scanlist_list_add(GtkListStore          *model,
                       const conf_scanlist_t *sl)
{
    GtkTreeIter iter;
    gtk_list_store_append(model, &iter);
    gtk_list_store_set(model, &iter,
                       CONF_SCANLIST_COL_NAME, sl->name,
                       CONF_SCANLIST_COL_DATA, sl->data,
                       CONF_SCANLIST_COL_MAIN, sl->main,
                       -1);
    return iter;
}

conf_scanlist_t*
conf_scanlist_list_get(GtkListStore *model,
                       GtkTreeIter  *iter)
{
    conf_scanlist_t* sl = g_malloc(sizeof(conf_scanlist_t));
    gtk_tree_model_get(GTK_TREE_MODEL(model), iter,
                       CONF_SCANLIST_COL_NAME, &sl->name,
                       CONF_SCANLIST_COL_DATA, &sl->data,
                       CONF_SCANLIST_COL_MAIN, &sl->main,
                       -1);
    return sl;
}