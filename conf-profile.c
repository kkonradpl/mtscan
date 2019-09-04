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

#include <gtk/gtk.h>
#include "conf-profile.h"

typedef struct conf_profile
{
    gchar *name;
    gchar *host;
    gint port;
    gchar *login;
    gchar *password;
    gchar *iface;
    mtscan_conf_profile_mode_t mode;
    gint duration_time;
    gboolean duration;
    gboolean remote;
    gboolean background;
} conf_profile_t;


conf_profile_t*
conf_profile_new(gchar                      *name,
                 gchar                      *host,
                 gint                        port,
                 gchar                      *login,
                 gchar                      *password,
                 gchar                      *iface,
                 mtscan_conf_profile_mode_t  mode,
                 gint                        duration_time,
                 gboolean                    duration,
                 gboolean                    remote,
                 gboolean                    background)
{
    conf_profile_t* p = g_malloc(sizeof(conf_profile_t));
    p->name = name;
    p->host = host;
    p->port = port;
    p->login = login;
    p->password = password;
    p->iface = iface;
    p->mode = mode;
    p->duration_time = duration_time;
    p->duration = duration;
    p->remote = remote;
    p->background = background;
    return p;
}

void
conf_profile_free(conf_profile_t *p)
{
    if(p)
    {
        g_free(p->name);
        g_free(p->host);
        g_free(p->login);
        g_free(p->password);
        g_free(p->iface);
        g_free(p);
    }
}

const gchar*
conf_profile_get_name(const conf_profile_t *p)
{
    return p->name;
}

const gchar*
conf_profile_get_host(const conf_profile_t *p)
{
    return p->host;
}

gint
conf_profile_get_port(const conf_profile_t *p)
{
    return p->port;
}

const gchar*
conf_profile_get_login(const conf_profile_t *p)
{
    return p->login;
}

const gchar*
conf_profile_get_password(const conf_profile_t *p)
{
    return p->password;
}

const gchar*
conf_profile_get_interface(const conf_profile_t *p)
{
    return p->iface;
}

mtscan_conf_profile_mode_t
conf_profile_get_mode(const conf_profile_t *p)
{
    return p->mode;
}

gint
conf_profile_get_duration_time(const conf_profile_t *p)
{
    return p->duration_time;
}

gint
conf_profile_get_duration(const conf_profile_t *p)
{
    return p->duration;
}

gboolean
conf_profile_get_remote(const conf_profile_t *p)
{
    return p->remote;
}

gboolean
conf_profile_get_background(const conf_profile_t *p)
{
    return p->background;
}

GtkListStore*
conf_profile_list_new(void)
{
    return gtk_list_store_new(CONF_PROFILE_COLS,
                              G_TYPE_STRING,    /* CONF_PROFILE_COL_NAME          */
                              G_TYPE_STRING,    /* CONF_PROFILE_COL_HOST          */
                              G_TYPE_INT,       /* CONF_PROFILE_COL_PORT          */
                              G_TYPE_STRING,    /* CONF_PROFILE_COL_LOGIN         */
                              G_TYPE_STRING,    /* CONF_PROFILE_COL_PASSWORD      */
                              G_TYPE_STRING,    /* CONF_PROFILE_COL_INTERFACE     */
                              G_TYPE_INT,       /* CONF_PROFILE_COL_MODE          */
                              G_TYPE_INT,       /* CONF_PROFILE_COL_DURATION_TIME */
                              G_TYPE_BOOLEAN,   /* CONF_PROFILE_COL_DURATION      */
                              G_TYPE_BOOLEAN,   /* CONF_PROFILE_COL_REMOTE        */
                              G_TYPE_BOOLEAN,   /* CONF_PROFILE_COL_BACKGROUND    */
                              G_TYPE_BOOLEAN);  /* CONF_PROFILE_COL_RECONNECT     */
}

GtkTreeIter
conf_profile_list_add(GtkListStore         *model,
                      const conf_profile_t *p)
{
    GtkTreeIter iter;
    gtk_list_store_append(model, &iter);
    gtk_list_store_set(model, &iter,
                       CONF_PROFILE_COL_NAME, p->name,
                       CONF_PROFILE_COL_HOST, p->host,
                       CONF_PROFILE_COL_PORT, p->port,
                       CONF_PROFILE_COL_LOGIN, p->login,
                       CONF_PROFILE_COL_PASSWORD, p->password,
                       CONF_PROFILE_COL_INTERFACE, p->iface,
                       CONF_PROFILE_COL_MODE, p->mode,
                       CONF_PROFILE_COL_DURATION_TIME, p->duration_time,
                       CONF_PROFILE_COL_DURATION, p->duration,
                       CONF_PROFILE_COL_REMOTE, p->remote,
                       CONF_PROFILE_COL_BACKGROUND, p->background,
                       -1);
    return iter;
}

conf_profile_t*
conf_profile_list_get(GtkListStore *model,
                      GtkTreeIter  *iter)
{
    conf_profile_t* p = g_malloc(sizeof(conf_profile_t));
    gtk_tree_model_get(GTK_TREE_MODEL(model), iter,
                       CONF_PROFILE_COL_NAME, &p->name,
                       CONF_PROFILE_COL_HOST, &p->host,
                       CONF_PROFILE_COL_PORT, &p->port,
                       CONF_PROFILE_COL_LOGIN, &p->login,
                       CONF_PROFILE_COL_PASSWORD, &p->password,
                       CONF_PROFILE_COL_INTERFACE, &p->iface,
                       CONF_PROFILE_COL_MODE, &p->mode,
                       CONF_PROFILE_COL_DURATION_TIME, &p->duration_time,
                       CONF_PROFILE_COL_DURATION, &p->duration,
                       CONF_PROFILE_COL_REMOTE, &p->remote,
                       CONF_PROFILE_COL_BACKGROUND, &p->background,
                       -1);
    return p;
}
