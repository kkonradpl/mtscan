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

#include <gdk/gdkkeysyms.h>
#include <string.h>
#include "conf.h"
#include "ui.h"
#include "ui-view.h"
#include "ui-dialogs.h"
#include "ui-log.h"
#include "log.h"
#include "mt-ssh.h"
#include "ui-scanlist.h"
#include "ui-scanlist-manager.h"
#include "ui-connection.h"
#include "ui-toolbar.h"
#include "ui-preferences.h"
#include "export.h"
#include "gps.h"
#include "conf-scanlist.h"
#include "ui-callbacks.h"

static void ui_toolbar_connect(GtkWidget*, gpointer);
static void ui_toolbar_scan(GtkWidget*, gpointer);
static void ui_toolbar_restart(GtkWidget*, gpointer);

static void ui_toolbar_scanlist_apply(const gchar*, const gchar*);
static void ui_toolbar_scanlist_default(GtkWidget*, gpointer);
static void ui_toolbar_scanlist(GtkWidget*, gpointer);
static void ui_toolbar_scanlist_preset(GtkWidget *, gpointer);
static void ui_toolbar_scanlist_menu(GtkWidget*, gpointer);
static gboolean ui_toolbar_scanlist_menu_foreach(GtkTreeModel*, GtkTreePath*, GtkTreeIter*, gpointer);
static void ui_toolbar_scanlist_menu_manager(GtkWidget*, gpointer);
static void ui_toolbar_scanlist_menu_preset(GtkWidget*, gpointer);

static void ui_toolbar_new(GtkWidget*, gpointer);
static void ui_toolbar_open(GtkWidget*, gpointer);
static void ui_toolbar_merge(GtkWidget*, gpointer);
static void ui_toolbar_save(GtkWidget*, gpointer);
static void ui_toolbar_save_as(GtkWidget*, gpointer);
static void ui_toolbar_export(GtkWidget*, gpointer);
static void ui_toolbar_screenshot(GtkWidget*, gpointer);

static void ui_toolbar_preferences(GtkWidget*, gpointer);
static void ui_toolbar_sound(GtkWidget*, gpointer);
static void ui_toolbar_mode(GtkWidget*, gpointer);
static void ui_toolbar_autosave(GtkWidget*, gpointer);
static void ui_toolbar_gps(GtkWidget*, gpointer);
static void ui_toolbar_geoloc(GtkWidget*, gpointer);
static void ui_toolbar_about(GtkWidget*, gpointer);

/* ToggleToolButton's 'clicked' callback contain
   some hacks, as the "toggled" signal does not
   work with gtk_widget_add_accelerator. */

