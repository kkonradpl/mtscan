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

#ifndef MTSCAN_UI_VIEW_H_
#define MTSCAN_UI_VIEW_H_
#include <gtk/gtk.h>
#include "model.h"

enum
{
    MTSCAN_VIEW_COL_INVALID = -1,
    MTSCAN_VIEW_COL_ACTIVITY = 0,
    MTSCAN_VIEW_COL_ADDRESS,
    MTSCAN_VIEW_COL_FREQUENCY,
    MTSCAN_VIEW_COL_MODE,
    MTSCAN_VIEW_COL_SPATIAL_STREAMS,
    MTSCAN_VIEW_COL_CHANNEL,
    MTSCAN_VIEW_COL_SSID,
    MTSCAN_VIEW_COL_RADIO_NAME,
    MTSCAN_VIEW_COL_MAX_RSSI,
    MTSCAN_VIEW_COL_RSSI,
    MTSCAN_VIEW_COL_NOISE,
    MTSCAN_VIEW_COL_PRIVACY,
    MTSCAN_VIEW_COL_ROUTEROS,
    MTSCAN_VIEW_COL_NSTREME,
    MTSCAN_VIEW_COL_TDMA,
    MTSCAN_VIEW_COL_WDS,
    MTSCAN_VIEW_COL_BRIDGE,
    MTSCAN_VIEW_COL_ROUTEROS_VER,
    MTSCAN_VIEW_COL_AIRMAX,
    MTSCAN_VIEW_COL_AIRMAX_AC_PTP,
    MTSCAN_VIEW_COL_AIRMAX_AC_PTMP,
    MTSCAN_VIEW_COL_AIRMAX_AC_MIXED,
    MTSCAN_VIEW_COL_WPS,
    MTSCAN_VIEW_COL_FIRST_LOG,
    MTSCAN_VIEW_COL_LAST_LOG,
    MTSCAN_VIEW_COL_LATITUDE,
    MTSCAN_VIEW_COL_LONGITUDE,
    MTSCAN_VIEW_COL_AZIMUTH,
    MTSCAN_VIEW_COL_DISTANCE,
    MTSCAN_VIEW_COLS
};


GtkWidget* ui_view_new(mtscan_model_t*, gint);
void ui_view_configure(GtkWidget*);
void ui_view_set_icon_size(GtkWidget*, gint);
gboolean ui_view_compare_address(GtkTreeModel*, gint, const gchar*, GtkTreeIter*, gpointer);
void ui_view_lock(GtkWidget*);
void ui_view_unlock(GtkWidget*);

void ui_view_remove_iter(GtkWidget*, GtkTreeIter*, gboolean);
void ui_view_remove_selection(GtkWidget*);
void ui_view_check_position(GtkWidget*);
void ui_view_dark_mode(GtkWidget*, gboolean);

const gchar* ui_view_get_column_title(gint);
const gchar* ui_view_get_column_name(gint);
gint ui_view_get_column_id(const gchar*);

void ui_view_set_columns_order(GtkWidget*, const gchar* const*);
void ui_view_set_columns_hidden(GtkWidget*, const gchar* const*);

#endif

