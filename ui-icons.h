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

#ifndef MTSCAN_UI_ICONS_H_
#define MTSCAN_UI_ICONS_H_
#include <gtk/gtk.h>

#define SIGNAL_LEVEL_MARGINAL_OVER    -90
#define SIGNAL_LEVEL_WEAK_OVER        -85
#define SIGNAL_LEVEL_MEDIUM_OVER      -80
#define SIGNAL_LEVEL_GOOD_OVER        -70
#define SIGNAL_LEVEL_STRONG_OVER      -60

#define SIGNAL_ICON_NONE     0xC0C0C0
#define SIGNAL_ICON_MARGINAL 0xFF8000
#define SIGNAL_ICON_WEAK     0xFFC000
#define SIGNAL_ICON_MEDIUM   0xFFFF00
#define SIGNAL_ICON_GOOD     0x80FF00
#define SIGNAL_ICON_STRONG   0x40FF00
#define SIGNAL_ICON_PERFECT  0x00FF00

void ui_icon_size(gint);
GdkPixbuf* ui_icon(gint, gint);
const gchar* ui_icon_string(gint);

#if GTK_CHECK_VERSION (3, 0, 0)
gboolean ui_icon_draw_heartbeat(GtkWidget*, cairo_t*, gpointer);
#else
gboolean ui_icon_draw_heartbeat(GtkWidget*, GdkEventExpose*, gpointer);
#endif


#endif
