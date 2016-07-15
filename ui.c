#include <string.h>
#include <time.h>
#include <gdk/gdkkeysyms.h>
#include "ui.h"
#include "ui-dialogs.h"
#include "ui-view.h"
#include "ui-toolbar.h"
#include "ui-connect.h"
#include "ui-icons.h"
#include "log.h"
#include "conn.h"
#include "conf.h"
#include "model.h"
#include "gps.h"
#include "signals.h"
#ifdef G_OS_WIN32
#include "win32.h"
#endif

#define UI_DRAG_URI_LIST_ID 0
static const GtkTargetEntry drop_types[] = {{ "text/uri-list", 0, UI_DRAG_URI_LIST_ID }};
static const gint n_drop_types = sizeof(drop_types) / sizeof(drop_types[0]);

static void ui_restore();
static gboolean ui_key(GtkWidget*, GdkEventKey*, gpointer);
static gboolean ui_delete_event(GtkWidget*, GdkEvent*, gpointer);
static void ui_drag_data_received(GtkWidget*, GdkDragContext*, gint, gint, GtkSelectionData*, guint, guint);
static gboolean ui_status_gps_timeout(gpointer);
static gchar* ui_get_name(const gchar*);

void
ui_init()
{
    ui_icon_load(UI_ICON_SIZE);

    ui.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_icon_name(APP_ICON);
    gtk_container_set_border_width(GTK_CONTAINER(ui.window), 0);
    ui_set_title(NULL);

    ui.box = gtk_vbox_new(FALSE, 2);
    gtk_container_set_border_width(GTK_CONTAINER(ui.box), 0);
    gtk_container_add(GTK_CONTAINER(ui.window), ui.box);

    ui.toolbar = ui_toolbar_create(ui.box);

    ui.scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(ui.scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(ui.box), ui.scroll, TRUE, TRUE, 0);

    ui.treeview = ui_view_new(ui.model);
    gtk_container_add(GTK_CONTAINER(ui.scroll), ui.treeview);

    ui.statusbar = gtk_hbox_new(FALSE, 6);
    gtk_box_pack_start(GTK_BOX(ui.box), ui.statusbar, FALSE, FALSE, 1);

    ui.heartbeat = gtk_drawing_area_new();
    gtk_widget_set_tooltip_text(ui.heartbeat, "Activity icon");
#if GTK_CHECK_VERSION (3, 0, 0)
    g_signal_connect(G_OBJECT(ui.heartbeat), "draw", G_CALLBACK(ui_icon_draw_heartbeat), &ui.heartbeat_status);
#else
    g_signal_connect(G_OBJECT(ui.heartbeat), "expose-event", G_CALLBACK(ui_icon_draw_heartbeat), &ui.heartbeat_status);
#endif
    gtk_box_pack_start(GTK_BOX(ui.statusbar), ui.heartbeat, FALSE, FALSE, 1);

    ui.l_net_status = gtk_label_new(NULL);
    gtk_misc_set_alignment(GTK_MISC(ui.l_net_status), 0.0, 0.5);
    gtk_box_pack_start(GTK_BOX(ui.statusbar), ui.l_net_status, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(ui.statusbar), gtk_vseparator_new(), FALSE, FALSE, 5);

    ui.l_conn_status = gtk_label_new(NULL);
    gtk_box_pack_start(GTK_BOX(ui.statusbar), ui.l_conn_status, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(ui.statusbar), gtk_vseparator_new(), FALSE, FALSE, 5);

    ui.l_gps_status = gtk_label_new(NULL);
    gtk_box_pack_start(GTK_BOX(ui.statusbar), ui.l_gps_status, FALSE, FALSE, 0);

    ui_status_gps_timeout(NULL);
    g_timeout_add(1000, ui_status_gps_timeout, NULL);

    gtk_drag_dest_set(ui.window, GTK_DEST_DEFAULT_ALL, drop_types, n_drop_types, GDK_ACTION_COPY);
    g_signal_connect(ui.window, "drag-data-received", G_CALLBACK(ui_drag_data_received), NULL);
    g_signal_connect(ui.window, "destroy", gtk_main_quit, NULL);
    g_signal_connect(ui.window, "delete-event", G_CALLBACK(ui_delete_event), NULL);
    g_signal_connect(ui.window, "key-press-event", G_CALLBACK(ui_key), ui.treeview);

    ui_restore();
    ui_disconnected();
    gtk_widget_show_all(ui.window);
}