GtkWidget*
ui_toolbar_create(void)
{
    GtkAccelGroup *accel_group;
    GtkWidget *toolbar;

    accel_group = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(ui.window), accel_group);

    toolbar = gtk_toolbar_new();
    gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);

    ui.b_connect = gtk_toggle_tool_button_new();
    gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(ui.b_connect), gtk_image_new_from_stock(GTK_STOCK_CONNECT, GTK_ICON_SIZE_BUTTON));
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(ui.b_connect), "Connect");
    gtk_widget_set_tooltip_text(GTK_WIDGET(ui.b_connect), "Connect (Ctrl+1)");
    g_signal_connect(ui.b_connect, "clicked", G_CALLBACK(ui_toolbar_connect), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), ui.b_connect, -1);
    gtk_widget_add_accelerator(GTK_WIDGET(ui.b_connect), "clicked", accel_group, GDK_KEY_1, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    ui.b_scan = gtk_toggle_tool_button_new();
    gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(ui.b_scan), gtk_image_new_from_stock(GTK_STOCK_MEDIA_PLAY, GTK_ICON_SIZE_BUTTON));
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(ui.b_scan), "Scan");
    gtk_widget_set_tooltip_text(GTK_WIDGET(ui.b_scan), "Scan (Ctrl+2)");
    g_signal_connect(ui.b_scan, "clicked", G_CALLBACK(ui_toolbar_scan), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), ui.b_scan, -1);
    gtk_widget_add_accelerator(GTK_WIDGET(ui.b_scan), "clicked", accel_group, GDK_KEY_2, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    ui.b_restart = gtk_tool_button_new(gtk_image_new_from_stock(GTK_STOCK_REFRESH, GTK_ICON_SIZE_BUTTON), "Restart");
    gtk_widget_set_tooltip_text(GTK_WIDGET(ui.b_restart), "Restart (Ctrl+3)");
    g_signal_connect(ui.b_restart, "clicked", G_CALLBACK(ui_toolbar_restart), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), ui.b_restart, -1);
    gtk_widget_add_accelerator(GTK_WIDGET(ui.b_restart), "clicked", accel_group, GDK_KEY_3, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    ui.b_scanlist_default = gtk_tool_button_new(gtk_image_new_from_stock(GTK_STOCK_HOME, GTK_ICON_SIZE_BUTTON), "Default scan-list");
    gtk_widget_set_tooltip_text(GTK_WIDGET(ui.b_scanlist_default), "Default scan-list (Ctrl+4)");
    g_signal_connect(ui.b_scanlist_default, "clicked", G_CALLBACK(ui_toolbar_scanlist_default), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), ui.b_scanlist_default, -1);
    gtk_widget_add_accelerator(GTK_WIDGET(ui.b_scanlist_default), "clicked", accel_group, GDK_KEY_4, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    ui.b_scanlist = gtk_tool_button_new(gtk_image_new_from_stock(GTK_STOCK_EDIT, GTK_ICON_SIZE_BUTTON), "Scan-list editor");
    gtk_widget_set_tooltip_text(GTK_WIDGET(ui.b_scanlist), "Scan-list (Ctrl+L)");
    g_signal_connect(ui.b_scanlist, "clicked", G_CALLBACK(ui_toolbar_scanlist), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), ui.b_scanlist, -1);
    gtk_widget_add_accelerator(GTK_WIDGET(ui.b_scanlist), "clicked", accel_group, GDK_KEY_l, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    ui.b_scanlist_preset = gtk_menu_tool_button_new(gtk_image_new_from_stock(GTK_STOCK_SELECT_ALL, GTK_ICON_SIZE_BUTTON), "Preset scan-list");
    gtk_widget_set_tooltip_text(GTK_WIDGET(ui.b_scanlist_preset), "Preset scan-list");
    g_signal_connect(ui.b_scanlist_preset, "clicked", G_CALLBACK(ui_toolbar_scanlist_preset), NULL);
    gtk_menu_tool_button_set_menu(GTK_MENU_TOOL_BUTTON(ui.b_scanlist_preset), gtk_menu_new());
    g_signal_connect(ui.b_scanlist_preset, "show-menu", G_CALLBACK(ui_toolbar_scanlist_menu), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), ui.b_scanlist_preset, -1);

    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);

    ui.b_new = gtk_tool_button_new(gtk_image_new_from_stock(GTK_STOCK_NEW, GTK_ICON_SIZE_BUTTON), "New");
    gtk_widget_set_tooltip_text(GTK_WIDGET(ui.b_new), "New (Ctrl+N)");
    g_signal_connect(ui.b_new, "clicked", G_CALLBACK(ui_toolbar_new), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), ui.b_new, -1);
    gtk_widget_add_accelerator(GTK_WIDGET(ui.b_new), "clicked", accel_group, GDK_KEY_n, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    ui.b_open = gtk_tool_button_new(gtk_image_new_from_stock(GTK_STOCK_OPEN, GTK_ICON_SIZE_BUTTON), "Open");
    gtk_widget_set_tooltip_text(GTK_WIDGET(ui.b_open), "Open (Ctrl+O)");
    g_signal_connect(ui.b_open, "clicked", G_CALLBACK(ui_toolbar_open), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), ui.b_open, -1);
    gtk_widget_add_accelerator(GTK_WIDGET(ui.b_open), "clicked", accel_group, GDK_KEY_o, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    ui.b_merge = gtk_tool_button_new(gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_BUTTON), "Merge");
    gtk_widget_set_tooltip_text(GTK_WIDGET(ui.b_merge), "Merge (Ctrl+M)");
    g_signal_connect(ui.b_merge, "clicked", G_CALLBACK(ui_toolbar_merge), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), ui.b_merge, -1);
    gtk_widget_add_accelerator(GTK_WIDGET(ui.b_merge), "clicked", accel_group, GDK_KEY_m, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    ui.b_save = gtk_tool_button_new(gtk_image_new_from_stock(GTK_STOCK_SAVE, GTK_ICON_SIZE_BUTTON), "Save");
    gtk_widget_set_tooltip_text(GTK_WIDGET(ui.b_save), "Save (Ctrl+S)");
    g_signal_connect(ui.b_save, "clicked", G_CALLBACK(ui_toolbar_save), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), ui.b_save, -1);
    gtk_widget_add_accelerator(GTK_WIDGET(ui.b_save), "clicked", accel_group, GDK_KEY_s, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    ui.b_save_as = gtk_tool_button_new(gtk_image_new_from_stock(GTK_STOCK_SAVE_AS, GTK_ICON_SIZE_BUTTON), "Save as");
    gtk_widget_set_tooltip_text(GTK_WIDGET(ui.b_save_as), "Save as");
    g_signal_connect(ui.b_save_as, "clicked", G_CALLBACK(ui_toolbar_save_as), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), ui.b_save_as, -1);

    ui.b_export = gtk_tool_button_new(gtk_image_new_from_icon_name("mtscan-export", GTK_ICON_SIZE_BUTTON), "Export");
    gtk_widget_set_tooltip_text(GTK_WIDGET(ui.b_export), "Export as HTML (Ctrl+E)");
    gtk_tool_button_set_use_underline(GTK_TOOL_BUTTON(ui.b_export), TRUE);
    g_signal_connect(ui.b_export, "clicked", G_CALLBACK(ui_toolbar_export), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), ui.b_export, -1);
    gtk_widget_add_accelerator(GTK_WIDGET(ui.b_export), "clicked", accel_group, GDK_KEY_e, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    ui.b_screenshot = gtk_tool_button_new(gtk_image_new_from_icon_name("mtscan-screen", GTK_ICON_SIZE_BUTTON), "Screenshot");
    gtk_widget_set_tooltip_text(GTK_WIDGET(ui.b_screenshot), "Take a screenshot (Ctrl+Z)");
    gtk_tool_button_set_use_underline(GTK_TOOL_BUTTON(ui.b_screenshot), TRUE);
    g_signal_connect(ui.b_screenshot, "clicked", G_CALLBACK(ui_toolbar_screenshot), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), ui.b_screenshot, -1);
    gtk_widget_add_accelerator(GTK_WIDGET(ui.b_screenshot), "clicked", accel_group, GDK_KEY_z, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);

    ui.b_preferences = gtk_tool_button_new(gtk_image_new_from_stock(GTK_STOCK_PREFERENCES, GTK_ICON_SIZE_BUTTON), "Preferences");
    gtk_widget_set_tooltip_text(GTK_WIDGET(ui.b_preferences), "Preferences (CTRL+P)");
    g_signal_connect(ui.b_preferences, "clicked", G_CALLBACK(ui_toolbar_preferences), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), ui.b_preferences, -1);
    gtk_widget_add_accelerator(GTK_WIDGET(ui.b_preferences), "clicked", accel_group, GDK_KEY_p, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    ui.b_sound = gtk_toggle_tool_button_new();
    gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(ui.b_sound), gtk_image_new_from_icon_name("audio-volume-high", GTK_ICON_SIZE_BUTTON));
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(ui.b_sound), "Enable sound");
    gtk_widget_set_tooltip_text(GTK_WIDGET(ui.b_sound), "Enable sound");
    g_signal_connect(ui.b_sound, "clicked", G_CALLBACK(ui_toolbar_sound), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), ui.b_sound, -1);

    ui.b_mode = gtk_toggle_tool_button_new();
    gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(ui.b_mode), gtk_image_new_from_icon_name("mtscan-dark", GTK_ICON_SIZE_BUTTON));
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(ui.b_mode), "Dark mode");
    gtk_widget_set_tooltip_text(GTK_WIDGET(ui.b_mode), "Dark mode");
    g_signal_connect(ui.b_mode, "clicked", G_CALLBACK(ui_toolbar_mode), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), ui.b_mode, -1);

    ui.b_autosave = gtk_toggle_tool_button_new();
    gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(ui.b_autosave), gtk_image_new_from_stock(GTK_STOCK_FLOPPY, GTK_ICON_SIZE_BUTTON));
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(ui.b_autosave), "Enable autosave");
    gtk_widget_set_tooltip_text(GTK_WIDGET(ui.b_autosave), "Enable autosave");
    g_signal_connect(ui.b_autosave, "clicked", G_CALLBACK(ui_toolbar_autosave), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), ui.b_autosave, -1);

    ui.b_gps = gtk_toggle_tool_button_new();
    gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(ui.b_gps), gtk_image_new_from_icon_name("mtscan-gps", GTK_ICON_SIZE_BUTTON));
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(ui.b_gps), "Enable GPS");
    gtk_widget_set_tooltip_text(GTK_WIDGET(ui.b_gps), "Enable GPS");
    g_signal_connect(ui.b_gps, "clicked", G_CALLBACK(ui_toolbar_gps), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), ui.b_gps, -1);

    ui.b_geoloc = gtk_toggle_tool_button_new();
    gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(ui.b_geoloc), gtk_image_new_from_icon_name("mtscan-geoloc", GTK_ICON_SIZE_BUTTON));
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(ui.b_geoloc), "Enable geolocation");
    gtk_widget_set_tooltip_text(GTK_WIDGET(ui.b_geoloc), "Enable geolocation");
    g_signal_connect(ui.b_geoloc, "clicked", G_CALLBACK(ui_toolbar_geoloc), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), ui.b_geoloc, -1);

    ui.b_about = gtk_tool_button_new(gtk_image_new_from_stock(GTK_STOCK_ABOUT, GTK_ICON_SIZE_BUTTON), "About");
    gtk_widget_set_tooltip_text(GTK_WIDGET(ui.b_about), "About " APP_NAME);
    g_signal_connect(ui.b_about, "clicked", G_CALLBACK(ui_toolbar_about), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), ui.b_about, -1);

    return toolbar;
}

