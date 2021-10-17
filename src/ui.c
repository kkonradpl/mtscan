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

#include <string.h>
#include <time.h>
#include <math.h>
#include <gdk/gdkkeysyms.h>
#include "ui.h"
#include "ui-dialogs.h"
#include "ui-view.h"
#include "ui-toolbar.h"
#include "ui-connection.h"
#include "ui-icons.h"
#include "ui-log.h"
#include "log.h"
#include "conf.h"
#include "model.h"
#include "gnss.h"
#include "signals.h"
#include "misc.h"
#include "ui-callbacks.h"

#ifdef G_OS_WIN32
#include "win32.h"
#endif

#define ACTIVITY_TIMEOUT 5

#define UI_DRAG_URI_LIST_ID 0
static const GtkTargetEntry drop_types[] = {{ "text/uri-list", 0, UI_DRAG_URI_LIST_ID }};
static const gint n_drop_types = sizeof(drop_types) / sizeof(drop_types[0]);
static const char rc_string[] = "style \"minimal-toolbar-style\"\n"
                                "{\n"
                                    "GtkToolbar::shadow-type = GTK_SHADOW_NONE\n"
                                 "}\n"
                                 "widget \"*.minimal-toolbar\" style\n\"minimal-toolbar-style\"\n"
                                 "style \"treeview-normal-style\"\n"
                                 "{\n"
                                    "GtkTreeView::even-row-color = \"#FFFFFF\"\n"
                                 "}\n"
                                "widget \"*.treeview-normal\" style\n\"treeview-normal-style\"\n"
                                "style \"treeview-dark-style\"\n"
                                "{\n"
                                "GtkTreeView::even-row-color = \"#151515\"\n"
                                "}\n"
                                "widget \"*.treeview-dark\" style\n\"treeview-dark-style\"\n";

mtscan_gtk_t ui;

static void ui_restore(void);
static gboolean ui_key(GtkWidget*, GdkEventKey*, gpointer);
static gboolean ui_delete_event(GtkWidget*, GdkEvent*, gpointer);
static void ui_drag_data_received(GtkWidget*, GdkDragContext*, gint, gint, GtkSelectionData*, guint, guint);
static gboolean ui_idle_timeout(gpointer);
static gboolean ui_idle_timeout_autosave(gpointer);
static void ui_gnss(mtscan_gnss_state_t, const mtscan_gnss_data_t*, gpointer);
static gchar* ui_get_name(const gchar*);

void
ui_init(void)
{
    gint icon_size = conf_get_preferences_icon_size();

    ui_icon_size(icon_size);
    gtk_rc_parse_string(rc_string);

    ui.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_icon_name(APP_ICON);
    gtk_container_set_border_width(GTK_CONTAINER(ui.window), 0);
    ui_set_title(NULL);

    ui.box = gtk_vbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(ui.box), 0);
    gtk_container_add(GTK_CONTAINER(ui.window), ui.box);

    gtk_box_pack_start(GTK_BOX(ui.box), gtk_hseparator_new(), FALSE, FALSE, 0);

    ui.box_toolbar = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(ui.box), ui.box_toolbar, FALSE, FALSE, 0);

    ui.toolbar = ui_toolbar_create();
    gtk_widget_set_name(ui.toolbar, "minimal-toolbar");
    gtk_box_pack_start(GTK_BOX(ui.box_toolbar), ui.toolbar, TRUE, TRUE, 1);

    gtk_box_pack_start(GTK_BOX(ui.box), gtk_hseparator_new(), FALSE, FALSE, 0);

    ui.scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(ui.scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(ui.box), ui.scroll, TRUE, TRUE, 0);

    ui.treeview = ui_view_new(ui.model, icon_size);
    gtk_container_add(GTK_CONTAINER(ui.scroll), ui.treeview);

    ui.statusbar_align = gtk_alignment_new(0, 0, 0, 0);
    gtk_alignment_set_padding(GTK_ALIGNMENT(ui.statusbar_align), 2, 0, 0, 0);
    gtk_box_pack_start(GTK_BOX(ui.box), ui.statusbar_align, FALSE, FALSE, 0);

    ui.statusbar = gtk_hbox_new(FALSE, 6);
    gtk_container_add(GTK_CONTAINER(ui.statusbar_align), ui.statusbar);

    ui.activity_icon = gtk_drawing_area_new();
    gtk_widget_set_tooltip_text(ui.activity_icon, "Activity icon");
