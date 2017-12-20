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

gboolean conf_get_interface_gps(void);
void conf_set_interface_gps(gboolean);

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

/* Configuration [preferences] */
gint conf_get_preferences_icon_size(void);
void conf_set_preferences_icon_size(gint);

gint conf_get_preferences_search_column(void);
void conf_set_preferences_search_column(gint);

gboolean conf_get_preferences_latlon_column(void);
void conf_set_preferences_latlon_column(gboolean);

gboolean conf_get_preferences_azimuth_column(void);
void conf_set_preferences_azimuth_column(gboolean);

gboolean conf_get_preferences_signals(void);
void conf_set_preferences_signals(gboolean);

const gchar* conf_get_preferences_gps_hostname(void);
void conf_set_preferences_gps_hostname(const gchar*);

gint conf_get_preferences_gps_tcp_port(void);
void conf_set_preferences_gps_tcp_port(gint);

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

#endif
