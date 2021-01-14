/*
 *  MTscan - MikroTik RouterOS wireless scanner
 *  Copyright (c) 2015-2020  Konrad Kosmatka
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

#include <string.h>
#include <glib/gstdio.h>
#include <math.h>
#include "ui-dialogs.h"
#include "conf.h"
#include "ui-view.h"
#include "misc.h"
#include "conf-scanlist.h"
#include "conf-extlist.h"

#define CONF_DIR  "mtscan"
#define CONF_FILE "mtscan.conf"

#define CONF_DEFAULT_WINDOW_X         (-1)
#define CONF_DEFAULT_WINDOW_Y         (-1)
#define CONF_DEFAULT_WINDOW_WIDTH     1000
#define CONF_DEFAULT_WINDOW_HEIGHT    500
#define CONF_DEFAULT_WINDOW_MAXIMIZED FALSE

#define CONF_DEFAULT_INTERFACE_SOUND         FALSE
#define CONF_DEFAULT_INTERFACE_DARK_MODE     FALSE
#define CONF_DEFAULT_INTERFACE_AUTOSAVE      FALSE
#define CONF_DEFAULT_INTERFACE_GPS           FALSE
#define CONF_DEFAULT_INTERFACE_GEOLOC        FALSE
#define CONF_DEFAULT_INTERFACE_ROTATOR       FALSE
#define CONF_DEFAULT_INTERFACE_LAST_PROFILE  (-1)

#define CONF_DEFAULT_PROFILE_NAME           "unnamed"
#define CONF_DEFAULT_PROFILE_HOST           ""
#define CONF_DEFAULT_PROFILE_PORT           22
#define CONF_DEFAULT_PROFILE_LOGIN          "admin"
#define CONF_DEFAULT_PROFILE_PASSWORD       ""
#define CONF_DEFAULT_PROFILE_INTERFACE      "wlan1"
#define CONF_DEFAULT_PROFILE_MODE           MTSCAN_CONF_PROFILE_MODE_SCANNER
#define CONF_DEFAULT_PROFILE_DURATION_TIME  10
#define CONF_DEFAULT_PROFILE_DURATION       FALSE
#define CONF_DEFAULT_PROFILE_REMOTE         FALSE
#define CONF_DEFAULT_PROFILE_BACKGROUND     FALSE

#define CONF_DEFAULT_SCANLIST_NAME          "unnamed"
#define CONF_DEFAULT_SCANLIST_DATA          "default"

#define CONF_DEFAULT_PATH_LOG_OPEN   ""
#define CONF_DEFAULT_PATH_LOG_SAVE   ""
#define CONF_DEFAULT_PATH_LOG_EXPORT ""
#define CONF_DEFAULT_PATH_AUTOSAVE   ""
#define CONF_DEFAULT_PATH_SCREENSHOT ""

#define CONF_DEFAULT_PREFERENCES_ICON_SIZE              18
#define CONF_DEFAULT_PREFERENCES_AUTOSAVE_INTERVAL      5
#define CONF_DEFAULT_PREFERENCES_SEARCH_COLUMN          1
#define CONF_DEFAULT_PREFERENCES_FALLBACK_ENCODING      "ISO-8859-2"
#define CONF_DEFAULT_PREFERENCES_NO_STYLE_OVERRIDE      FALSE
#define CONF_DEFAULT_PREFERENCES_SIGNALS                TRUE
#define CONF_DEFAULT_PREFERENCES_DISPLAY_TIME_ONLY      FALSE
#define CONF_DEFAULT_PREFERENCES_RECONNECT              FALSE
#define CONF_DEFAULT_PREFERENCES_SOUNDS_NEW_NETWORK     TRUE
#define CONF_DEFAULT_PREFERENCES_SOUNDS_NEW_NETWORK_HI  TRUE
#define CONF_DEFAULT_PREFERENCES_SOUNDS_NEW_NETWORK_AL  TRUE
#define CONF_DEFAULT_PREFERENCES_SOUNDS_NO_DATA         TRUE
#define CONF_DEFAULT_PREFERENCES_SOUNDS_NO_GPS_DATA     TRUE
#define CONF_DEFAULT_PREFERENCES_EVENTS_NEW_NETWORK     FALSE
#define CONF_DEFAULT_PREFERENCES_TZSP_UDP_PORT          0x9090
#define CONF_DEFAULT_PREFERENCES_TZSP_CHANNEL_WIDTH     20
#define CONF_DEFAULT_PREFERENCES_TZSP_BAND              MTSCAN_CONF_TZSP_BAND_5GHZ
#define CONF_DEFAULT_PREFERENCES_GPS_HOSTNAME           "localhost"
#define CONF_DEFAULT_PREFERENCES_GPS_TCP_PORT           2947
#define CONF_DEFAULT_PREFERENCES_GPS_SHOW_ALTITUDE      TRUE
#define CONF_DEFAULT_PREFERENCES_GPS_SHOW_ERRORS        FALSE
#define CONF_DEFAULT_PREFERENCES_ROTATOR_HOSTNAME       "localhost"
#define CONF_DEFAULT_PREFERENCES_ROTATOR_TCP_PORT       7399
#define CONF_DEFAULT_PREFERENCES_ROTATOR_PASSWORD       ""
#define CONF_DEFAULT_PREFERENCES_ROTATOR_MIN_SPEED      25
#define CONF_DEFAULT_PREFERENCES_ROTATOR_DEF_SPEED      100
#define CONF_DEFAULT_PREFERENCES_BLACKLIST_ENABLED      FALSE
#define CONF_DEFAULT_PREFERENCES_BLACKLIST_INVERTED     FALSE
#define CONF_DEFAULT_PREFERENCES_BLACKLIST_EXTERNAL     FALSE
#define CONF_DEFAULT_PREFERENCES_HIGHLIGHTLIST_ENABLED  FALSE
#define CONF_DEFAULT_PREFERENCES_HIGHLIGHTLIST_INVERTED FALSE
#define CONF_DEFAULT_PREFERENCES_HIGHLIGHTLIST_EXTERNAL FALSE
#define CONF_DEFAULT_PREFERENCES_ALARMLIST_ENABLED      FALSE
#define CONF_DEFAULT_PREFERENCES_ALARMLIST_EXTERNAL     FALSE
#define CONF_DEFAULT_PREFERENCES_LOCATION_LATITUDE      NAN
#define CONF_DEFAULT_PREFERENCES_LOCATION_LONGITUDE     NAN
#define CONF_DEFAULT_PREFERENCES_LOCATION_MTSCAN        FALSE
#define CONF_DEFAULT_PREFERENCES_LOCATION_WIGLE         FALSE
#define CONF_DEFAULT_PREFERENCES_LOCATION_WIGLE_API_URL "https://api.wigle.net/api/v2/network/detail?netid="
#define CONF_DEFAULT_PREFERENCES_LOCATION_WIGLE_API_KEY ""
#define CONF_DEFAULT_PREFERENCES_LOCATION_AZIMUTH_ERROR 5
#define CONF_DEFAULT_PREFERENCES_LOCATION_MIN_DISTANCE  0
#define CONF_DEFAULT_PREFERENCES_LOCATION_MAX_DISTANCE  999


#define CONF_PROFILE_GROUP_PREFIX  "profile_"
#define CONF_PROFILE_GROUP_DEFAULT "profile"
#define CONF_SCANLIST_GROUP_PREFIX "scanlist_"

typedef struct conf
{
    gchar    *path;
    GKeyFile *keyfile;

    /* [window] */
    gint     window_x;
    gint     window_y;
    gint     window_width;
    gint     window_height;
    gboolean window_maximized;

    /* [interface] */
    gboolean interface_sound;
    gboolean interface_dark_mode;
    gboolean interface_autosave;
    gboolean interface_gps;
    gboolean interface_geoloc;
    gboolean interface_rotator;
    gint     interface_last_profile;

    /* [profile] */
    conf_profile_t *profile_default;

    /* [profile_x] */
    GtkListStore *profiles;

    /* [scanlist_x] */
    GtkListStore *scanlists;

    /* [path] */
    gchar *path_log_open;
    gchar *path_log_save;
    gchar *path_log_export;
    gchar *path_autosave;
    gchar *path_screenshot;

    /* [preferences] */
    gint      preferences_icon_size;
    gint      preferences_autosave_interval;
    gint      preferences_search_column;
    gchar    *preferences_fallback_encoding;
    gboolean  preferences_no_style_override;
    gboolean  preferences_signals;
    gboolean  preferences_display_time_only;
    gboolean  preferences_reconnect;

    gchar   **preferences_view_cols_order;
    gchar   **preferences_view_cols_hidden;

    gboolean  preferences_sounds_new_network;
    gboolean  preferences_sounds_new_network_hi;
    gboolean  preferences_sounds_new_network_al;
    gboolean  preferences_sounds_no_data;
    gboolean  preferences_sounds_no_gps_data;

    gboolean  preferences_events_new_network;
    gchar    *preferences_events_new_network_exec;

    gint                     preferences_tzsp_udp_port;
    gint                     preferences_tzsp_channel_width;
    mtscan_conf_tzsp_band_t  preferences_tzsp_band;

    gchar    *preferences_gps_hostname;
    gint      preferences_gps_tcp_port;
    gboolean  preferences_gps_show_altitude;
    gboolean  preferences_gps_show_errors;

    gchar    *preferences_rotator_hostname;
    gint      preferences_rotator_tcp_port;
    gchar    *preferences_rotator_password;
    gint      preferences_rotator_min_speed;
    gint      preferences_rotator_def_speed;

    gboolean  preferences_blacklist_enabled;
    gboolean  preferences_blacklist_inverted;
    gboolean  preferences_blacklist_external;
    gchar    *preferences_blacklist_ext_path;
    GTree    *blacklist;
    GTree    *blacklist_ext;

    gboolean  preferences_highlightlist_enabled;
    gboolean  preferences_highlightlist_inverted;
    gboolean  preferences_highlightlist_external;
    gchar    *preferences_highlightlist_ext_path;
    GTree    *highlightlist;
    GTree    *highlightlist_ext;

    gboolean  preferences_alarmlist_enabled;
    gboolean  preferences_alarmlist_external;
    gchar    *preferences_alarmlist_ext_path;
    GTree    *alarmlist;
    GTree    *alarmlist_ext;

    gdouble    preferences_location_latitude;
    gdouble    preferences_location_longitude;
    gboolean   preferences_location_mtscan;
    gchar    **preferences_location_mtscan_data;
    gboolean   preferences_location_wigle;
    gchar     *preferences_location_wigle_api_url;
    gchar     *preferences_location_wigle_api_key;
    gint       preferences_location_azimuth_error;
    gint       preferences_location_min_distance;
    gint       preferences_location_max_distance;
} conf_t;

