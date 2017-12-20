#include <string.h>
#include <glib/gstdio.h>
#include <math.h>
#include "ui-dialogs.h"
#include "conf.h"
#include "ui-view.h"
#include "misc.h"
#include "conf-profile.h"

#define CONF_DIR  "mtscan"
#define CONF_FILE "mtscan.conf"

#define CONF_DEFAULT_WINDOW_X         -1
#define CONF_DEFAULT_WINDOW_Y         -1
#define CONF_DEFAULT_WINDOW_WIDTH     1000
#define CONF_DEFAULT_WINDOW_HEIGHT    500
#define CONF_DEFAULT_WINDOW_MAXIMIZED FALSE

#define CONF_DEFAULT_INTERFACE_LAST_PROFILE  -1
#define CONF_DEFAULT_INTERFACE_SOUND         FALSE
#define CONF_DEFAULT_INTERFACE_DARK_MODE     FALSE
#define CONF_DEFAULT_INTERFACE_GPS           FALSE

#define CONF_DEFAULT_PROFILE_NAME           "unnamed"
#define CONF_DEFAULT_PROFILE_HOST           ""
#define CONF_DEFAULT_PROFILE_PORT           22
#define CONF_DEFAULT_PROFILE_LOGIN          "admin"
#define CONF_DEFAULT_PROFILE_PASSWORD       ""
#define CONF_DEFAULT_PROFILE_INTERFACE      "wlan1"
#define CONF_DEFAULT_PROFILE_DURATION_TIME  10
#define CONF_DEFAULT_PROFILE_DURATION       FALSE
#define CONF_DEFAULT_PROFILE_REMOTE         FALSE
#define CONF_DEFAULT_PROFILE_BACKGROUND     FALSE

#define CONF_DEFAULT_PATH_LOG_OPEN   ""
#define CONF_DEFAULT_PATH_LOG_SAVE   ""
#define CONF_DEFAULT_PATH_LOG_EXPORT ""

#define CONF_DEFAULT_PREFERENCES_ICON_SIZE              18
#define CONF_DEFAULT_PREFERENCES_SEARCH_COLUMN          1
#define CONF_DEFAULT_PREFERENCES_LATLON_COLUMN          FALSE
#define CONF_DEFAULT_PREFERENCES_AZIMUTH_COLUMN         FALSE
#define CONF_DEFAULT_PREFERENCES_SIGNALS                TRUE
#define CONF_DEFAULT_PREFERENCES_GPS_HOSTNAME           "localhost"
#define CONF_DEFAULT_PREFERENCES_GPS_TCP_PORT           2947
#define CONF_DEFAULT_PREFERENCES_BLACKLIST_ENABLED      FALSE
#define CONF_DEFAULT_PREFERENCES_BLACKLIST_INVERTED     FALSE
#define CONF_DEFAULT_PREFERENCES_HIGHLIGHTLIST_ENABLED  FALSE
#define CONF_DEFAULT_PREFERENCES_HIGHLIGHTLIST_INVERTED FALSE

#define CONF_PROFILE_GROUP_PREFIX  "profile_"
#define CONF_PROFILE_GROUP_DEFAULT "profile"

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
    gboolean interface_gps;
    gint     interface_last_profile;

    /* [profile] */
    conf_profile_t *profile_default;

    /* [profile_x] */
    GtkListStore *profiles;

    /* [path] */
    gchar *path_log_open;
    gchar *path_log_save;
    gchar *path_log_export;

    /* [preferences] */
    gint     preferences_icon_size;
    gint     preferences_search_column;
    gboolean preferences_latlon_column;
    gboolean preferences_azimuth_column;
    gboolean preferences_signals;

    gchar    *preferences_gps_hostname;
    gint      preferences_gps_tcp_port;
    gboolean  preferences_blacklist_enabled;
    gboolean  preferences_blacklist_inverted;
    GTree    *blacklist;
    gboolean  preferences_highlightlist_enabled;
    gboolean  preferences_highlightlist_inverted;
    GTree    *highlightlist;

} conf_t;