static void
ui_toolbar_connect(GtkWidget *widget,
                   gpointer   data)
{
    ui_toggle_connection(-1);
}

void
ui_toolbar_connect_set_state(gboolean state)
{
    g_signal_handlers_block_by_func(G_OBJECT(ui.b_connect), GINT_TO_POINTER(ui_toolbar_connect), NULL);
    gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(ui.b_connect), state);
    g_signal_handlers_unblock_by_func(G_OBJECT(ui.b_connect), GINT_TO_POINTER(ui_toolbar_connect), NULL);
}

static void
ui_toolbar_scan(GtkWidget *widget,
                gpointer   data)
{
    ui_toolbar_scan_set_state(ui.active);
    if(ui.conn)
    {
        mt_ssh_cmd(ui.conn, MT_SSH_CMD_STOP, NULL);
        if(!ui.active && ui.mode == MTSCAN_MODE_SCANNER)
            mt_ssh_cmd(ui.conn, MT_SSH_CMD_SCAN, NULL);
        else if(!ui.active && ui.mode == MTSCAN_MODE_SNIFFER)
            mt_ssh_cmd(ui.conn, MT_SSH_CMD_SNIFF, NULL);
        gtk_widget_set_sensitive(widget, FALSE);
    }
}

void
ui_toolbar_scan_set_state(gboolean active)
{
    g_signal_handlers_block_by_func(G_OBJECT(ui.b_scan), GINT_TO_POINTER(ui_toolbar_scan), NULL);
    gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(ui.b_scan), active);
    g_signal_handlers_unblock_by_func(G_OBJECT(ui.b_scan), GINT_TO_POINTER(ui_toolbar_scan), NULL);
}