static void
ui_restore()
{
    gint x = conf_get_window_x();
    gint y = conf_get_window_y();

    if(x >= 0 && y >= 0)
        gtk_window_move(GTK_WINDOW(ui.window), x, y);
    else
        gtk_window_set_position(GTK_WINDOW(ui.window), GTK_WIN_POS_CENTER);

    gtk_window_set_default_size(GTK_WINDOW(ui.window),
                                conf_get_window_width(),
                                conf_get_window_height());

    if(conf_get_interface_dark_mode())
        g_signal_emit_by_name(ui.b_mode, "clicked");

    if(conf_get_interface_sound())
        g_signal_emit_by_name(ui.b_sound, "clicked");

    if(conf_get_interface_gps())
        g_signal_emit_by_name(ui.b_gps, "clicked");

    if(conf_get_interface_signals())
        g_signal_emit_by_name(ui.b_signals, "clicked");
}

static gboolean
ui_delete_event(GtkWidget *widget,
                GdkEvent  *event,
                gpointer   data)
{
    gboolean really_quit = TRUE;
    gint x, y;

    if(ui.changed)
        really_quit = ui_dialog_ask_unsaved();

    if(really_quit)
    {
        gtk_window_get_position(GTK_WINDOW(widget), &x, &y);
        conf_set_window_xy(x, y);
        gtk_window_get_size(GTK_WINDOW(widget), &x, &y);
        conf_set_window_position(x, y);
        conf_save();
    }

    return !really_quit;
}

static gboolean
ui_key(GtkWidget   *widget,
       GdkEventKey *event,
       gpointer     treeview)
{
    guint current = gdk_keyval_to_upper(event->keyval);
    if(current == GDK_KEY_Escape)
    {
        gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)));
        return TRUE;
    }
    return FALSE;
}

static void
ui_drag_data_received(GtkWidget        *widget,
                      GdkDragContext   *context,
                      gint              x,
                      gint              y,
                      GtkSelectionData *selection_data,
                      guint             info,
                      guint             time)
{
    GSList *filenames = NULL;
    gchar **list, **uri;
    gchar *current;
    gint count = 0;
    gint action;

    if(!selection_data || info != UI_DRAG_URI_LIST_ID)
        return;

    list = gtk_selection_data_get_uris(selection_data);
    if(!list)
        return;

    uri = list;
    while(*uri)
    {
        current = g_filename_from_uri(*uri, NULL, NULL);
        if(current)
        {
            filenames = g_slist_prepend(filenames, current);
            count++;
        }
        uri++;
    }
    g_strfreev(list);

    if(filenames)
    {
        action = (count > 1 ? ui_dialog_ask_merge(count) : ui_dialog_ask_open_or_merge());
        if(action != UI_DIALOG_CANCEL)
            log_open(filenames, action);
        g_slist_free_full(filenames, g_free);
    }
}

static gboolean
ui_status_gps_timeout(gpointer data)
{
    static gint last_gps_state = -1;
    const mtscan_gps_data_t *gps_data;
    gint state;
    gchar *text;

    state = gps_get_data(&gps_data);

    if(conf_get_interface_sound() &&
       state > GPS_OFF &&
       state < GPS_OK &&
       UNIX_TIMESTAMP() % 2)
    {
        ui_play_sound(APP_SOUND_GPSLOST);
    }

    if(state != last_gps_state || state == GPS_OK)
    {
        switch(state)
        {
        case GPS_OFF:
            gtk_label_set_text(GTK_LABEL(ui.l_gps_status), "GPS: off");
            break;
        case GPS_OPENING:
            gtk_label_set_markup(GTK_LABEL(ui.l_gps_status), "GPS: <span color=\"red\"><b>opening…</b></span>");
            break;
        case GPS_WAITING_FOR_DATA:
            gtk_label_set_markup(GTK_LABEL(ui.l_gps_status), "GPS: <span color=\"red\"><b>waiting for data</b></span>");
            break;
        case GPS_INVALID:
            gtk_label_set_markup(GTK_LABEL(ui.l_gps_status), "GPS: <span color=\"red\"><b>no fix</b></span>");
            break;
        case GPS_OK:
            text = g_strdup_printf("GPS: %c%.5f° %c%.5f° (HDOP: %.1f)",
                                   (gps_data->lat >= 0.0 ? 'N' : 'S'),
                                   gps_data->lat,
                                   (gps_data->lon >= 0.0 ? 'E' : 'W'),
                                   gps_data->lon,
                                   gps_data->hdop);
            gtk_label_set_text(GTK_LABEL(ui.l_gps_status), text);
            g_free(text);
        }
        last_gps_state = state;
    }
    return TRUE;
}

void
ui_connected(const gchar *login,
             const gchar *host,
             const gchar *iface)
{
    gchar *text = g_strdup_printf("%s@%s/%s", login, host, iface);
    gtk_label_set_text(GTK_LABEL(ui.l_conn_status), text);
    g_free(text);
    ui_toolbar_connect_set_state(TRUE);
    ui.connected = TRUE;
}