static conf_t conf;

static void             conf_read(void);
static gboolean         conf_read_boolean(const gchar*, const gchar*, gboolean);
static gint             conf_read_integer(const gchar*, const gchar*, gint);
static gdouble          conf_read_double(const gchar*, const gchar*, gdouble);
static gchar*           conf_read_string(const gchar*, const gchar*, const gchar*);
static gchar**          conf_read_string_list(GKeyFile*, const gchar*, const gchar*, const gchar* const*);
static gchar**          conf_read_columns(GKeyFile*, const gchar*, const gchar*);
static void             conf_read_gint64_tree(GKeyFile*, const gchar*, const gchar*, GTree*);

static void             conf_read_list(GtkListStore*, const gchar*, void (*)(GtkListStore*, const gchar*));
static void             conf_read_profile_callback(GtkListStore*, const gchar*);
static conf_profile_t*  conf_read_profile(const gchar*);
static void             conf_read_scanlist_callback(GtkListStore*, const gchar*);
static conf_scanlist_t* conf_read_scanlist(const gchar*);

static void             conf_save_list(GtkListStore*, GtkTreeModelForeachFunc);
static gboolean         conf_save_profiles_foreach(GtkTreeModel*, GtkTreePath*, GtkTreeIter*, gpointer);
static void             conf_save_profile(GKeyFile*, const gchar*, const conf_profile_t*);
static gboolean         conf_save_scanlists_foreach(GtkTreeModel*, GtkTreePath*, GtkTreeIter*, gpointer);
static void             conf_save_scanlist(GKeyFile*, const gchar*, const conf_scanlist_t*);

static void             conf_save_gint64_tree(GKeyFile*, const gchar*, const gchar*, GTree*);
static gboolean         conf_save_gint64_tree_foreach(gpointer, gpointer, gpointer);

static void             conf_change_string(gchar**, const gchar*);

void
conf_init(const gchar *custom_path)
{
    gchar *directory;
    if(custom_path)
        conf.path = g_strdup(custom_path);
    else
    {
        directory = g_build_filename(g_get_user_config_dir(), CONF_DIR, NULL);
        g_mkdir(directory, 0700);
        conf.path = g_build_filename(directory, CONF_FILE, NULL);
        g_free(directory);
    }

    conf.keyfile = g_key_file_new();
    conf.blacklist = g_tree_new_full((GCompareDataFunc)gint64cmp, NULL, g_free, NULL);
    conf.blacklist_ext = g_tree_new_full((GCompareDataFunc)gint64cmp, NULL, g_free, NULL);

    conf.highlightlist = g_tree_new_full((GCompareDataFunc)gint64cmp, NULL, g_free, NULL);
    conf.highlightlist_ext = g_tree_new_full((GCompareDataFunc)gint64cmp, NULL, g_free, NULL);

    conf.alarmlist = g_tree_new_full((GCompareDataFunc)gint64cmp, NULL, g_free, NULL);
    conf.alarmlist_ext = g_tree_new_full((GCompareDataFunc)gint64cmp, NULL, g_free, NULL);

    conf_read();

    if(conf_get_preferences_blacklist_external())
        conf_extlist_load(conf.blacklist_ext, conf_get_preferences_blacklist_ext_path());
    if(conf_get_preferences_highlightlist_external())
        conf_extlist_load(conf.highlightlist_ext, conf_get_preferences_highlightlist_ext_path());
    if(conf_get_preferences_alarmlist_external())
        conf_extlist_load(conf.alarmlist_ext, conf_get_preferences_alarmlist_ext_path());
}