static void
ui_toolbar_restart(GtkWidget *widget,
                   gpointer   data)
{
    if(ui.conn)
    {
        mt_ssh_cmd(ui.conn, MT_SSH_CMD_STOP, NULL);
        if(ui.mode == MTSCAN_MODE_SCANNER)
            mt_ssh_cmd(ui.conn, MT_SSH_CMD_SCAN, NULL);
        else if(ui.mode == MTSCAN_MODE_SNIFFER)
            mt_ssh_cmd(ui.conn, MT_SSH_CMD_SNIFF, NULL);
        gtk_widget_set_sensitive(widget, FALSE);
    }
}

static void
ui_toolbar_scanlist_apply(const gchar *name,
                          const gchar *value)
{
    static gboolean scanlist_set = FALSE;

    if(conf_get_runtime_skip_scanlist_warning())
        scanlist_set = TRUE;

    if(!scanlist_set)
        scanlist_set = ui_dialog_scanlist_warn(GTK_WINDOW(ui.window), name, value);

    if(scanlist_set && ui.conn)
        mt_ssh_cmd(ui.conn, MT_SSH_CMD_SCANLIST, value);
}

static void
ui_toolbar_scanlist_default(GtkWidget *widget,
                            gpointer   data)
{
    GtkListStore *model = conf_get_scanlists();
    conf_scanlist_t *sl;

    sl = conf_scanlist_find_default(model);
    if(sl)
    {
        ui_toolbar_scanlist_apply(conf_scanlist_get_name(sl),
                                  conf_scanlist_get_data(sl));
        conf_scanlist_free(sl);
        return;
    }

    if(!ui.conn)
        return;

    mt_ssh_cmd(ui.conn, MT_SSH_CMD_SCANLIST, "default");
}