static conf_t conf;

static void            conf_read(void);
static gboolean        conf_read_boolean(const gchar*, const gchar*, gboolean);
static gint            conf_read_integer(const gchar*, const gchar*, gint);
static gchar*          conf_read_string(const gchar*, const gchar*, const gchar*);
static void            conf_read_gint64_tree(GKeyFile*, const gchar*, const gchar*, GTree*);
static GtkListStore*   conf_read_profiles(void);
static conf_profile_t* conf_read_profile(const gchar*);

static void            conf_save_profiles(GtkListStore*);
static gboolean        conf_save_profiles_foreach(GtkTreeModel*, GtkTreePath*, GtkTreeIter*, gpointer);
static void            conf_save_profile(GKeyFile*, const gchar*, conf_profile_t*);
static void            conf_save_gint64_tree(GKeyFile*, const gchar*, const gchar*, GTree*);
static gboolean        conf_save_gint64_tree_foreach(gpointer, gpointer, gpointer);

static void            conf_change_string(gchar**, const gchar*);

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
    conf.highlightlist = g_tree_new_full((GCompareDataFunc)gint64cmp, NULL, g_free, NULL);
    conf_read();
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
    conf.interface_gps = conf_read_boolean("interface", "gps", CONF_DEFAULT_INTERFACE_GPS);
    conf.interface_last_profile = conf_read_integer("interface", "last_profile", CONF_DEFAULT_INTERFACE_LAST_PROFILE);

    conf.profile_default = conf_read_profile(CONF_PROFILE_GROUP_DEFAULT);
    conf.profiles = conf_read_profiles();

    conf.path_log_open = conf_read_string("path", "log_open", CONF_DEFAULT_PATH_LOG_OPEN);
    conf.path_log_save = conf_read_string("path", "log_save", CONF_DEFAULT_PATH_LOG_SAVE);
    conf.path_log_export = conf_read_string("path", "log_export", CONF_DEFAULT_PATH_LOG_EXPORT);

    conf.preferences_icon_size = conf_read_integer("preferences", "icon_size", CONF_DEFAULT_PREFERENCES_ICON_SIZE);
    conf.preferences_search_column = conf_read_integer("preferences", "search_column", CONF_DEFAULT_PREFERENCES_SEARCH_COLUMN);
    conf.preferences_latlon_column = conf_read_boolean("preferences", "latlon_column", CONF_DEFAULT_PREFERENCES_LATLON_COLUMN);
    conf.preferences_azimuth_column = conf_read_boolean("preferences", "azimuth_column", CONF_DEFAULT_PREFERENCES_AZIMUTH_COLUMN);
    conf.preferences_signals = conf_read_boolean("preferences", "signals", CONF_DEFAULT_PREFERENCES_SIGNALS);
    conf.preferences_gps_hostname = conf_read_string("preferences", "gps_hostname", CONF_DEFAULT_PREFERENCES_GPS_HOSTNAME);
    conf.preferences_gps_tcp_port = conf_read_integer("preferences", "gps_tcp_port", CONF_DEFAULT_PREFERENCES_GPS_TCP_PORT);
    conf.preferences_blacklist_enabled = conf_read_boolean("preferences", "blacklist_enabled", CONF_DEFAULT_PREFERENCES_BLACKLIST_ENABLED);
    conf.preferences_blacklist_inverted = conf_read_boolean("preferences", "blacklist_inverted", CONF_DEFAULT_PREFERENCES_BLACKLIST_INVERTED);
    conf_read_gint64_tree(conf.keyfile, "preferences", "blacklist", conf.blacklist);
    conf.preferences_highlightlist_enabled = conf_read_boolean("preferences", "highlightlist_enabled", CONF_DEFAULT_PREFERENCES_HIGHLIGHTLIST_ENABLED);
    conf.preferences_highlightlist_inverted = conf_read_boolean("preferences", "highlightlist_inverted", CONF_DEFAULT_PREFERENCES_HIGHLIGHTLIST_INVERTED);
    conf_read_gint64_tree(conf.keyfile, "preferences", "highlightlist", conf.highlightlist);

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


