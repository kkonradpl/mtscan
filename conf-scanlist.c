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
#include "conf-scanlist.h"

typedef struct conf_scanlist
{
    gchar *name;
    gchar *data;
    gboolean main;
    gboolean def;
} conf_scanlist_t;

static gboolean conf_scanlist_find_main_foreach(GtkTreeModel*, GtkTreePath*, GtkTreeIter*, gpointer);
static gboolean conf_scanlist_find_default_foreach(GtkTreeModel*, GtkTreePath*, GtkTreeIter*, gpointer);


conf_scanlist_t*
conf_scanlist_new(gchar    *name,
                  gchar    *data,
                  gboolean  main,
                  gboolean  def)
{
    conf_scanlist_t* sl = g_malloc(sizeof(conf_scanlist_t));
    sl->name = name;
    sl->data = data;
    sl->main = main;
    sl->def = def;
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

gboolean
conf_scanlist_get_default(const conf_scanlist_t *sl)
{
    return sl->def;
}

GtkListStore*
conf_scanlist_list_new(void)
{
    return gtk_list_store_new(CONF_SCANLIST_COLS,
                              G_TYPE_STRING,    /* CONF_SCANLIST_COL_NAME     */
                              G_TYPE_STRING,    /* CONF_SCANLIST_COL_DATA     */
                              G_TYPE_BOOLEAN,   /* CONF_SCANLIST_COL_MAIN     */
                              G_TYPE_BOOLEAN);  /* CONF_SCANLIST_COL_DEFAULT  */
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
                       CONF_SCANLIST_COL_DEFAULT, sl->def,
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
                       CONF_SCANLIST_COL_DEFAULT, &sl->def,
                       -1);
    return sl;
}

conf_scanlist_t*
conf_scanlist_find_main(GtkListStore *model)
{
    conf_scanlist_t *found = NULL;
    gtk_tree_model_foreach(GTK_TREE_MODEL(model), conf_scanlist_find_main_foreach, &found);
    return found;
}

static gboolean
conf_scanlist_find_main_foreach(GtkTreeModel *store,
                                GtkTreePath  *path,
                                GtkTreeIter  *iter,
                                gpointer      data)
{
    conf_scanlist_t **found = (conf_scanlist_t**)data;
    conf_scanlist_t *sl;

    sl = conf_scanlist_list_get(GTK_LIST_STORE(store), iter);
    if(conf_scanlist_get_main(sl))
    {
        *found = sl;
        return TRUE;
    }

    conf_scanlist_free(sl);
    return FALSE;
}

conf_scanlist_t*
conf_scanlist_find_default(GtkListStore *model)
{
    conf_scanlist_t *found = NULL;
    gtk_tree_model_foreach(GTK_TREE_MODEL(model), conf_scanlist_find_default_foreach, &found);
    return found;
}

static gboolean
conf_scanlist_find_default_foreach(GtkTreeModel *store,
                                   GtkTreePath  *path,
                                   GtkTreeIter  *iter,
                                   gpointer      data)
{
    conf_scanlist_t **found = (conf_scanlist_t*)data;
    conf_scanlist_t *sl;

    sl = conf_scanlist_list_get(GTK_LIST_STORE(store), iter);
    if(conf_scanlist_get_default(sl))
    {
        *found = sl;
        return TRUE;
    }

    conf_scanlist_free(sl);
    return FALSE;
}