static void
conf_read(void)
{
    gboolean file_exists = TRUE;

    if(!g_key_file_load_from_file(conf.keyfile, conf.path, G_KEY_FILE_KEEP_COMMENTS, NULL))
    {
        file_exists = FALSE;
        ui_dialog(NULL,
                  GTK_MESSAGE_INFO,
                  "Configuration",
                  "Unable to read the configuration.\n\nUsing default settings.");
    }

    conf.window_x = conf_read_integer("window", "x", CONF_DEFAULT_WINDOW_X);
    conf.window_y = conf_read_integer("window", "y", CONF_DEFAULT_WINDOW_Y);
    conf.window_width = conf_read_integer("window", "width", CONF_DEFAULT_WINDOW_WIDTH);
    conf.window_height = conf_read_integer("window", "height", CONF_DEFAULT_WINDOW_HEIGHT);
    conf.window_maximized = conf_read_boolean("window", "maximized", CONF_DEFAULT_WINDOW_MAXIMIZED);

    conf.interface_sound = conf_read_boolean("interface", "sound", CONF_DEFAULT_INTERFACE_SOUND);
    conf.interface_dark_mode = conf_read_boolean("interface", "dark_mode", CONF_DEFAULT_INTERFACE_DARK_MODE);
    conf.interface_autosave = conf_read_boolean("interface", "autosave", CONF_DEFAULT_INTERFACE_AUTOSAVE);
    conf.interface_gps = conf_read_boolean("interface", "gps", CONF_DEFAULT_INTERFACE_GPS);
    conf.interface_geoloc = conf_read_boolean("interface", "geoloc", CONF_DEFAULT_INTERFACE_GEOLOC);
    conf.interface_rotator = conf_read_boolean("interface", "rotator", CONF_DEFAULT_INTERFACE_ROTATOR);
    conf.interface_last_profile = conf_read_integer("interface", "last_profile", CONF_DEFAULT_INTERFACE_LAST_PROFILE);

    conf.profile_default = conf_read_profile(CONF_PROFILE_GROUP_DEFAULT);
    conf.profiles = conf_profile_list_new();
    conf_read_list(conf.profiles, CONF_PROFILE_GROUP_PREFIX, conf_read_profile_callback);

    conf.scanlists = conf_scanlist_list_new();
    conf_read_list(conf.scanlists, CONF_SCANLIST_GROUP_PREFIX, conf_read_scanlist_callback);

    conf.path_log_open = conf_read_string("path", "log_open", CONF_DEFAULT_PATH_LOG_OPEN);
    conf.path_log_save = conf_read_string("path", "log_save", CONF_DEFAULT_PATH_LOG_SAVE);
    conf.path_log_export = conf_read_string("path", "log_export", CONF_DEFAULT_PATH_LOG_EXPORT);
    conf.path_autosave = conf_read_string("path", "autosave", CONF_DEFAULT_PATH_AUTOSAVE);
    conf.path_screenshot = conf_read_string("path", "screenshot", CONF_DEFAULT_PATH_SCREENSHOT);

    conf.preferences_icon_size = conf_read_integer("preferences", "icon_size", CONF_DEFAULT_PREFERENCES_ICON_SIZE);
    conf.preferences_autosave_interval = conf_read_integer("preferences", "autosave_interval", CONF_DEFAULT_PREFERENCES_AUTOSAVE_INTERVAL);
    conf.preferences_search_column = conf_read_integer("preferences", "search_column", CONF_DEFAULT_PREFERENCES_SEARCH_COLUMN);
    conf.preferences_fallback_encoding = conf_read_string("preferences", "fallback_encoding", CONF_DEFAULT_PREFERENCES_FALLBACK_ENCODING);
    conf.preferences_no_style_override = conf_read_boolean("preferences", "no_style_override", CONF_DEFAULT_PREFERENCES_NO_STYLE_OVERRIDE);
    conf.preferences_signals = conf_read_boolean("preferences", "signals", CONF_DEFAULT_PREFERENCES_SIGNALS);
    conf.preferences_display_time_only = conf_read_boolean("preferences", "display_time_only", CONF_DEFAULT_PREFERENCES_DISPLAY_TIME_ONLY);
    conf.preferences_reconnect = conf_read_boolean("preferences", "reconnect", CONF_DEFAULT_PREFERENCES_RECONNECT);

    conf.preferences_view_cols_order = conf_read_columns(conf.keyfile, "preferences", "view_cols_order");
    conf.preferences_view_cols_hidden = conf_read_string_list(conf.keyfile, "preferences", "view_cols_hidden", NULL);

    conf.preferences_sounds_new_network = conf_read_boolean("preferences", "sounds_new_network", CONF_DEFAULT_PREFERENCES_SOUNDS_NEW_NETWORK);
    conf.preferences_sounds_new_network_hi = conf_read_boolean("preferences", "sounds_new_network_hi", CONF_DEFAULT_PREFERENCES_SOUNDS_NEW_NETWORK_HI);
    conf.preferences_sounds_new_network_al = conf_read_boolean("preferences", "sounds_new_network_al", CONF_DEFAULT_PREFERENCES_SOUNDS_NEW_NETWORK_AL);
    conf.preferences_sounds_no_data = conf_read_boolean("preferences", "sounds_no_data", CONF_DEFAULT_PREFERENCES_SOUNDS_NO_DATA);
    conf.preferences_sounds_no_gps_data = conf_read_boolean("preferences", "sounds_no_gps_data", CONF_DEFAULT_PREFERENCES_SOUNDS_NO_GPS_DATA);

    conf.preferences_events_new_network = conf_read_boolean("preferences", "events_new_network", CONF_DEFAULT_PREFERENCES_EVENTS_NEW_NETWORK);
    conf.preferences_events_new_network_exec = conf_read_string("preferences", "events_new_network_exec", "");

    conf.preferences_tzsp_udp_port = conf_read_integer("preferences", "tzsp_udp_port", CONF_DEFAULT_PREFERENCES_TZSP_UDP_PORT);
    conf.preferences_tzsp_channel_width = conf_read_integer("preferences", "tzsp_channel_width", CONF_DEFAULT_PREFERENCES_TZSP_CHANNEL_WIDTH);
    conf.preferences_tzsp_band = (mtscan_conf_tzsp_band_t)conf_read_integer("preferences", "tzsp_band", CONF_DEFAULT_PREFERENCES_TZSP_BAND);

    conf.preferences_gps_hostname = conf_read_string("preferences", "gps_hostname", CONF_DEFAULT_PREFERENCES_GPS_HOSTNAME);
    conf.preferences_gps_tcp_port = conf_read_integer("preferences", "gps_tcp_port", CONF_DEFAULT_PREFERENCES_GPS_TCP_PORT);
    conf.preferences_gps_show_altitude = conf_read_boolean("preferences", "gps_show_altitude", CONF_DEFAULT_PREFERENCES_GPS_SHOW_ALTITUDE);
    conf.preferences_gps_show_errors = conf_read_boolean("preferences", "gps_show_errors", CONF_DEFAULT_PREFERENCES_GPS_SHOW_ERRORS);

    conf.preferences_rotator_hostname = conf_read_string("preferences", "rotator_hostname", CONF_DEFAULT_PREFERENCES_ROTATOR_HOSTNAME);
    conf.preferences_rotator_tcp_port = conf_read_integer("preferences", "rotator_tcp_port", CONF_DEFAULT_PREFERENCES_ROTATOR_TCP_PORT);
    conf.preferences_rotator_password = conf_read_string("preferences", "rotator_password", CONF_DEFAULT_PREFERENCES_ROTATOR_PASSWORD);
    conf.preferences_rotator_min_speed = conf_read_integer("preferences", "rotator_min_speed", CONF_DEFAULT_PREFERENCES_ROTATOR_MIN_SPEED);
    conf.preferences_rotator_def_speed = conf_read_integer("preferences", "rotator_def_speed", CONF_DEFAULT_PREFERENCES_ROTATOR_DEF_SPEED);

    conf.preferences_blacklist_enabled = conf_read_boolean("preferences", "blacklist_enabled", CONF_DEFAULT_PREFERENCES_BLACKLIST_ENABLED);
    conf.preferences_blacklist_inverted = conf_read_boolean("preferences", "blacklist_inverted", CONF_DEFAULT_PREFERENCES_BLACKLIST_INVERTED);
    conf.preferences_blacklist_external = conf_read_boolean("preferences", "blacklist_external", CONF_DEFAULT_PREFERENCES_BLACKLIST_EXTERNAL);
    conf.preferences_blacklist_ext_path = conf_read_string("preferences", "blacklist_ext_path", "");
    conf_read_gint64_tree(conf.keyfile, "preferences", "blacklist", conf.blacklist);

    conf.preferences_highlightlist_enabled = conf_read_boolean("preferences", "highlightlist_enabled", CONF_DEFAULT_PREFERENCES_HIGHLIGHTLIST_ENABLED);
    conf.preferences_highlightlist_inverted = conf_read_boolean("preferences", "highlightlist_inverted", CONF_DEFAULT_PREFERENCES_HIGHLIGHTLIST_INVERTED);
    conf.preferences_highlightlist_external = conf_read_boolean("preferences", "highlightlist_external", CONF_DEFAULT_PREFERENCES_HIGHLIGHTLIST_EXTERNAL);
    conf.preferences_highlightlist_ext_path = conf_read_string("preferences", "highlightlist_ext_path", "");
    conf_read_gint64_tree(conf.keyfile, "preferences", "highlightlist", conf.highlightlist);

    conf.preferences_alarmlist_enabled = conf_read_boolean("preferences", "alarmlist_enabled", CONF_DEFAULT_PREFERENCES_ALARMLIST_ENABLED);
    conf.preferences_alarmlist_external = conf_read_boolean("preferences", "alarmlist_external", CONF_DEFAULT_PREFERENCES_ALARMLIST_EXTERNAL);
    conf.preferences_alarmlist_ext_path = conf_read_string("preferences", "alarmlist_ext_path", "");
    conf_read_gint64_tree(conf.keyfile, "preferences", "alarmlist", conf.alarmlist);

    conf.preferences_location_latitude = conf_read_double("preferences", "location_latitude", CONF_DEFAULT_PREFERENCES_LOCATION_LATITUDE);
    conf.preferences_location_longitude = conf_read_double("preferences", "location_longitude", CONF_DEFAULT_PREFERENCES_LOCATION_LONGITUDE);
    conf.preferences_location_mtscan = conf_read_boolean("preferences", "location_mtscan", CONF_DEFAULT_PREFERENCES_LOCATION_MTSCAN);
    conf.preferences_location_mtscan_data = conf_read_string_list(conf.keyfile, "preferences", "location_mtscan_data", NULL);
    conf.preferences_location_wigle = conf_read_boolean("preferences", "location_wigle", CONF_DEFAULT_PREFERENCES_LOCATION_WIGLE);
    conf.preferences_location_wigle_api_url = conf_read_string("preferences", "location_wigle_api_url", CONF_DEFAULT_PREFERENCES_LOCATION_WIGLE_API_URL);
    conf.preferences_location_wigle_api_key = conf_read_string("preferences", "location_wigle_api_key", CONF_DEFAULT_PREFERENCES_LOCATION_WIGLE_API_KEY);
    conf.preferences_location_azimuth_error = conf_read_integer("preferences", "location_azimuth_error", CONF_DEFAULT_PREFERENCES_LOCATION_AZIMUTH_ERROR);
    conf.preferences_location_min_distance = conf_read_integer("preferences", "location_min_distance", CONF_DEFAULT_PREFERENCES_LOCATION_MIN_DISTANCE);
    conf.preferences_location_max_distance = conf_read_integer("preferences", "location_max_distance", CONF_DEFAULT_PREFERENCES_LOCATION_MAX_DISTANCE);

    if(!file_exists)
        conf_save();
}