static GtkListStore*
conf_read_profiles(void)
{
    gchar **groups = g_key_file_get_groups(conf.keyfile, NULL);
    gchar **group;
    GtkListStore *model;
    conf_profile_t *p;

    model = conf_profile_list_new();
    for(group=groups; *group; ++group)
    {
        if(!strncmp(*group, CONF_PROFILE_GROUP_PREFIX, strlen(CONF_PROFILE_GROUP_PREFIX)))
        {
            p = conf_read_profile(*group);
            conf_profile_list_add(model, p);
            conf_profile_free(p);
            g_key_file_remove_group(conf.keyfile, *group, NULL);
        }
    }

    g_strfreev(groups);
    return model;
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
                         conf_read_integer(group_name, "duration_time", CONF_DEFAULT_PROFILE_DURATION_TIME),
                         conf_read_boolean(group_name, "duration", CONF_DEFAULT_PROFILE_DURATION),
                         conf_read_boolean(group_name, "remote", CONF_DEFAULT_PROFILE_REMOTE),
                         conf_read_boolean(group_name, "background", CONF_DEFAULT_PROFILE_BACKGROUND));
    return p;
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
    g_key_file_set_boolean(conf.keyfile, "interface", "gps", conf.interface_gps);
    g_key_file_set_integer(conf.keyfile, "interface", "last_profile", conf.interface_last_profile);

    conf_save_profile(conf.keyfile, CONF_PROFILE_GROUP_DEFAULT, conf.profile_default);
    conf_save_profiles(conf.profiles);

    g_key_file_set_string(conf.keyfile, "path", "log_open", conf.path_log_open);
    g_key_file_set_string(conf.keyfile, "path", "log_save", conf.path_log_save);
    g_key_file_set_string(conf.keyfile, "path", "log_export", conf.path_log_export);

    g_key_file_set_integer(conf.keyfile, "preferences", "icon_size", conf.preferences_icon_size);
    g_key_file_set_integer(conf.keyfile, "preferences", "search_column", conf.preferences_search_column);
    g_key_file_set_boolean(conf.keyfile, "preferences", "latlon_column", conf.preferences_latlon_column);
    g_key_file_set_boolean(conf.keyfile, "preferences", "azimuth_column", conf.preferences_azimuth_column);
    g_key_file_set_boolean(conf.keyfile, "preferences", "signals", conf.preferences_signals);
    g_key_file_set_string(conf.keyfile, "preferences", "gps_hostname", conf.preferences_gps_hostname);
    g_key_file_set_integer(conf.keyfile, "preferences", "gps_tcp_port", conf.preferences_gps_tcp_port);
    g_key_file_set_boolean(conf.keyfile, "preferences", "blacklist_enabled", conf.preferences_blacklist_enabled);
    g_key_file_set_boolean(conf.keyfile, "preferences", "blacklist_inverted", conf.preferences_blacklist_inverted);
    conf_save_gint64_tree(conf.keyfile, "preferences", "blacklist", conf.blacklist);
    g_key_file_set_boolean(conf.keyfile, "preferences", "highlightlist_enabled", conf.preferences_highlightlist_enabled);
    g_key_file_set_boolean(conf.keyfile, "preferences", "highlightlist_inverted", conf.preferences_highlightlist_inverted);
    conf_save_gint64_tree(conf.keyfile, "preferences", "highlightlist", conf.highlightlist);

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

    if((fp = fopen(conf.path, "w")) == NULL)
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
conf_save_profiles(GtkListStore *model)
{
    gint number = 0;
    gtk_tree_model_foreach(GTK_TREE_MODEL(model), conf_save_profiles_foreach, &number);
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

    p = conf_profile_get(iter);
    group_name = g_strdup_printf("%s%d", CONF_PROFILE_GROUP_PREFIX, (*number)++);
    conf_save_profile(conf.keyfile, group_name, p);
    g_free(group_name);
    conf_profile_free(p);
    return FALSE;
}