void
ui_disconnected()
{
    ui.connected = FALSE;
    ui.scanning = FALSE;
    conn = NULL;

    ui_toolbar_connect_set_state(FALSE);
    ui_toolbar_scan_set_state(FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(ui.b_connect), TRUE);
    gtk_widget_set_sensitive(GTK_WIDGET(ui.b_scan), TRUE);
    gtk_widget_set_sensitive(GTK_WIDGET(ui.b_restart), TRUE);
    gtk_label_set_text(GTK_LABEL(ui.l_conn_status), "disconnected");

    mtscan_model_clear_active(ui.model);
    ui_status_update_networks();
}

void
ui_changed()
{
    if(!ui.changed)
    {
        ui.changed = TRUE;
        ui_set_title(ui.filename);
    }
}

void
ui_status_update_networks()
{
    static gint last_networks = -1;
    static gint last_active = -1;
    gint networks, active;
    gchar *text;

    networks = g_hash_table_size(ui.model->map);
    active = g_hash_table_size(ui.model->active);
    if(networks != last_networks ||
       active != last_active)
    {
        text = g_strdup_printf("%d/%d networks", active, networks);
        gtk_label_set_text(GTK_LABEL(ui.l_net_status), text);
        g_free(text);
        last_active = active;
        last_networks = networks;
    }
}

void
ui_set_title(gchar *filename)
{
    gchar *title;

    if(filename != ui.filename)
    {
        g_free(ui.filename);
        ui.filename = filename;
        g_free(ui.name);
        ui.name = (filename ? ui_get_name(filename) : NULL);
    }

    if(!ui.filename)
    {
        gtk_window_set_title(GTK_WINDOW(ui.window), APP_NAME);
        return;
    }

    title = g_strdup_printf(APP_NAME " [%s%s]", (ui.changed?"*":""), ui.name);
    gtk_window_set_title(GTK_WINDOW(ui.window), title);
    g_free(title);
}

static gchar*
ui_get_name(const gchar *filename)
{
    gchar *name;
    gchar *ext;

    name = g_path_get_basename(filename);
    ext = strrchr(name, '.');
    if(ext && !g_ascii_strcasecmp(ext, ".gz"))
    {
        *ext = '\0';
        ext = strrchr(name, '.');
    }
    if(ext && !g_ascii_strcasecmp(ext, APP_FILE_EXT))
        *ext = '\0';

    return name;
}


void
ui_clear()
{
    mtscan_model_clear(ui.model);
    ui.changed = FALSE;
    ui_status_update_networks();
}

void
ui_show_uri(const gchar *uri)
{
#ifdef G_OS_WIN32
    win32_uri(uri);
#else
    GError *err = NULL;
    if(!gtk_show_uri(NULL, uri, gtk_get_current_event_time(), &err))
    {
        ui_dialog(ui.window, GTK_MESSAGE_ERROR, "Error", "%s", err->message);
        g_error_free(err);
    }
#endif
}

void
ui_play_sound(gchar *filename)
{
    if(!conf_get_interface_sound())
        return;

#ifdef G_OS_WIN32
    win32_play(filename);
#else
    gchar *command[] = { APP_SOUND_EXEC, filename, NULL };
    GError *error = NULL;
    if(!g_spawn_async(NULL, command, NULL, G_SPAWN_SEARCH_PATH, 0, NULL, NULL, &error))
    {
        fprintf(stderr, "Unable to start " APP_SOUND_EXEC ": %s\n", error->message);
        g_error_free(error);
    }
#endif
}

void
ui_screenshot()
{
    GdkPixmap *pixmap;
    GdkPixbuf *pixbuf;
    GError *err = NULL;
    gint width, height;
    time_t tt = time(NULL);
    gchar *filename;
    gchar t[20];

    strftime(t, sizeof(t), "%Y%m%d-%H%M%S", localtime(&tt));
    filename = g_strdup_printf("%s/mtscan-%s.png", g_get_home_dir(), t);

    gtk_widget_queue_draw(ui.window);
    gdk_window_process_all_updates();

    pixmap = gtk_widget_get_snapshot(ui.window, NULL);
    gdk_pixmap_get_size(pixmap, &width, &height);
    pixbuf = gdk_pixbuf_get_from_drawable(NULL, pixmap, NULL, 0, 0, 0, 0, width, height);
    if(!gdk_pixbuf_save(pixbuf, filename, "png", &err, NULL))
    {
        ui_dialog(ui.window,
                  GTK_MESSAGE_ERROR,
                  "Screenshot",
                  "%s",
                  err->message);
        g_error_free(err);
    }
    g_object_unref(G_OBJECT(pixmap));
    g_object_unref(G_OBJECT(pixbuf));
    g_free(filename);
}