#if GTK_CHECK_VERSION (3, 0, 0)
    g_signal_connect(G_OBJECT(ui.activity_icon), "draw", G_CALLBACK(ui_icon_draw_heartbeat), &ui.activity);
#else
    g_signal_connect(G_OBJECT(ui.activity_icon), "expose-event", G_CALLBACK(ui_icon_draw_heartbeat), &ui.activity);
#endif
    gtk_box_pack_start(GTK_BOX(ui.statusbar), ui.activity_icon, FALSE, FALSE, 1);

    ui.l_net_status = gtk_label_new(NULL);
    gtk_misc_set_alignment(GTK_MISC(ui.l_net_status), 0.0, 0.5);
    gtk_box_pack_start(GTK_BOX(ui.statusbar), ui.l_net_status, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(ui.statusbar), gtk_vseparator_new(), FALSE, FALSE, 5);

    ui.l_conn_status = gtk_label_new(NULL);
    gtk_box_pack_start(GTK_BOX(ui.statusbar), ui.l_conn_status, FALSE, FALSE, 0);

    ui.group_gnss = gtk_hbox_new(FALSE, 6);
    gtk_box_pack_start(GTK_BOX(ui.group_gnss), gtk_vseparator_new(), FALSE, FALSE, 5);
    ui.l_gnss_status = gtk_label_new(NULL);
    gtk_box_pack_start(GTK_BOX(ui.group_gnss), ui.l_gnss_status, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(ui.statusbar), ui.group_gnss, FALSE, FALSE, 0);

    ui_idle_timeout(&ui);
    g_timeout_add(1000, ui_idle_timeout, &ui);
    g_timeout_add(1000, ui_idle_timeout_autosave, &ui);
    gnss_set_callback(ui_gnss, &ui);

    ui.scanlist = ui_scanlist(ui.window, UI_SCANLIST_PAGE_5_GHZ);

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
ui_restore(void)
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

    if(conf_get_window_maximized())
        gtk_window_maximize(GTK_WINDOW(ui.window));

    if(conf_get_interface_dark_mode())
        g_signal_emit_by_name(ui.b_mode, "clicked");

    if(conf_get_interface_autosave())
        g_signal_emit_by_name(ui.b_autosave, "clicked");

    if(conf_get_interface_sound())
        g_signal_emit_by_name(ui.b_sound, "clicked");

    if(conf_get_interface_gnss())
        g_signal_emit_by_name(ui.b_gnss, "clicked");

    if(conf_get_interface_geoloc())
        g_signal_emit_by_name(ui.b_geoloc, "clicked");

    ui_view_set_columns_order(ui.treeview, conf_get_preferences_view_cols_order());
    ui_view_set_columns_hidden(ui.treeview, conf_get_preferences_view_cols_hidden());
}

static gboolean
ui_delete_event(GtkWidget *widget,
                GdkEvent  *event,
                gpointer   data)
{
    GdkWindow *window;
    gboolean really_quit;
    gboolean maximized;
    gint x, y;

    really_quit = ui_can_discard_unsaved();
    if(really_quit)
    {
        window = gtk_widget_get_window(GTK_WIDGET(widget));
        maximized = window && (gdk_window_get_state(window) & GDK_WINDOW_STATE_MAXIMIZED);
        if(!maximized)
        {
            gtk_window_get_position(GTK_WINDOW(ui.window), &x, &y);
            conf_set_window_xy(x, y);
            gtk_window_get_size(GTK_WINDOW(ui.window), &x, &y);
            conf_set_window_position(x, y);
        }
        conf_set_window_maximized(maximized);
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
    ui_dialog_open_or_merge_t *ret;

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
        if(count > 1)
            ret = ui_dialog_ask_merge(GTK_WINDOW(ui.window), count);
        else
            ret = ui_dialog_ask_open_or_merge(GTK_WINDOW(ui.window));

        if(ret)
        {
            ui_log_open(filenames, ret->value, ret->strip_signals);
            g_free(ret);
        }

        g_slist_free_full(filenames, g_free);
    }
}