static void
ui_toolbar_scanlist(GtkWidget *widget,
                    gpointer   data)
{
    ui_scanlist_show(ui.scanlist);
}

static void
ui_toolbar_scanlist_preset(GtkWidget *widget,
                           gpointer data)
{
    GtkListStore *model = conf_get_scanlists();
    conf_scanlist_t *sl;

    sl = conf_scanlist_find_main(model);
    if(sl)
    {
        ui_toolbar_scanlist_apply(conf_scanlist_get_name(sl),
                                  conf_scanlist_get_data(sl));
        conf_scanlist_free(sl);
        return;
    }

    ui_scanlist_manager(ui.window, conf_get_scanlists());
}

static void
ui_toolbar_scanlist_menu(GtkWidget *widget,
                         gpointer   data)
{
    GtkListStore *model = conf_get_scanlists();
    GtkWidget *menu;
    GtkWidget *item;
    gint rows;

    menu = gtk_menu_new();
    item = gtk_image_menu_item_new_with_label("Scan-list manager");
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), gtk_image_new_from_stock(GTK_STOCK_PREFERENCES, GTK_ICON_SIZE_MENU));
    g_signal_connect(item, "activate", G_CALLBACK(ui_toolbar_scanlist_menu_manager), 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    rows = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(model), NULL);

    if(!rows)
    {
        item = gtk_image_menu_item_new_with_label("(no saved scanlists)");
        gtk_widget_set_sensitive(GTK_WIDGET(item), FALSE);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    }
    else
    {
        gtk_tree_model_foreach(GTK_TREE_MODEL(model), ui_toolbar_scanlist_menu_foreach, menu);
    }

    gtk_widget_show_all(menu);
    gtk_menu_tool_button_set_menu(GTK_MENU_TOOL_BUTTON(widget), menu);
}

