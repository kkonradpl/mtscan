#ifndef MTSCAN_UI_H_
#define MTSCAN_UI_H_
#include <gtk/gtk.h>
#include "mtscan.h"
#include "model.h"

#define UNIX_TIMESTAMP() (g_get_real_time() / 1000000)

#ifndef G_OS_WIN32
#define UI_ICON_SIZE 19
#else
#define UI_ICON_SIZE 18
#endif


typedef struct mtscan_gtk
{
    GtkWidget *window;
    GtkWidget *box;

    GtkWidget *toolbar;
    GtkToolItem *b_connect;
    GtkToolItem *b_scan;
    GtkToolItem *b_restart;
    GtkToolItem *b_scanlist_default;
    GtkToolItem *b_scanlist_full;
    GtkToolItem *b_scanlist;

    GtkToolItem *b_new;
    GtkToolItem *b_open;
    GtkToolItem *b_merge;
    GtkToolItem *b_save;
    GtkToolItem *b_save_as;
    GtkToolItem *b_export;
    GtkToolItem *b_screenshot;

    GtkToolItem *b_preferences;
    GtkToolItem *b_sound;
    GtkToolItem *b_mode;
    GtkToolItem *b_gps;
    GtkToolItem *b_signals;
    GtkToolItem *b_about;

    GtkWidget *scroll;
    GtkWidget *treeview;

    GtkWidget *statusbar;
    GtkWidget *heartbeat;
    GtkWidget *l_net_status;
    GtkWidget *l_conn_status;
    GtkWidget *l_gps_status;

    mtscan_model_t *model;
    gchar *filename;
    gchar *name;
    gboolean connected;
    gboolean scanning;
    gboolean changed;
    gboolean heartbeat_status;
} mtscan_gtk_t;

mtscan_gtk_t ui;

void ui_init();

void ui_connected(const gchar*, const gchar*, const gchar*);
void ui_disconnected();
void ui_changed();
void ui_status_update_networks();

void ui_set_title(gchar*);
void ui_clear();
void ui_show_uri(const gchar*);
void ui_play_sound(gchar*);
void ui_screenshot();

#endif