static gboolean
conf_read_boolean(const gchar *group_name,
                  const gchar *key,
                  gboolean     default_value)
{
    gboolean value;
    GError *err = NULL;

    value = g_key_file_get_boolean(conf.keyfile, group_name, key, &err);
    if(err)
    {
        value = default_value;
        g_error_free(err);
    }
    return value;
}

static gint
conf_read_integer(const gchar *group_name,
                  const gchar *key,
                  gint         default_value)
{
    gint value;
    GError *err = NULL;

    value = g_key_file_get_integer(conf.keyfile, group_name, key, &err);
    if(err)
    {
        value = default_value;
        g_error_free(err);
    }
    return value;
}

static gdouble
conf_read_double(const gchar *group_name,
                 const gchar *key,
                 gdouble      default_value)
{
    gdouble value;
    GError *err = NULL;

    value = g_key_file_get_double(conf.keyfile, group_name, key, &err);
    if(err)
    {
        value = default_value;
        g_error_free(err);
    }
    return value;
}

static gchar*
conf_read_string(const gchar *group_name,
                 const gchar *key,
                 const gchar *default_value)
{
    gchar *value;
    GError *err = NULL;

    value = g_key_file_get_string(conf.keyfile, group_name, key, &err);
    if(err)
    {
        value = g_strdup(default_value);
        g_error_free(err);
    }
    return value;
}

static gchar**
conf_read_string_list(GKeyFile           *keyfile,
                      const gchar        *group_name,
                      const gchar        *key,
                      const gchar *const *default_value)
{
    gchar **values;
    gsize length;

    values = g_key_file_get_string_list(keyfile, group_name, key, &length, NULL);
    if(!values)
    {
        if(!default_value)
            values = g_malloc0(sizeof(gchar*));
        else
            values = g_strdupv((gchar**)default_value);
    }
    return values;
}

static gchar**
conf_read_columns(GKeyFile    *keyfile,
                  const gchar *group_name,
                  const gchar *key)
{
    gchar **values;
    gchar **new_values;
    const gchar *name;
    gsize len;
    guint i, j, missing;

    values = g_key_file_get_string_list(keyfile, group_name, key, &len, NULL);
    if(!values)
    {
        /* Create default order */
        values = g_new(gchar*, MTSCAN_VIEW_COLS+1);
        for(i=0; i<MTSCAN_VIEW_COLS; i++)
            values[i] = g_strdup(ui_view_get_column_name(i));
        values[i] = NULL;
    }

    /* Make sure that all columns are included */
    missing = 0;
    for(i=0; i<MTSCAN_VIEW_COLS; i++)
        if(!g_strv_contains((const gchar* const*)values, ui_view_get_column_name(i)))
            missing++;

    if(!missing)
        return values;

    len = g_strv_length(values);
    new_values = g_new(gchar*, len+missing+1);

    for(i=0; i<len; i++)
        new_values[i] = g_strdup(values[i]);

    for(j=0; j<MTSCAN_VIEW_COLS; j++)
    {
        name = ui_view_get_column_name(j);
        if(!g_strv_contains((const gchar * const *)values, ui_view_get_column_name(j)))
            new_values[i++] = g_strdup(name);
    }

    g_strfreev(values);
    new_values[i] = NULL;
    return new_values;
}

static void
conf_read_list(GtkListStore  *model,
               const gchar   *prefix,
               void         (*callback)(GtkListStore*, const gchar*))
{
    gchar **groups = g_key_file_get_groups(conf.keyfile, NULL);
    gchar **group;

    for(group=groups; *group; ++group)
    {
        if(strncmp(*group, prefix, strlen(prefix)) == 0)
        {
            callback(model, *group);
            g_key_file_remove_group(conf.keyfile, *group, NULL);
        }
    }

    g_strfreev(groups);
}


static void
conf_read_profile_callback(GtkListStore *model,
                           const gchar  *group_name)
{
    conf_profile_t *p = conf_read_profile(group_name);
    conf_profile_list_add(model, p);
    conf_profile_free(p);
}

static conf_profile_t*
conf_read_profile(const gchar *group_name)
{
    conf_profile_t *p;
    p = conf_profile_new(conf_read_string(group_name, "name", CONF_DEFAULT_PROFILE_NAME),
                         conf_read_string(group_name, "host", CONF_DEFAULT_PROFILE_HOST),
                         conf_read_integer(group_name, "port", CONF_DEFAULT_PROFILE_PORT),
                         conf_read_string(group_name, "login", CONF_DEFAULT_PROFILE_LOGIN),
                         conf_read_string(group_name, "password", CONF_DEFAULT_PROFILE_PASSWORD),
                         conf_read_string(group_name, "interface", CONF_DEFAULT_PROFILE_INTERFACE),
                         (mtscan_conf_profile_mode_t)conf_read_integer(group_name, "mode", CONF_DEFAULT_PROFILE_MODE),
                         conf_read_integer(group_name, "duration_time", CONF_DEFAULT_PROFILE_DURATION_TIME),
                         conf_read_boolean(group_name, "duration", CONF_DEFAULT_PROFILE_DURATION),
                         conf_read_boolean(group_name, "remote", CONF_DEFAULT_PROFILE_REMOTE),
                         conf_read_boolean(group_name, "background", CONF_DEFAULT_PROFILE_BACKGROUND));
    return p;
}

static void
conf_read_scanlist_callback(GtkListStore *model,
                            const gchar  *group_name)
{
    conf_scanlist_t *sl = conf_read_scanlist(group_name);
    conf_scanlist_list_add(model, sl);
    conf_scanlist_free(sl);
}

static conf_scanlist_t*
conf_read_scanlist(const gchar *group_name)
{
    conf_scanlist_t *sl;
    sl = conf_scanlist_new(conf_read_string(group_name, "name", CONF_DEFAULT_SCANLIST_NAME),
                           conf_read_string(group_name, "data", CONF_DEFAULT_SCANLIST_DATA),
                           conf_read_boolean(group_name, "main", FALSE),
                           conf_read_boolean(group_name, "default", FALSE));

    return sl;
}

static void
conf_read_gint64_tree(GKeyFile    *keyfile,
                      const gchar *group_name,
                      const gchar *key,
                      GTree       *tree)
{
    gchar **values;
    gchar **it;
    gsize length;
    gint64 addr;

    values = g_key_file_get_string_list(keyfile, group_name, key, &length, NULL);
    if(values)
    {
        for(it = values; *it; it++)
        {
            if(((addr = str_addr_to_gint64(*it, strlen(*it))) >= 0))
                g_tree_insert(tree, gint64dup(&addr), GINT_TO_POINTER(TRUE));
        }
        g_strfreev(values);
    }

}