static gboolean
ui_toolbar_scanlist_menu_foreach(GtkTreeModel *store,
                                 GtkTreePath  *path,
                                 GtkTreeIter  *iter,
                                 gpointer      data)
{
    GtkWidget *menu = GTK_WIDGET(data);
    conf_scanlist_t *sl;
    const gchar *name;
    const gchar *value;
    GtkWidget *item;
    GtkWidget *label;
    gchar *text;
    gchar *tooltip;

    sl = conf_scanlist_list_get(GTK_LIST_STORE(store), iter);
    name = conf_scanlist_get_name(sl);
    value = conf_scanlist_get_data(sl);

    item = gtk_image_menu_item_new();
    g_object_set_data_full(G_OBJECT(item), "mtscan_scanlist_name", (gpointer)g_strdup(name), g_free);
    g_object_set_data_full(G_OBJECT(item), "mtscan_scanlist_value", (gpointer)g_strdup(value), g_free);

    if(strlen(value) > 50)
    {
        text = g_utf8_substring(value, 0, 50);
        tooltip = g_strdup_printf("%s…", value);
        gtk_widget_set_tooltip_text(item, tooltip);
        g_free(tooltip);
        g_free(text);
    }
    else
    {
        gtk_widget_set_tooltip_text(item, value);
    }

    label = gtk_label_new(NULL);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_label_set_text(GTK_LABEL(label), name);

    if(conf_scanlist_get_main(sl))
        gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), gtk_image_new_from_stock(GTK_STOCK_SELECT_ALL, GTK_ICON_SIZE_MENU));
    else if(conf_scanlist_get_default(sl))
        gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), gtk_image_new_from_stock(GTK_STOCK_HOME, GTK_ICON_SIZE_MENU));

    gtk_container_add(GTK_CONTAINER(item), label);
    gtk_widget_show(label);

    g_signal_connect(item, "activate", G_CALLBACK(ui_toolbar_scanlist_menu_preset), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    conf_scanlist_free(sl);
    return FALSE;
}

static void
ui_toolbar_scanlist_menu_manager(GtkWidget *widget,
                                 gpointer   data)
{
    ui_scanlist_manager(ui.window, conf_get_scanlists());
}

static void
ui_toolbar_scanlist_menu_preset(GtkWidget *widget,
                                gpointer   data)
{
    const gchar *name = (const gchar*)g_object_get_data(G_OBJECT(widget), "mtscan_scanlist_name");
    const gchar *value = (const gchar*)g_object_get_data(G_OBJECT(widget), "mtscan_scanlist_value");

    if(name && value)
        ui_toolbar_scanlist_apply(name, value);
}

static void
ui_toolbar_new(GtkWidget *widget,
               gpointer   data)
{
    if(ui_can_discard_unsaved())
    {
        ui_clear();
        ui_set_title(NULL);
    }
}

static void
ui_toolbar_open(GtkWidget *widget,
                gpointer   data)
{
    ui_dialog_open_t *o = ui_dialog_open(GTK_WINDOW(ui.window), UI_DIALOG_OPEN, TRUE);
    if(o)
    {
        ui_log_open(o->filenames, FALSE, o->strip_signals);
        g_slist_free_full(o->filenames, g_free);
        g_free(o);
    }
}

static void
ui_toolbar_merge(GtkWidget *widget,
                 gpointer   data)
{
    ui_dialog_open_t *o = ui_dialog_open(GTK_WINDOW(ui.window), UI_DIALOG_MERGE, TRUE);
    if(o)
    {
        ui_log_open(o->filenames, TRUE, o->strip_signals);
        g_slist_free_full(o->filenames, g_free);
        g_free(o);
    }
}

static void
ui_toolbar_save(GtkWidget *widget,
                gpointer   data)
{
    if(!ui.filename)
    {
        ui_toolbar_save_as(widget, data);
        return;
    }

    ui_log_save_full(ui.filename, FALSE, FALSE, FALSE, NULL, TRUE);
}

static void
ui_toolbar_save_as(GtkWidget *widget,
                   gpointer   data)
{
    ui_dialog_save_t *s = ui_dialog_save(GTK_WINDOW(ui.window));
    if(s)
    {
        ui_log_save_full(s->filename, s->strip_signals, s->strip_gps, s->strip_azi, NULL, TRUE);
        g_free(s->filename);
        g_free(s);
    }
}

