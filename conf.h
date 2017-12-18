#ifndef MTSCAN_CONF_H_
#define MTSCAN_CONF_H_
#include <gtk/gtk.h>

enum
{
    PROFILE_COL_NAME,
    PROFILE_COL_HOST,
    PROFILE_COL_PORT,
    PROFILE_COL_LOGIN,
    PROFILE_COL_PASSWORD,
    PROFILE_COL_INTERFACE,
    PROFILE_COL_DURATION_TIME,
    PROFILE_COL_DURATION,
    PROFILE_COL_REMOTE,
    PROFILE_COL_BACKGROUND,
    PROFILE_COLS
};

typedef struct mtscan_conf_profile
{
    gchar *name;
    gchar *host;
    gint port;
    gchar *login;
    gchar *password;
    gchar *iface;
    gint duration_time;
    gboolean duration;
    gboolean remote;
    gboolean background;
} mtscan_profile_t;

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
GtkListStore* conf_get_profiles(void);

GtkTreeIter conf_profile_add(mtscan_profile_t*);
mtscan_profile_t conf_profile_get(GtkTreeIter*);
void conf_profile_free(mtscan_profile_t*);

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

gboolean conf_get_preferences_blacklist(const gchar*);
void conf_set_preferences_blacklist(const gchar*);
void conf_del_preferences_blacklist(const gchar*);

GtkListStore* conf_get_preferences_blacklist_as_liststore(void);
void conf_set_preferences_blacklist_from_liststore(GtkListStore*);

gboolean conf_get_preferences_highlightlist_enabled(void);
void conf_set_preferences_highlightlist_enabled(gboolean);

gboolean conf_get_preferences_highlightlist_inverted(void);
void conf_set_preferences_highlightlist_inverted(gboolean);

gboolean conf_get_preferences_highlightlist(const gchar*);
void conf_set_preferences_highlightlist(const gchar*);
void conf_del_preferences_highlightlist(const gchar*);

GtkListStore* conf_get_preferences_highlightlist_as_liststore(void);
void conf_set_preferences_highlightlist_from_liststore(GtkListStore*);

#endif