static gboolean
ui_idle_timeout(gpointer user_data)
{
    mtscan_gtk_t *ui = (mtscan_gtk_t*)user_data;
    gint64 ts = UNIX_TIMESTAMP();
    mtscan_gnss_state_t state;

    if(conf_get_interface_sound() &&
       conf_get_preferences_sounds_no_data() &&
       ui->active &&
       (ts - ui->activity_ts) >= ACTIVITY_TIMEOUT &&
       (ts % 2) == 0)
    {
        mtscan_sound(APP_SOUND_CONNECTION_LOST);
    }

    state = gnss_get_data(NULL);
    if(conf_get_interface_sound() &&
       conf_get_preferences_sounds_no_gnss_data() &&
       state > GNSS_OFF &&
       state < GNSS_OK &&
       (ts % 2) != 0)
    {
        mtscan_sound(APP_SOUND_GNSS_LOST);
    }

    return G_SOURCE_CONTINUE;
}

static gboolean
ui_idle_timeout_autosave(gpointer user_data)
{
    mtscan_gtk_t *ui = (mtscan_gtk_t*)user_data;
    gint64 ts;
    gchar *filename;

    if(conf_get_interface_autosave() &&
       ui->changed &&
       ui->active)
    {
        ts = UNIX_TIMESTAMP();
        if((ts - ui->log_ts) >= conf_get_preferences_autosave_interval()*60)
        {
            filename = (!ui->filename ? timestamp_to_filename(conf_get_path_autosave(), ui->log_ts) : NULL);
            if(!ui_log_save_full((!filename ? ui->filename : filename), FALSE, FALSE, FALSE, NULL, FALSE))
            {
                g_signal_emit_by_name(ui->b_autosave, "clicked");

                ui_dialog(GTK_WINDOW(ui->window),
                          GTK_MESSAGE_ERROR,
                          "Error",
                          "Unable to save a file:\n%s\n\n<b>Autosave has been disabled.</b>",
                          (!filename ? ui->filename : filename));
            }
            g_free(filename);
        }
    }

    return G_SOURCE_CONTINUE;
}

static void
ui_gnss(mtscan_gnss_state_t       state,
        const mtscan_gnss_data_t *gnss_data,
        gpointer                  user_data)
{
    mtscan_gtk_t *ui = (mtscan_gtk_t*)user_data;
    GString *string;
    gchar *text;

    switch(state)
    {
        case GNSS_OFF:
            gtk_label_set_text(GTK_LABEL(ui->l_gnss_status), "GNSS: off");
            break;
        case GNSS_OPENING:
            gtk_label_set_markup(GTK_LABEL(ui->l_gnss_status), "GNSS: <span color=\"red\"><b>opening…</b></span>");
            break;
        case GNSS_ACCESS_DENIED:
            gtk_label_set_markup(GTK_LABEL(ui->l_gnss_status), "GNSS: <span color=\"red\"><b>access denied</b></span>");
            break;
        case GNSS_AWAITING:
            gtk_label_set_markup(GTK_LABEL(ui->l_gnss_status), "GNSS: <span color=\"red\"><b>awaiting…</b></span>");
            break;
        case GNSS_NO_FIX:
            gtk_label_set_markup(GTK_LABEL(ui->l_gnss_status), "GNSS: <span color=\"red\"><b>no fix</b></span>");
            break;
        case GNSS_OK:
            string = g_string_new("GNSS: ");
            g_string_append_printf(string, " %c%.5f° %c%.5f°",
                                   (gnss_data->lat >= 0.0 ? 'N' : 'S'),
                                   gnss_data->lat,
                                   (gnss_data->lon >= 0.0 ? 'E' : 'W'),
                                   gnss_data->lon);

            if(conf_get_preferences_gnss_show_errors() &&
               !isnan(gnss_data->epx) && !isnan(gnss_data->epy))
            {
                if(fabs(gnss_data->epx - gnss_data->epy) < 10e-7)
                {
                    g_string_append_printf(string, " (±%.0fm)",
                                           round(gnss_data->epy));
                }
                else
                {
                    g_string_append_printf(string, " (±%.0f/%.0fm)",
                                           round(gnss_data->epy),
                                           round(gnss_data->epx));
                }
            }

            if(conf_get_preferences_gnss_show_altitude() &&
               !isnan(gnss_data->alt))
            {

                g_string_append_printf(string, " %.0fm",
                                       round(gnss_data->alt));

                if(conf_get_preferences_gnss_show_errors() &&
                   !isnan(gnss_data->epv))
                {
                    g_string_append_printf(string, " (±%.0fm)",
                                           round(gnss_data->epv));
                }
            }

            text = g_string_free(string, FALSE);
            gtk_label_set_text(GTK_LABEL(ui->l_gnss_status), text);
            g_free(text);
    }
}