static void
ui_toolbar_export(GtkWidget *widget,
                  gpointer   data)
{
    gchar *filename = ui_dialog_export(GTK_WINDOW(ui.window));
    if(filename)
    {
        if(!export_html(filename,
                        ui.name,
                        ui.model,
                        conf_get_preferences_view_cols_order(),
                        conf_get_preferences_view_cols_hidden()))

        {
            ui_dialog(GTK_WINDOW(ui.window),
                      GTK_MESSAGE_ERROR,
                      "Error",
                      "Unable to export the log to a file:\n%s",
                      filename);
        }
        g_free(filename);
    }
}

static void
ui_toolbar_screenshot(GtkWidget *widget,
                      gpointer   data)
{
    ui_screenshot();
}

static void
ui_toolbar_preferences(GtkWidget *widget,
                       gpointer   data)
{
    ui_preferences_dialog();
}

static void
ui_toolbar_sound(GtkWidget *widget,
                 gpointer   data)
{
    static gboolean pressed = FALSE;
    pressed = !pressed;
    conf_set_interface_sound(pressed);

    g_signal_handlers_block_by_func(G_OBJECT(widget), GINT_TO_POINTER(ui_toolbar_sound), NULL);
    gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(widget), pressed);
    g_signal_handlers_unblock_by_func(G_OBJECT(widget), GINT_TO_POINTER(ui_toolbar_sound), NULL);
}

static void
ui_toolbar_mode(GtkWidget *widget,
                gpointer   data)
{
    static gboolean pressed = FALSE;
    pressed = !pressed;
    ui_view_dark_mode(ui.treeview, pressed);
    conf_set_interface_dark_mode(pressed);

    g_signal_handlers_block_by_func(G_OBJECT(widget), GINT_TO_POINTER(ui_toolbar_mode), NULL);
    gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(widget), pressed);
    g_signal_handlers_unblock_by_func(G_OBJECT(widget), GINT_TO_POINTER(ui_toolbar_mode), NULL);
}

static void
ui_toolbar_gps(GtkWidget *widget,
               gpointer   data)
{
    static gboolean pressed = FALSE;
    pressed = !pressed;
    conf_set_interface_gps(pressed);

    if(pressed)
        gps_start(conf_get_preferences_gps_hostname(), conf_get_preferences_gps_tcp_port());
    else
        gps_stop();

    g_signal_handlers_block_by_func(G_OBJECT(widget), GINT_TO_POINTER(ui_toolbar_gps), NULL);
    gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(widget), pressed);
    g_signal_handlers_unblock_by_func(G_OBJECT(widget), GINT_TO_POINTER(ui_toolbar_gps), NULL);
}

static void
ui_toolbar_geoloc(GtkWidget *widget,
                  gpointer   data)
{
    static gboolean pressed = FALSE;
    pressed = !pressed;
    conf_set_interface_geoloc(pressed);

    if(pressed)
    {
        geoloc_init(conf_get_preferences_location_mtscan_data(),
                    &ui_callback_geoloc);
        geoloc_wigle(conf_get_preferences_location_wigle_api_url(),
                     conf_get_preferences_location_wigle_api_key());
    }

    g_signal_handlers_block_by_func(G_OBJECT(widget), GINT_TO_POINTER(ui_toolbar_geoloc), NULL);
    gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(widget), pressed);
    g_signal_handlers_unblock_by_func(G_OBJECT(widget), GINT_TO_POINTER(ui_toolbar_geoloc), NULL);
}

static void
ui_toolbar_autosave(GtkWidget *widget,
                    gpointer   data)
{
    static gboolean pressed = FALSE;
    pressed = !pressed;
    conf_set_interface_autosave(pressed);

    g_signal_handlers_block_by_func(G_OBJECT(widget), GINT_TO_POINTER(ui_toolbar_autosave), NULL);
    gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(widget), pressed);
    g_signal_handlers_unblock_by_func(G_OBJECT(widget), GINT_TO_POINTER(ui_toolbar_autosave), NULL);
}

static void
ui_toolbar_about(GtkWidget *widget,
                 gpointer   data)
{
    ui_dialog_about(GTK_WINDOW(ui.window));
}
