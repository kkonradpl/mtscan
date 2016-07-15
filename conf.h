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
void conf_save();

/* Configuration [window] */
gint conf_get_window_x();
gint conf_get_window_y();
void conf_set_window_xy(gint, gint);

gint conf_get_window_width();
gint conf_get_window_height();
void conf_set_window_position(gint, gint);

/* Configuration [interface] */
gboolean conf_get_interface_sound();
void conf_set_interface_sound(gboolean);

gboolean conf_get_interface_dark_mode();
void conf_set_interface_dark_mode(gboolean);

gboolean conf_get_interface_gps();
void conf_set_interface_gps(gboolean);

gboolean conf_get_interface_signals();
void conf_set_interface_signals(gboolean signals);

gint conf_get_interface_last_profile();
void conf_set_interface_last_profile(gint);

/* Connection profiles */
GtkListStore* conf_get_profiles();

GtkTreeIter conf_profile_add(mtscan_profile_t*);
mtscan_profile_t conf_profile_get(GtkTreeIter*);
void conf_profile_free(mtscan_profile_t*);

#endif