void
conf_save(void)
{
    GError *err = NULL;
    gchar *configuration;
    gsize length, out;
    FILE *fp;

    g_key_file_set_integer(conf.keyfile, "window", "x", conf.window_x);
    g_key_file_set_integer(conf.keyfile, "window", "y", conf.window_y);
    g_key_file_set_integer(conf.keyfile, "window", "width", conf.window_width);
    g_key_file_set_integer(conf.keyfile, "window", "height", conf.window_height);
    g_key_file_set_boolean(conf.keyfile, "window", "maximized", conf.window_maximized);

    g_key_file_set_boolean(conf.keyfile, "interface", "sound", conf.interface_sound);
    g_key_file_set_boolean(conf.keyfile, "interface", "dark_mode", conf.interface_dark_mode);
    g_key_file_set_boolean(conf.keyfile, "interface", "autosave", conf.interface_autosave);
    g_key_file_set_boolean(conf.keyfile, "interface", "gps", conf.interface_gps);
    g_key_file_set_boolean(conf.keyfile, "interface", "geoloc", conf.interface_geoloc);
    g_key_file_set_boolean(conf.keyfile, "interface", "rotator", conf.interface_rotator);
    g_key_file_set_integer(conf.keyfile, "interface", "last_profile", conf.interface_last_profile);

    conf_save_profile(conf.keyfile, CONF_PROFILE_GROUP_DEFAULT, conf.profile_default);
    conf_save_list(conf.profiles, conf_save_profiles_foreach);
    conf_save_list(conf.scanlists, conf_save_scanlists_foreach);

    g_key_file_set_string(conf.keyfile, "path", "log_open", conf.path_log_open);
    g_key_file_set_string(conf.keyfile, "path", "log_save", conf.path_log_save);
    g_key_file_set_string(conf.keyfile, "path", "log_export", conf.path_log_export);
    g_key_file_set_string(conf.keyfile, "path", "autosave", conf.path_autosave);
    g_key_file_set_string(conf.keyfile, "path", "screenshot", conf.path_screenshot);

    g_key_file_set_integer(conf.keyfile, "preferences", "icon_size", conf.preferences_icon_size);
    g_key_file_set_integer(conf.keyfile, "preferences", "autosave_interval", conf.preferences_autosave_interval);
    g_key_file_set_integer(conf.keyfile, "preferences", "search_column", conf.preferences_search_column);
    g_key_file_set_string(conf.keyfile, "preferences", "fallback_encoding", conf.preferences_fallback_encoding);
    g_key_file_set_boolean(conf.keyfile, "preferences", "no_style_override", conf.preferences_no_style_override);
    g_key_file_set_boolean(conf.keyfile, "preferences", "signals", conf.preferences_signals);
    g_key_file_set_boolean(conf.keyfile, "preferences", "display_time_only", conf.preferences_display_time_only);
    g_key_file_set_boolean(conf.keyfile, "preferences", "reconnect", conf.preferences_reconnect);

    g_key_file_set_string_list(conf.keyfile, "preferences", "view_cols_order",
                               (const gchar * const *)conf.preferences_view_cols_order, g_strv_length(conf.preferences_view_cols_order));
    g_key_file_set_string_list(conf.keyfile, "preferences", "view_cols_hidden",
                               (const gchar * const *)conf.preferences_view_cols_hidden, g_strv_length(conf.preferences_view_cols_hidden));

    g_key_file_set_boolean(conf.keyfile, "preferences", "sounds_new_network", conf.preferences_sounds_new_network);
    g_key_file_set_boolean(conf.keyfile, "preferences", "sounds_new_network_hi", conf.preferences_sounds_new_network_hi);
    g_key_file_set_boolean(conf.keyfile, "preferences", "sounds_new_network_al", conf.preferences_sounds_new_network_al);
    g_key_file_set_boolean(conf.keyfile, "preferences", "sounds_no_data", conf.preferences_sounds_no_data);
    g_key_file_set_boolean(conf.keyfile, "preferences", "sounds_no_gps_data", conf.preferences_sounds_no_gps_data);

    g_key_file_set_boolean(conf.keyfile, "preferences", "events_new_network", conf.preferences_events_new_network);
    g_key_file_set_string(conf.keyfile, "preferences", "events_new_network_exec", conf.preferences_events_new_network_exec);

    g_key_file_set_integer(conf.keyfile, "preferences", "tzsp_udp_port", conf.preferences_tzsp_udp_port);
    g_key_file_set_integer(conf.keyfile, "preferences", "tzsp_channel_width", conf.preferences_tzsp_channel_width);
    g_key_file_set_integer(conf.keyfile, "preferences", "tzsp_band", conf.preferences_tzsp_band);

    g_key_file_set_string(conf.keyfile, "preferences", "gps_hostname", conf.preferences_gps_hostname);
    g_key_file_set_integer(conf.keyfile, "preferences", "gps_tcp_port", conf.preferences_gps_tcp_port);
    g_key_file_set_boolean(conf.keyfile, "preferences", "gps_show_altitude", conf.preferences_gps_show_altitude);
    g_key_file_set_boolean(conf.keyfile, "preferences", "gps_show_errors", conf.preferences_gps_show_errors);

    g_key_file_set_string(conf.keyfile, "preferences", "rotator_hostname", conf.preferences_rotator_hostname);
    g_key_file_set_integer(conf.keyfile, "preferences", "rotator_tcp_port", conf.preferences_rotator_tcp_port);
    g_key_file_set_string(conf.keyfile, "preferences", "rotator_password", conf.preferences_rotator_password);
    g_key_file_set_integer(conf.keyfile, "preferences", "rotator_min_speed", conf.preferences_rotator_min_speed);
    g_key_file_set_integer(conf.keyfile, "preferences", "rotator_def_speed", conf.preferences_rotator_def_speed);

    g_key_file_set_boolean(conf.keyfile, "preferences", "blacklist_enabled", conf.preferences_blacklist_enabled);
    g_key_file_set_boolean(conf.keyfile, "preferences", "blacklist_inverted", conf.preferences_blacklist_inverted);
    g_key_file_set_boolean(conf.keyfile, "preferences", "blacklist_external", conf.preferences_blacklist_external);
    g_key_file_set_string(conf.keyfile, "preferences", "blacklist_ext_path", conf.preferences_blacklist_ext_path);
    conf_save_gint64_tree(conf.keyfile, "preferences", "blacklist", conf.blacklist);

    g_key_file_set_boolean(conf.keyfile, "preferences", "highlightlist_enabled", conf.preferences_highlightlist_enabled);
    g_key_file_set_boolean(conf.keyfile, "preferences", "highlightlist_inverted", conf.preferences_highlightlist_inverted);
    g_key_file_set_boolean(conf.keyfile, "preferences", "highlightlist_external", conf.preferences_highlightlist_external);
    g_key_file_set_string(conf.keyfile, "preferences", "highlightlist_ext_path", conf.preferences_highlightlist_ext_path);
    conf_save_gint64_tree(conf.keyfile, "preferences", "highlightlist", conf.highlightlist);

    g_key_file_set_boolean(conf.keyfile, "preferences", "alarmlist_enabled", conf.preferences_alarmlist_enabled);
    g_key_file_set_boolean(conf.keyfile, "preferences", "alarmlist_external", conf.preferences_alarmlist_external);
    g_key_file_set_string(conf.keyfile, "preferences", "alarmlist_ext_path", conf.preferences_alarmlist_ext_path);
    conf_save_gint64_tree(conf.keyfile, "preferences", "alarmlist", conf.alarmlist);

    g_key_file_set_double(conf.keyfile, "preferences", "location_latitude", conf.preferences_location_latitude);
    g_key_file_set_double(conf.keyfile, "preferences", "location_longitude", conf.preferences_location_longitude);
    g_key_file_set_boolean(conf.keyfile, "preferences", "location_mtscan", conf.preferences_location_mtscan);
    g_key_file_set_string_list(conf.keyfile, "preferences", "location_mtscan_data",
                               (const gchar * const *)conf.preferences_location_mtscan_data,
                               g_strv_length(conf.preferences_location_mtscan_data));
    g_key_file_set_boolean(conf.keyfile, "preferences", "location_wigle", conf.preferences_location_wigle);
    g_key_file_set_string(conf.keyfile, "preferences", "location_wigle_api_url", conf.preferences_location_wigle_api_url);
    g_key_file_set_string(conf.keyfile, "preferences", "location_wigle_api_key", conf.preferences_location_wigle_api_key);
    g_key_file_set_integer(conf.keyfile, "preferences", "location_azimuth_error", conf.preferences_location_azimuth_error);
    g_key_file_set_integer(conf.keyfile, "preferences", "location_min_distance", conf.preferences_location_min_distance);
    g_key_file_set_integer(conf.keyfile, "preferences", "location_max_distance", conf.preferences_location_max_distance);


    if(!(configuration = g_key_file_to_data(conf.keyfile, &length, &err)))
    {
        ui_dialog(NULL,
                  GTK_MESSAGE_ERROR,
                  "Configuration",
                  "Unable to generate the configuration file.\n%s",
                  err->message);
        g_error_free(err);
        err = NULL;
    }

    if((fp = g_fopen(conf.path, "w")) == NULL)
    {
        ui_dialog(NULL,
                  GTK_MESSAGE_ERROR,
                  "Configuration",
                  "Unable to save the configuration to a file:\n%s",
                  conf.path);
    }
    else
    {
        out = fwrite(configuration, sizeof(char), length, fp);
        if(out != length)
        {
            ui_dialog(NULL,
                      GTK_MESSAGE_ERROR,
                      "Configuration",
                      "Failed to save the configuration to a file:\n%s\n(wrote only %d of %d bytes)",
                      conf.path, out, length);
        }
        fclose(fp);
    }
    g_free(configuration);
}