static void
conf_save_profile(GKeyFile       *keyfile,
                  const gchar    *group_name,
                  conf_profile_t *p)
{
    g_key_file_set_string(keyfile, group_name, "name", conf_profile_get_name(p));
    g_key_file_set_string(keyfile, group_name, "host", conf_profile_get_host(p));
    g_key_file_set_integer(keyfile, group_name, "port", conf_profile_get_port(p));
    g_key_file_set_string(keyfile, group_name, "login", conf_profile_get_login(p));
    g_key_file_set_string(keyfile, group_name, "password", conf_profile_get_password(p));
    g_key_file_set_string(keyfile, group_name, "interface", conf_profile_get_interface(p));
    g_key_file_set_integer(keyfile, group_name, "duration_time", conf_profile_get_duration_time(p));
    g_key_file_set_boolean(keyfile, group_name, "duration", conf_profile_get_duration(p));
    g_key_file_set_boolean(keyfile, group_name, "remote", conf_profile_get_remote(p));
    g_key_file_set_boolean(keyfile, group_name, "background", conf_profile_get_background(p));
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
conf_get_interface_gps(void)
{
    return conf.interface_gps;
}

void
conf_set_interface_gps(gboolean gps)
{
    conf.interface_gps = gps;
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

GtkTreeIter
conf_profile_add(const conf_profile_t *p)
{
    return conf_profile_list_add(conf.profiles, p);
}

conf_profile_t*
conf_profile_get(GtkTreeIter *iter)
{
    return conf_profile_list_get(conf.profiles, iter);
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
conf_get_preferences_search_column(void)
{
    return conf.preferences_search_column;
}

void
conf_set_preferences_search_column(gint value)
{
    conf.preferences_search_column = value;
}

gboolean
conf_get_preferences_latlon_column(void)
{
    return conf.preferences_latlon_column;
}

void
conf_set_preferences_latlon_column(gboolean value)
{
    conf.preferences_latlon_column = value;
}

gboolean
conf_get_preferences_azimuth_column(void)
{
    return conf.preferences_azimuth_column;
}

void
conf_set_preferences_azimuth_column(gboolean value)
{
    conf.preferences_azimuth_column = value;
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
conf_get_preferences_blacklist(gint64 value)
{
    gboolean found = GPOINTER_TO_INT(g_tree_lookup(conf.blacklist, &value));
    return (conf.preferences_blacklist_inverted ? !found : found);
}

void
conf_set_preferences_blacklist(gint64 value)
{
    if(!conf.preferences_blacklist_inverted)
        g_tree_insert(conf.blacklist, gint64dup(&value), GINT_TO_POINTER(TRUE));
    else
        g_tree_remove(conf.blacklist, &value);
}

void
conf_del_preferences_blacklist(gint64 value)
{
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
conf_get_preferences_highlightlist(gint64 value)
{
    gboolean found = GPOINTER_TO_INT(g_tree_lookup(conf.highlightlist, &value));
    return (conf.preferences_highlightlist_inverted ? !found : found);
}

void
conf_set_preferences_highlightlist(gint64 value)
{
    if(!conf.preferences_highlightlist_inverted)
        g_tree_insert(conf.highlightlist, gint64dup(&value), GINT_TO_POINTER(TRUE));
    else
        g_tree_remove(conf.highlightlist, &value);
}

void
conf_del_preferences_highlightlist(gint64 value)
{
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
