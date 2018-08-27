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

#ifndef MTSCAN_UI_DIALOGS_H_
#define MTSCAN_UI_DIALOGS_H_
#include <gtk/gtk.h>

enum
{
    UI_DIALOG_YES = GTK_RESPONSE_YES,
    UI_DIALOG_NO = GTK_RESPONSE_NO,
    UI_DIALOG_CANCEL = GTK_RESPONSE_CANCEL,
    UI_DIALOG_OPEN = 0,
    UI_DIALOG_MERGE = 1,
};

typedef struct
{
    GSList *filenames;
    gboolean strip_signals;
} ui_dialog_open_t;

typedef struct
{
    gchar *filename;
    gboolean compress;
    gboolean strip_signals;
    gboolean strip_gps;
    gboolean strip_azi;
} ui_dialog_save_t;

typedef struct
{
    gint value;
    gboolean strip_signals;
} ui_dialog_open_or_merge_t;

typedef struct
{
    gchar *name;
    gchar *value;
} ui_dialog_scanlist_t;

void ui_dialog(GtkWindow*, GtkMessageType, const gchar*, const gchar*, ...);

ui_dialog_open_t* ui_dialog_open(GtkWindow*, gboolean);
ui_dialog_save_t* ui_dialog_save(GtkWindow*);
gchar* ui_dialog_export(GtkWindow*);

gint ui_dialog_ask_unsaved(GtkWindow*);
ui_dialog_open_or_merge_t* ui_dialog_ask_open_or_merge(GtkWindow*);
ui_dialog_open_or_merge_t* ui_dialog_ask_merge(GtkWindow*, gint);
gint ui_dialog_yesno(GtkWindow*, const gchar*);

void ui_dialog_about(GtkWindow*);

gboolean ui_dialog_scanlist_warn(GtkWindow*, const gchar*, const gchar*);
ui_dialog_scanlist_t* ui_dialog_scanlist(GtkWindow*, gboolean);
void ui_dialog_scanlist_free(ui_dialog_scanlist_t*);

#endif