static void
conf_save_list(GtkListStore            *model,
               GtkTreeModelForeachFunc  func)
{
    gint number = 1;
    gtk_tree_model_foreach(GTK_TREE_MODEL(model), func, &number);
}

static gboolean
conf_save_profiles_foreach(GtkTreeModel *store,
                           GtkTreePath  *path,
                           GtkTreeIter  *iter,
                           gpointer     data)
{
    gint *number = (gint*)data;
    conf_profile_t *p;
    gchar *group_name;

    p = conf_profile_list_get(GTK_LIST_STORE(store), iter);
    group_name = g_strdup_printf("%s%d", CONF_PROFILE_GROUP_PREFIX, (*number)++);
    conf_save_profile(conf.keyfile, group_name, p);
    g_free(group_name);
    conf_profile_free(p);
    return FALSE;
}

static void
conf_save_profile(GKeyFile             *keyfile,
                  const gchar          *group_name,
                  const conf_profile_t *p)
{
    g_key_file_set_string(keyfile, group_name, "name", conf_profile_get_name(p));
    g_key_file_set_string(keyfile, group_name, "host", conf_profile_get_host(p));
    g_key_file_set_integer(keyfile, group_name, "port", conf_profile_get_port(p));
    g_key_file_set_string(keyfile, group_name, "login", conf_profile_get_login(p));
    g_key_file_set_string(keyfile, group_name, "password", conf_profile_get_password(p));
    g_key_file_set_string(keyfile, group_name, "interface", conf_profile_get_interface(p));
    g_key_file_set_integer(keyfile, group_name, "mode", conf_profile_get_mode(p));
    g_key_file_set_integer(keyfile, group_name, "duration_time", conf_profile_get_duration_time(p));
    g_key_file_set_boolean(keyfile, group_name, "duration", conf_profile_get_duration(p));
    g_key_file_set_boolean(keyfile, group_name, "remote", conf_profile_get_remote(p));
    g_key_file_set_boolean(keyfile, group_name, "background", conf_profile_get_background(p));
}

static gboolean
conf_save_scanlists_foreach(GtkTreeModel *store,
                            GtkTreePath  *path,
                            GtkTreeIter  *iter,
                            gpointer     data)
{
    gint *number = (gint*)data;
    conf_scanlist_t *sl;
    gchar *group_name;

    sl = conf_scanlist_list_get(GTK_LIST_STORE(store), iter);
    group_name = g_strdup_printf("%s%d", CONF_SCANLIST_GROUP_PREFIX, (*number)++);
    conf_save_scanlist(conf.keyfile, group_name, sl);
    g_free(group_name);
    conf_scanlist_free(sl);
    return FALSE;
}

static void
conf_save_scanlist(GKeyFile              *keyfile,
                   const gchar           *group_name,
                   const conf_scanlist_t *sl)
{
    g_key_file_set_string(keyfile, group_name, "name", conf_scanlist_get_name(sl));
    g_key_file_set_string(keyfile, group_name, "data", conf_scanlist_get_data(sl));
    if(conf_scanlist_get_main(sl))
        g_key_file_set_boolean(keyfile, group_name, "main", TRUE);
    if(conf_scanlist_get_default(sl))
        g_key_file_set_boolean(keyfile, group_name, "default", TRUE);
}

static void
conf_save_gint64_tree(GKeyFile    *keyfile,
                      const gchar *group_name,
                      const gchar *key,
                      GTree       *tree)
{
    gchar **values = NULL;
    gchar **ptr;
    gint length;
    gint i;

    length = g_tree_nnodes(tree);
    if(length)
    {
        values = g_new(gchar*, length);
        ptr = values;
        g_tree_foreach(tree, conf_save_gint64_tree_foreach, &ptr);
    }

    g_key_file_set_string_list(keyfile, group_name, key, (const gchar**)values, (gsize)length);

    if(values)
    {
        for(i=0; i<length; i++)
            g_free(values[i]);
        g_free(values);
    }
}

static gboolean
conf_save_gint64_tree_foreach(gpointer key,
                              gpointer value,
                              gpointer data)
{

    gchar ***ptr = (gchar***)data;
    *((*ptr)++) = g_strdup(model_format_address(*(gint64*)key, FALSE));
    return FALSE;
}

static void
conf_change_string(gchar      **ptr,
                   const gchar *value)
{
    g_free(*ptr);
    *ptr = g_strdup(value);
}

gint
conf_get_window_x(void)
{
    return conf.window_x;
}

gint
conf_get_window_y(void)
{
    return conf.window_y;
}

void
conf_set_window_xy(gint x,
                   gint y)
{
    conf.window_x = x;
    conf.window_y = y;
}

gint
conf_get_window_width(void)
{
    return conf.window_width;
}

gint
conf_get_window_height(void)
{
    return conf.window_height;
}

void
conf_set_window_position(gint width,
                         gint height)
{
    conf.window_width = width;
    conf.window_height = height;
}

gboolean
conf_get_window_maximized(void)
{
    return conf.window_maximized;
}

void
conf_set_window_maximized(gboolean value)
{
    conf.window_maximized = value;
}

gboolean
conf_get_interface_sound(void)
{
    return conf.interface_sound;
}

void
conf_set_interface_sound(gboolean sound)
{
    conf.interface_sound = sound;
}

gboolean
conf_get_interface_dark_mode(void)
{
    return conf.interface_dark_mode;
}

void
conf_set_interface_dark_mode(gboolean dark_mode)
{
    conf.interface_dark_mode = dark_mode;
}

gboolean
conf_get_interface_autosave(void)
{
    return conf.interface_autosave;
}

void
conf_set_interface_autosave(gboolean autosave)
{
    conf.interface_autosave = autosave;
}

gboolean
conf_get_interface_gps(void)
{
    return conf.interface_gps;
}

void
conf_set_interface_gps(gboolean gps)
{
    conf.interface_gps = gps;
}

gboolean
conf_get_interface_geoloc(void)
{
    return conf.interface_geoloc;
}

void
conf_set_interface_geoloc(gboolean geoloc)
{
    conf.interface_geoloc = geoloc;
}

gboolean
conf_get_interface_rotator(void)
{
    return conf.interface_rotator;
}

void
conf_set_interface_rotator(gboolean rotator)
{
    conf.interface_rotator = rotator;
}

gint
conf_get_interface_last_profile(void)
{
    return conf.interface_last_profile;
}

void
conf_set_interface_last_profile(gint profile)
{
    conf.interface_last_profile = profile;
}

const conf_profile_t*
conf_get_profile_default(void)
{
    return conf.profile_default;
}

void
conf_set_profile_default(conf_profile_t *p)
{
    conf_profile_free(conf.profile_default);
    conf.profile_default = p;
}

GtkListStore*
conf_get_profiles(void)
{
    return conf.profiles;
}

GtkListStore*
conf_get_scanlists(void)
{
    return conf.scanlists;
}

const gchar*
conf_get_path_log_open(void)
{
    return conf.path_log_open;
}

void
conf_set_path_log_open(const gchar *path)
{
    conf_change_string(&conf.path_log_open, path);
}

const gchar*
conf_get_path_log_save(void)
{
    return conf.path_log_save;
}

void
conf_set_path_log_save(const gchar *path)
{
    conf_change_string(&conf.path_log_save, path);
}

const gchar*
conf_get_path_log_export(void)
{
    return conf.path_log_export;
}

void
conf_set_path_log_export(const gchar *path)
{
    conf_change_string(&conf.path_log_export, path);
}

const gchar*
conf_get_path_autosave(void)
{
    return conf.path_autosave;
}