void
ui_connected(const gchar *name,
             const gchar *login,
             const gchar *host,
             const gchar *iface,
             gint64       hwaddr,
             gint         band,
             gint         channel_width)
{
    gchar *status_text;

    ui.connected = TRUE;
    ui.active = FALSE;
    ui.hwaddr = hwaddr;
    ui.band = band;
    ui.channel_width = channel_width;

    ui_toolbar_connect_set_state(TRUE);

    if(name && *name)
        status_text = g_strdup_printf("%s", name);
    else if(conf_get_preferences_compact_status())
        status_text = g_strdup_printf("%s", host);
    else
        status_text = g_strdup_printf("%s@%s/%s", login, host, iface);

    gtk_label_set_text(GTK_LABEL(ui.l_conn_status), status_text);
    g_free(status_text);

    ui_tzsp();
}

void
ui_disconnected(void)
{
    ui.connected = FALSE;
    ui.mode = MTSCAN_MODE_NONE;
    ui.active = FALSE;
    ui.conn = NULL;
    ui.hwaddr = -1;
    ui.band = MT_SSH_BAND_UNKNOWN;
    ui.channel_width = 0;

    ui_toolbar_connect_set_state(FALSE);
    ui_toolbar_scan_set_state(FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(ui.b_connect), TRUE);
    gtk_widget_set_sensitive(GTK_WIDGET(ui.b_scan), TRUE);
    gtk_widget_set_sensitive(GTK_WIDGET(ui.b_restart), TRUE);
    gtk_label_set_text(GTK_LABEL(ui.l_conn_status), "disconnected");

    mtscan_model_buffer_clear(ui.model);
    mtscan_model_clear_active(ui.model);
    ui_status_update_networks();

    ui_tzsp_destroy();
}

void
ui_changed(void)
{
    if(!ui.changed)
    {
        ui.changed = TRUE;
        ui.log_ts = UNIX_TIMESTAMP();
        ui_set_title(ui.filename);
    }
}

gboolean
ui_can_discard_unsaved(void)
{
    if(!ui.changed)
        return TRUE;

    switch(ui_dialog_ask_unsaved(GTK_WINDOW(ui.window)))
    {
    case UI_DIALOG_YES:
        g_signal_emit_by_name(ui.b_save, "clicked", NULL);
        return !ui.changed;
    case UI_DIALOG_NO:
        return TRUE;
    default:
        return FALSE;
    }
}

void
ui_status_update_networks(void)
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
        if(conf_get_preferences_compact_status())
            text = g_strdup_printf("%d/%d", active, networks);
        else
            text = g_strdup_printf("%d/%d networks", active, networks);

        gtk_label_set_text(GTK_LABEL(ui.l_net_status), text);
        g_free(text);
        last_active = active;
        last_networks = networks;
    }
}

void
ui_set_title(const gchar *filename)
{
    gchar *title;
    gchar *tmp = g_strdup(filename);

    g_free(ui.filename);
    ui.filename = tmp;

    g_free(ui.name);
    ui.name = (tmp ? ui_get_name(tmp) : NULL);

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
ui_clear(void)
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
        ui_dialog(GTK_WINDOW(ui.window), GTK_MESSAGE_ERROR, "Error", "%s", err->message);
        g_error_free(err);
    }
#endif
}

