/*
 *  MTscan - MikroTik RouterOS wireless scanner
 *  Copyright (c) 2015-2017  Konrad Kosmatka
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

#ifndef MTSCAN_UI_VIEW_H_
#define MTSCAN_UI_VIEW_H_
#include <gtk/gtk.h>
#include "model.h"

GtkWidget* ui_view_new(mtscan_model_t*, gint);
void ui_view_configure(GtkWidget*);
void ui_view_set_icon_size(GtkWidget*, gint);
void ui_view_lock(GtkWidget*);
void ui_view_unlock(GtkWidget*);

void ui_view_remove_iter(GtkWidget*, GtkTreeIter*, gboolean);
void ui_view_remove_selection(GtkWidget*);
void ui_view_check_position(GtkWidget*);
void ui_view_dark_mode(GtkWidget*, gboolean);
void ui_view_latlon_column(GtkWidget*, gboolean);
void ui_view_azimuth_column(GtkWidget*, gboolean);

gboolean ui_view_compare_address(GtkTreeModel*, gint, const gchar*, GtkTreeIter*, gpointer);

#endif