void
conf_set_path_autosave(const gchar *path)
{
    conf_change_string(&conf.path_autosave, path);
}

const gchar*
conf_get_path_screenshot(void)
{
    return conf.path_screenshot;
}

void
conf_set_path_screenshot(const gchar *path)
{
    conf_change_string(&conf.path_screenshot, path);
}

gint
conf_get_preferences_icon_size(void)
{
    return conf.preferences_icon_size;
}

void
conf_set_preferences_icon_size(gint value)
{
    conf.preferences_icon_size = value;
}

gint
conf_get_preferences_autosave_interval(void)
{
    return (conf.preferences_autosave_interval > 0 ? conf.preferences_autosave_interval : 1);
}

void
conf_set_preferences_autosave_interval(gint value)
{
    conf.preferences_autosave_interval = value;
}

gint
conf_get_preferences_search_column(void)
{
    return conf.preferences_search_column;
}

void
conf_set_preferences_search_column(gint value)
{
    conf.preferences_search_column = value;
}

const gchar*
conf_get_preferences_fallback_encoding(void)
{
    return conf.preferences_fallback_encoding;
}

void
conf_set_preferences_fallback_encoding(const gchar *value)
{
    conf_change_string(&conf.preferences_fallback_encoding, value);
}

gboolean
conf_get_preferences_no_style_override(void)
{
    return conf.preferences_no_style_override;
}

void
conf_set_preferences_no_style_override(gboolean value)
{
    conf.preferences_no_style_override = value;
}

gboolean
conf_get_preferences_signals(void)
{
    return conf.preferences_signals;
}

void
conf_set_preferences_signals(gboolean value)
{
    conf.preferences_signals = value;
}

gboolean
conf_get_preferences_display_time_only(void)
{
    return conf.preferences_display_time_only;
}

void
conf_set_preferences_display_time_only(gboolean value)
{
    conf.preferences_display_time_only = value;
}

gboolean
conf_get_preferences_reconnect(void)
{
    return conf.preferences_reconnect;
}

void
conf_set_preferences_reconnect(gboolean reconnect)
{
    conf.preferences_reconnect = reconnect;
}

const gchar* const*
conf_get_preferences_view_cols_order(void)
{
    return (const gchar* const*)conf.preferences_view_cols_order;
}

void
conf_set_preferences_view_cols_order(const gchar* const* value)
{
    g_strfreev(conf.preferences_view_cols_order);
    conf.preferences_view_cols_order = g_strdupv((gchar**)value);
}

const gchar* const*
conf_get_preferences_view_cols_hidden(void)
{
    return (const gchar* const*)conf.preferences_view_cols_hidden;
}

void
conf_set_preferences_view_cols_hidden(const gchar* const* value)
{
    g_strfreev(conf.preferences_view_cols_hidden);
    conf.preferences_view_cols_hidden = g_strdupv((gchar**)value);
}

gboolean
conf_get_preferences_sounds_new_network(void)
{
    return conf.preferences_sounds_new_network;
}

void
conf_set_preferences_sounds_new_network(gboolean value)
{
    conf.preferences_sounds_new_network = value;
}

gboolean
conf_get_preferences_sounds_new_network_hi(void)
{
    return conf.preferences_sounds_new_network_hi;
}

void
conf_set_preferences_sounds_new_network_hi(gboolean value)
{
    conf.preferences_sounds_new_network_hi = value;
}

gboolean
conf_get_preferences_sounds_new_network_al(void)
{
    return conf.preferences_sounds_new_network_al;
}

void
conf_set_preferences_sounds_new_network_al(gboolean value)
{
    conf.preferences_sounds_new_network_al = value;
}

gboolean
conf_get_preferences_sounds_no_data(void)
{
    return conf.preferences_sounds_no_data;
}

void
conf_set_preferences_sounds_no_data(gboolean value)
{
    conf.preferences_sounds_no_data = value;
}

gboolean
conf_get_preferences_sounds_no_gps_data(void)
{
    return conf.preferences_sounds_no_gps_data;
}

void
conf_set_preferences_sounds_no_gps_data(gboolean value)
{
    conf.preferences_sounds_no_gps_data = value;
}

gboolean
conf_get_preferences_events_new_network(void)
{
    return conf.preferences_events_new_network;
}

void
conf_set_preferences_events_new_network(gboolean value)
{
    conf.preferences_events_new_network = value;
}

const gchar*
conf_get_preferences_events_new_network_exec(void)
{
    return conf.preferences_events_new_network_exec;
}

void
conf_set_preferences_events_new_network_exec(const gchar *value)
{
    conf_change_string(&conf.preferences_events_new_network_exec, value);
}

gint
conf_get_preferences_tzsp_udp_port(void)
{
    return conf.preferences_tzsp_udp_port;
}

void
conf_set_preferences_tzsp_udp_port(gint value)
{
    conf.preferences_tzsp_udp_port = value;
}

gint
conf_get_preferences_tzsp_channel_width(void)
{
    return conf.preferences_tzsp_channel_width;
}

void
conf_set_preferences_tzsp_channel_width(gint value)
{
    conf.preferences_tzsp_channel_width = value;
}

mtscan_conf_tzsp_band_t
conf_get_preferences_tzsp_band(void)
{
    return conf.preferences_tzsp_band;
}

void
conf_set_preferences_tzsp_band(mtscan_conf_tzsp_band_t value)
{
    conf.preferences_tzsp_band = value;
}

const gchar*
conf_get_preferences_gps_hostname(void)
{
    return conf.preferences_gps_hostname;
}

void
conf_set_preferences_gps_hostname(const gchar *value)
{
    conf_change_string(&conf.preferences_gps_hostname, value);
}

gint
conf_get_preferences_gps_tcp_port(void)
{
    return conf.preferences_gps_tcp_port;
}

void
conf_set_preferences_gps_tcp_port(gint value)
{
    conf.preferences_gps_tcp_port = value;
}

gboolean
conf_get_preferences_gps_show_altitude(void)
{
    return conf.preferences_gps_show_altitude;
}

void
conf_set_preferences_gps_show_altitude(gboolean value)
{
    conf.preferences_gps_show_altitude = value;
}

gboolean
conf_get_preferences_gps_show_errors(void)
{
    return conf.preferences_gps_show_errors;
}

void
conf_set_preferences_gps_show_errors(gboolean value)
{
    conf.preferences_gps_show_errors = value;
}

const gchar*
conf_get_preferences_rotator_hostname(void)
{
    return conf.preferences_rotator_hostname;
}

void
conf_set_preferences_rotator_hostname(const gchar *value)
{
    conf_change_string(&conf.preferences_rotator_hostname, value);
}

gint
conf_get_preferences_rotator_tcp_port(void)
{
    return conf.preferences_rotator_tcp_port;
}

void
conf_set_preferences_rotator_tcp_port(gint value)
{
    conf.preferences_rotator_tcp_port = value;
}

const gchar*
conf_get_preferences_rotator_password(void)
{
    return conf.preferences_rotator_password;
}

void
conf_set_preferences_rotator_password(const gchar *value)
{
    conf_change_string(&conf.preferences_rotator_password, value);
}

gint
conf_get_preferences_rotator_min_speed(void)
{
    return conf.preferences_rotator_min_speed;
}

void
conf_set_preferences_rotator_min_speed(gint value)
{
    conf.preferences_rotator_min_speed = value;
}

gint
conf_get_preferences_rotator_def_speed(void)
{
    return conf.preferences_rotator_def_speed;
}

void
conf_set_preferences_rotator_def_speed(gint value)
{
    conf.preferences_rotator_def_speed = value;
}

gboolean
conf_get_preferences_blacklist_enabled(void)
{
    return conf.preferences_blacklist_enabled;
}

void
conf_set_preferences_blacklist_enabled(gboolean value)
{
    conf.preferences_blacklist_enabled = value;
}

gboolean
conf_get_preferences_blacklist_inverted(void)
{
    return conf.preferences_blacklist_inverted;
}

void
conf_set_preferences_blacklist_inverted(gboolean value)
{
    conf.preferences_blacklist_inverted = value;
}

gboolean
conf_get_preferences_blacklist_external(void)
{
    return conf.preferences_blacklist_external;
}

void
conf_set_preferences_blacklist_external(gboolean value)
{
    conf.preferences_blacklist_external = value;
}

const gchar*
conf_get_preferences_blacklist_ext_path(void)
{
    return conf.preferences_blacklist_ext_path;
}