void
ui_screenshot(void)
{
    gint width, height;
    time_t tt = time(NULL);
    gchar *filename;
    gchar *path;
    gchar t[20];
    gint i;

    strftime(t, sizeof(t), "%Y%m%d-%H%M%S", localtime(&tt));

    for(i=0; TRUE; i++)
    {
        if(i == 0)
            filename = g_strdup_printf("mtscan-%s.png", t);
        else
            filename = g_strdup_printf("mtscan-%s_%d.png", t, i);

        path = g_build_filename(conf_get_path_screenshot(), filename, NULL);

        if(!g_file_test(path, (G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR)))
            break;

        g_free(filename);
        g_free(path);
    }

#if GTK_CHECK_VERSION (3, 0, 0)
    cairo_surface_t *surface;
    cairo_t *cr;
    width = gtk_widget_get_allocated_width(ui.window);
    height = gtk_widget_get_allocated_height(ui.window);
    surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                                         width,
                                         height);
    cr = cairo_create(surface);
    gtk_widget_draw(ui.window, cr);
    cairo_destroy(cr);
    if(cairo_surface_write_to_png(surface, path) != CAIRO_STATUS_SUCCESS)
    {
        ui_dialog(GTK_WINDOW(ui.window),
                  GTK_MESSAGE_ERROR,
                  "Screenshot",
                  "Unable to save a screenshot to:\n%s",
                  path);
    }
    cairo_surface_destroy(surface);
#else
    GdkPixmap *pixmap;
    GdkPixbuf *pixbuf;
    GError *err = NULL;

    gtk_widget_queue_draw(ui.window);
    gdk_window_process_all_updates();

    pixmap = gtk_widget_get_snapshot(ui.window, NULL);
    gdk_pixmap_get_size(pixmap, &width, &height);
    pixbuf = gdk_pixbuf_get_from_drawable(NULL, pixmap, NULL, 0, 0, 0, 0, width, height);
    if(!gdk_pixbuf_save(pixbuf, path, "png", &err, NULL))
    {
        ui_dialog(GTK_WINDOW(ui.window),
                  GTK_MESSAGE_ERROR,
                  "Screenshot",
                  "%s",
                  err->message);
        g_error_free(err);
    }
    g_object_unref(G_OBJECT(pixmap));
    g_object_unref(G_OBJECT(pixbuf));
#endif
    g_free(path);
    g_free(filename);
}

void
ui_toggle_connection(gint auto_connect_profile)
{
    if(ui.conn)
    {
        /* Close connection */
        ui_toolbar_connect_set_state(TRUE);
        gtk_widget_set_sensitive(GTK_WIDGET(ui.b_connect), FALSE);

        mt_ssh_cancel(ui.conn);
        return;
    }

    /* Display connection dialog */
    ui_toolbar_connect_set_state(FALSE);
    ui.conn_dialog = ui_connection_new(auto_connect_profile,
                                       (auto_connect_profile > 0) ? UI_CONNECTION_MODE_AUTOCONNECT : UI_CONNECTION_MODE_NONE);
}

void
ui_tzsp(void)
{
    gint frequency_base;
    guint8 tzsp_hwaddr[6];

    if(ui.mode != MTSCAN_MODE_SNIFFER)
        return;

    /* Destroy current tzsp-receiver, if exists */
    ui_tzsp_destroy();

    /* Make sure that sensor address is available */
    if(!addr_to_guint8(ui.hwaddr, tzsp_hwaddr))
    {
        ui_dialog(NULL, GTK_MESSAGE_WARNING, "Error", "<b>Failed to create tzsp-receiver</b>\n\nSensor address is unavailable.");
        return;
    }

    if(ui.band == MTSCAN_BAND_2GHZ)
        frequency_base = 2407;
    else if(ui.band == MTSCAN_BAND_5GHZ)
        frequency_base = 5000;
    else
    {
        ui_dialog(NULL, GTK_MESSAGE_WARNING, "Error", "<b>Failed to create tzsp-receiver</b>\n\nFrequency band is unknown.");
        return;
    }

    ui.tzsp_rx = tzsp_receiver_new((guint16)conf_get_preferences_tzsp_udp_port(),
                                   tzsp_hwaddr,
                                   ui.channel_width,
                                   frequency_base,
                                   ui_callback_tzsp,
                                   ui_callback_tzsp_network);

    if(!ui.tzsp_rx)
        ui_dialog(NULL, GTK_MESSAGE_WARNING, "Error", "<b>Failed to enable tzsp-receiver.</b>");
}

void
ui_tzsp_destroy(void)
{
    if(ui.tzsp_rx)
    {
        tzsp_receiver_cancel(ui.tzsp_rx);
        ui.tzsp_rx = NULL;
    }
}
