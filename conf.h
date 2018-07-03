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

#ifndef MTSCAN_CONF_H_
#define MTSCAN_CONF_H_
#include <gtk/gtk.h>
#include "conf-profile.h"

/* Configuration reading & writing */
void conf_init(const gchar*);
void conf_save(void);

/* Configuration [window] */
gint conf_get_window_x(void);
gint conf_get_window_y(void);
void conf_set_window_xy(gint, gint);

gint conf_get_window_width(void);
gint conf_get_window_height(void);
void conf_set_window_position(gint, gint);

gboolean conf_get_window_maximized(void);
void conf_set_window_maximized(gboolean);

/* Configuration [interface] */
gboolean conf_get_interface_sound(void);
void conf_set_interface_sound(gboolean);

gboolean conf_get_interface_dark_mode(void);
void conf_set_interface_dark_mode(gboolean);

gboolean conf_get_interface_autosave(void);
void conf_set_interface_autosave(gboolean);

gboolean conf_get_interface_gps(void);
void conf_set_interface_gps(gboolean);

gboolean conf_get_interface_rotator(void);
void conf_set_interface_rotator(gboolean);

gint conf_get_interface_last_profile(void);
void conf_set_interface_last_profile(gint);

/* Connection profiles */
const conf_profile_t* conf_get_profile_default();
void conf_set_profile_default(conf_profile_t*);

GtkListStore* conf_get_profiles(void);
GtkTreeIter conf_profile_add(const conf_profile_t*);
conf_profile_t* conf_profile_get(GtkTreeIter*);

/* Configuration [path] */
const gchar* conf_get_path_log_open(void);
void conf_set_path_log_open(const gchar*);

const gchar* conf_get_path_log_save(void);
void conf_set_path_log_save(const gchar*);

const gchar* conf_get_path_log_export(void);
void conf_set_path_log_export(const gchar*);

const gchar* conf_get_path_autosave(void);
void conf_set_path_autosave(const gchar*);

const gchar* conf_get_path_screenshot(void);
void conf_set_path_screenshot(const gchar*);

/* Configuration [preferences] */
gint conf_get_preferences_icon_size(void);
void conf_set_preferences_icon_size(gint);

gint conf_get_preferences_autosave_interval(void);
void conf_set_preferences_autosave_interval(gint);

gint conf_get_preferences_search_column(void);
void conf_set_preferences_search_column(gint);

gboolean conf_get_preferences_signals(void);
void conf_set_preferences_signals(gboolean);

gboolean conf_get_preferences_display_time_only();
void conf_set_preferences_display_time_only(gboolean);

const gchar* const* conf_get_preferences_view_cols_order(void);
void conf_set_preferences_view_cols_order(const gchar* const*);

const gchar* const* conf_get_preferences_view_cols_hidden(void);
void conf_set_preferences_view_cols_hidden(const gchar* const*);

gboolean conf_get_preferences_sounds_new_network(void);
void conf_set_preferences_sounds_new_network(gboolean);

gboolean conf_get_preferences_sounds_new_network_hi(void);
void conf_set_preferences_sounds_new_network_hi(gboolean);

gboolean conf_get_preferences_sounds_new_network_al(void);
void conf_set_preferences_sounds_new_network_al(gboolean);

gboolean conf_get_preferences_sounds_no_data(void);
void conf_set_preferences_sounds_no_data(gboolean);

gboolean conf_get_preferences_sounds_no_gps_data(void);
void conf_set_preferences_sounds_no_gps_data(gboolean);

const gchar* conf_get_preferences_gps_hostname(void);
void conf_set_preferences_gps_hostname(const gchar*);

gint conf_get_preferences_gps_tcp_port(void);
void conf_set_preferences_gps_tcp_port(gint);

gboolean conf_get_preferences_gps_show_altitude(void);
void conf_set_preferences_gps_show_altitude(gboolean);

gboolean conf_get_preferences_gps_show_errors(void);
void conf_set_preferences_gps_show_errors(gboolean);

const gchar* conf_get_preferences_rotator_hostname(void);
void conf_set_preferences_rotator_hostname(const gchar*);

gint conf_get_preferences_rotator_tcp_port(void);
void conf_set_preferences_rotator_tcp_port(gint);

const gchar* conf_get_preferences_rotator_password(void);
void conf_set_preferences_rotator_password(const gchar*);

gint conf_get_preferences_rotator_min_speed(void);
void conf_set_preferences_rotator_min_speed(gint);

gint conf_get_preferences_rotator_def_speed(void);
void conf_set_preferences_rotator_def_speed(gint);

gdouble conf_get_preferences_rotator_latitude(void);
void conf_set_preferences_rotator_latitude(gdouble);

gdouble conf_get_preferences_rotator_longitude(void);
void conf_set_preferences_rotator_longitude(gdouble);

gboolean conf_get_preferences_blacklist_enabled(void);
void conf_set_preferences_blacklist_enabled(gboolean);

gboolean conf_get_preferences_blacklist_inverted(void);
void conf_set_preferences_blacklist_inverted(gboolean);

gboolean conf_get_preferences_blacklist(gint64);
void conf_set_preferences_blacklist(gint64);
void conf_del_preferences_blacklist(gint64);

GtkListStore* conf_get_preferences_blacklist_as_liststore(void);
void conf_set_preferences_blacklist_from_liststore(GtkListStore*);

gboolean conf_get_preferences_highlightlist_enabled(void);
void conf_set_preferences_highlightlist_enabled(gboolean);

gboolean conf_get_preferences_highlightlist_inverted(void);
void conf_set_preferences_highlightlist_inverted(gboolean);

gboolean conf_get_preferences_highlightlist(gint64);
void conf_set_preferences_highlightlist(gint64);
void conf_del_preferences_highlightlist(gint64);

GtkListStore* conf_get_preferences_highlightlist_as_liststore(void);
void conf_set_preferences_highlightlist_from_liststore(GtkListStore*);

gboolean conf_get_preferences_alarmlist_enabled(void);
void conf_set_preferences_alarmlist_enabled(gboolean);

gboolean conf_get_preferences_alarmlist_inverted(void);
void conf_set_preferences_alarmlist_inverted(gboolean);

gboolean conf_get_preferences_alarmlist(gint64);
void conf_set_preferences_alarmlist(gint64);
void conf_del_preferences_alarmlist(gint64);

GtkListStore* conf_get_preferences_alarmlist_as_liststore(void);
void conf_set_preferences_alarmlist_from_liststore(GtkListStore*);

#endif