void
conf_set_preferences_blacklist_ext_path(const gchar *path)
{
    conf_change_string(&conf.preferences_blacklist_ext_path, path);
}

gboolean
conf_get_preferences_blacklist(gint64 value)
{
    GTree *list = conf.preferences_blacklist_external ? conf.blacklist_ext : conf.blacklist;
    gboolean found = GPOINTER_TO_INT(g_tree_lookup(list, &value));
    return (conf.preferences_blacklist_inverted ? !found : found);
}

void
conf_set_preferences_blacklist(gint64 value)
{
    if(conf.preferences_blacklist_external)
        return;

    if(!conf.preferences_blacklist_inverted)
        g_tree_insert(conf.blacklist, gint64dup(&value), GINT_TO_POINTER(TRUE));
    else
        g_tree_remove(conf.blacklist, &value);
}

void
conf_del_preferences_blacklist(gint64 value)
{
    if(conf.preferences_blacklist_external)
        return;

    if(!conf.preferences_blacklist_inverted)
        g_tree_remove(conf.blacklist, &value);
    else
        g_tree_insert(conf.blacklist, gint64dup(&value), GINT_TO_POINTER(TRUE));
}

GtkListStore*
conf_get_preferences_blacklist_as_liststore(void)
{
    return create_liststore_from_tree(conf.blacklist);
}

void
conf_set_preferences_blacklist_from_liststore(GtkListStore *model)
{
    fill_tree_from_liststore(conf.blacklist, model);
}

gboolean
conf_get_preferences_highlightlist_enabled(void)
{
    return conf.preferences_highlightlist_enabled;
}

void
conf_set_preferences_highlightlist_enabled(gboolean value)
{
    conf.preferences_highlightlist_enabled = value;
}

gboolean
conf_get_preferences_highlightlist_inverted(void)
{
    return conf.preferences_highlightlist_inverted;
}

void
conf_set_preferences_highlightlist_inverted(gboolean value)
{
    conf.preferences_highlightlist_inverted = value;
}

gboolean
conf_get_preferences_highlightlist_external(void)
{
    return conf.preferences_highlightlist_external;
}

void
conf_set_preferences_highlightlist_external(gboolean value)
{
    conf.preferences_highlightlist_external = value;
}

const gchar*
conf_get_preferences_highlightlist_ext_path(void)
{
    return conf.preferences_highlightlist_ext_path;
}

void
conf_set_preferences_highlightlist_ext_path(const gchar *path)
{
    conf_change_string(&conf.preferences_highlightlist_ext_path, path);
}

gboolean
conf_get_preferences_highlightlist(gint64 value)
{
    GTree *list = conf.preferences_highlightlist_external ? conf.highlightlist_ext : conf.highlightlist;
    gboolean found = GPOINTER_TO_INT(g_tree_lookup(list, &value));
    return (conf.preferences_highlightlist_inverted ? !found : found);
}

void
conf_set_preferences_highlightlist(gint64 value)
{
    if(conf.preferences_highlightlist_external)
        return;

    if(!conf.preferences_highlightlist_inverted)
        g_tree_insert(conf.highlightlist, gint64dup(&value), GINT_TO_POINTER(TRUE));
    else
        g_tree_remove(conf.highlightlist, &value);
}

void
conf_del_preferences_highlightlist(gint64 value)
{
    if(conf.preferences_highlightlist_external)
        return;

    if(!conf.preferences_highlightlist_inverted)
        g_tree_remove(conf.highlightlist, &value);
    else
        g_tree_insert(conf.highlightlist, gint64dup(&value), GINT_TO_POINTER(TRUE));
}

GtkListStore*
conf_get_preferences_highlightlist_as_liststore(void)
{
    return create_liststore_from_tree(conf.highlightlist);
}

void
conf_set_preferences_highlightlist_from_liststore(GtkListStore *model)
{
    fill_tree_from_liststore(conf.highlightlist, model);
}

gboolean
conf_get_preferences_alarmlist_enabled(void)
{
    return conf.preferences_alarmlist_enabled;
}

void
conf_set_preferences_alarmlist_enabled(gboolean value)
{
    conf.preferences_alarmlist_enabled = value;
}

gboolean
conf_get_preferences_alarmlist_external(void)
{
    return conf.preferences_alarmlist_external;
}

void
conf_set_preferences_alarmlist_external(gboolean value)
{
    conf.preferences_alarmlist_external = value;
}

const gchar*
conf_get_preferences_alarmlist_ext_path(void)
{
    return conf.preferences_alarmlist_ext_path;
}

void
conf_set_preferences_alarmlist_ext_path(const gchar *path)
{
    conf_change_string(&conf.preferences_alarmlist_ext_path, path);
}

gboolean
conf_get_preferences_alarmlist(gint64 value)
{
    GTree *list = conf.preferences_alarmlist_external ? conf.alarmlist_ext : conf.alarmlist;
    gboolean found = GPOINTER_TO_INT(g_tree_lookup(list, &value));
    return found;
}

void
conf_set_preferences_alarmlist(gint64 value)
{
    if(conf.preferences_alarmlist_external)
        return;

    g_tree_insert(conf.alarmlist, gint64dup(&value), GINT_TO_POINTER(TRUE));
}

void
conf_del_preferences_alarmlist(gint64 value)
{
    if(conf.preferences_alarmlist_external)
        return;

    g_tree_remove(conf.alarmlist, &value);
}

GtkListStore*
conf_get_preferences_alarmlist_as_liststore(void)
{
    return create_liststore_from_tree(conf.alarmlist);
}

void
conf_set_preferences_alarmlist_from_liststore(GtkListStore *model)
{
    fill_tree_from_liststore(conf.alarmlist, model);
}

gdouble
conf_get_preferences_location_latitude(void)
{
    return conf.preferences_location_latitude;
}

void
conf_set_preferences_location_latitude(gdouble value)
{
    conf.preferences_location_latitude = value;
}

gdouble
conf_get_preferences_location_longitude(void)
{
    return conf.preferences_location_longitude;
}

void
conf_set_preferences_location_longitude(gdouble value)
{
    conf.preferences_location_longitude = value;
}

gboolean
conf_get_preferences_location_mtscan(void)
{
    return conf.preferences_location_mtscan;
}

void
conf_set_preferences_location_mtscan(gboolean value)
{
    conf.preferences_location_mtscan = value;
}

const gchar* const*
conf_get_preferences_location_mtscan_data(void)
{
    return (const gchar* const*)conf.preferences_location_mtscan_data;
}

void
conf_set_preferences_location_mtscan_data(const gchar* const* value)
{
    g_strfreev(conf.preferences_location_mtscan_data);
    conf.preferences_location_mtscan_data = g_strdupv((gchar**)value);
}

GtkListStore*
conf_get_preferences_location_mtscan_data_as_liststore(void)
{
    return create_liststore_from_strv((const gchar* const*)conf.preferences_location_mtscan_data);
}

gboolean
conf_get_preferences_location_wigle(void)
{
    return conf.preferences_location_wigle;
}

void
conf_set_preferences_location_wigle(gboolean value)
{
    conf.preferences_location_wigle = value;
}

const gchar*
conf_get_preferences_location_wigle_api_url(void)
{
    return conf.preferences_location_wigle_api_url;
}

void
conf_set_preferences_location_wigle_api_url(const gchar *value)
{
    conf_change_string(&conf.preferences_location_wigle_api_url, value);
}

const gchar*
conf_get_preferences_location_wigle_api_key(void)
{
    return conf.preferences_location_wigle_api_key;
}

void
conf_set_preferences_location_wigle_api_key(const gchar *value)
{
    conf_change_string(&conf.preferences_location_wigle_api_key, value);
}

gint
conf_get_preferences_location_azimuth_error(void)
{
    return conf.preferences_location_azimuth_error;
}

void
conf_set_preferences_location_azimuth_error(gint value)
{
    conf.preferences_location_azimuth_error = value;
}

gint
conf_get_preferences_location_min_distance(void)
{
    return conf.preferences_location_min_distance;
}

void
conf_set_preferences_location_min_distance(gint value)
{
    conf.preferences_location_min_distance = value;
}

gint
conf_get_preferences_location_max_distance(void)
{
    return conf.preferences_location_max_distance;
}

void
conf_set_preferences_location_max_distance(gint value)
{
    conf.preferences_location_max_distance = value;
}

