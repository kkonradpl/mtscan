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

#ifndef MTSCAN_UI_H_
#define MTSCAN_UI_H_
#include <gtk/gtk.h>
#include "mtscan.h"
#include "model.h"
#include "ui-connection.h"
#include "mt-ssh.h"
#include "ui-scanlist.h"

#define UNIX_TIMESTAMP() (g_get_real_time() / 1000000)

enum
{
    MTSCAN_MODE_NONE,
    MTSCAN_MODE_SCANNER,
    MTSCAN_MODE_SNIFFER
};

typedef struct mtscan_gtk
{
    GtkWidget *window;
    GtkWidget *box;
    GtkWidget *box_toolbar;

    GtkWidget *toolbar;
    GtkToolItem *b_connect;
    GtkToolItem *b_scan;
    GtkToolItem *b_sniff;
    GtkToolItem *b_restart;
    GtkToolItem *b_scanlist_default;
    GtkToolItem *b_scanlist;
    GtkToolItem *b_scanlist_preset;

    GtkToolItem *b_new;
    GtkToolItem *b_open;
    GtkToolItem *b_merge;
    GtkToolItem *b_save;
    GtkToolItem *b_save_as;
    GtkToolItem *b_export;
    GtkToolItem *b_screenshot;

    GtkToolItem *b_preferences;
    GtkToolItem *b_sound;
    GtkToolItem *b_mode;
    GtkToolItem *b_autosave;
    GtkToolItem *b_gps;
    GtkToolItem *b_about;

    GtkWidget *scroll;
    GtkWidget *treeview;

    GtkWidget *statusbar_align;
    GtkWidget *statusbar;
    GtkWidget *activity_icon;
    GtkWidget *l_net_status;
    GtkWidget *l_conn_status;
    GtkWidget *group_gps, *l_gps_status;

    mtscan_model_t *model;
    gboolean changed;
    gchar *filename;
    gchar *name;
    gint64 log_ts;

    ui_connection_t *conn_dialog;
    ui_scanlist_t *scanlist;

    mt_ssh_t *conn;
    gint mode;
    gboolean connected;
    gint activity;
    gint64 activity_ts;
    guint activity_timeout;
    guint data_timeout;
} mtscan_gtk_t;

mtscan_gtk_t ui;

void ui_init(void);

void ui_connected(const gchar*, const gchar*, const gchar*);
void ui_disconnected(void);
void ui_changed(void);
gboolean ui_can_discard_unsaved(void);
void ui_status_update_networks(void);

void ui_set_title(gchar*);
void ui_clear(void);
void ui_show_uri(const gchar*);
void ui_screenshot(void);

gboolean ui_log_save(gchar*, gboolean, gboolean, gboolean, GList*, gboolean);
gboolean ui_log_save_full(gchar*, gboolean, gboolean, gboolean, GList*, gboolean);

void ui_toggle_connection(gint);

#endif
