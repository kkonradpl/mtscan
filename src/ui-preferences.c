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

#include <gdk/gdkkeysyms.h>
#include <string.h>
#include "conf.h"
#include "ui.h"
#include "ui-view.h"
#include "gps.h"
#include "ui-dialogs.h"
#include "misc.h"
#include "ui-callbacks.h"

enum
{
    VIEW_MODEL_NAME,
    VIEW_MODEL_TITLE,
    VIEW_MODEL_SHOW,
    VIEW_MODEL_COLS
};

typedef struct ui_preferences_list
{
    GtkWidget *page;
    GtkWidget *box;
    GtkWidget *box_buttons;
    GtkWidget *x_enabled;
    GtkWidget *x_inverted;
    GtkWidget *view;
    GtkWidget *scroll;
    GtkWidget *b_add;
    GtkWidget *b_remove;
    GtkWidget *b_clear;
    GtkWidget *x_external;
    GtkWidget *c_ext_path;
} ui_preferences_list_t;

typedef struct ui_preferences
{
    GtkWidget *window;
    GtkWidget *content;
    GtkWidget *notebook;

    GtkWidget *page_general;
    GtkWidget *table_general;
    GtkWidget *l_general_icon_size;
    GtkWidget *s_general_icon_size;
    GtkWidget *l_general_icon_size_unit;
    GtkWidget *l_general_autosave_interval;
    GtkWidget *s_general_autosave_interval;
    GtkWidget *l_general_autosave_interval_unit;
    GtkWidget *l_general_autosave_directory;
    GtkWidget *c_general_autosave_directory;
    GtkWidget *l_general_screenshot_directory;
    GtkWidget *c_general_screenshot_directory;
    GtkWidget *l_general_search_column;
    GtkWidget *c_general_search_column;
    GtkWidget *l_general_fallback_encoding;
    GtkWidget *e_general_fallback_encoding;
    GtkWidget *x_general_no_style_override;
    GtkWidget *x_general_signals;
    GtkWidget *x_general_display_time_only;
    GtkWidget *x_general_compact_status;
    GtkWidget *x_general_reconnect;

    GtkWidget *page_view;
    GtkWidget *v_view;
    GtkWidget *s_view;

    GtkWidget *page_sounds;
    GtkWidget *table_sounds;
    GtkWidget *x_sounds_new_network;
    GtkWidget *x_sounds_new_network_hi;
    GtkWidget *x_sounds_new_network_al;
    GtkWidget *x_sounds_no_data;
    GtkWidget *x_sounds_no_gps_data;

    GtkWidget *page_events;
    GtkWidget *table_events;
    GtkWidget *x_events_new_network;
    GtkWidget *l_events_new_network;
    GtkWidget *c_events_new_network;

    GtkWidget *page_tzsp;
    GtkWidget *table_tzsp;
    GtkWidget *l_tzsp_mode;
    GtkWidget *box_tzsp_mode;
    GtkWidget *r_tzsp_mode_socket;
    GtkWidget *r_tzsp_mode_pcap;
    GtkWidget *l_tzsp_udp_port;
    GtkWidget *s_tzsp_udp_port;
    GtkWidget *box_tzsp_interface;
    GtkWidget *l_tzsp_interface;
    GtkWidget *e_tzsp_interface;
    GtkWidget *b_tzsp_interface;
    GtkWidget *l_tzsp_channel_width;
    GtkWidget *s_tzsp_channel_width;
    GtkWidget *l_tzsp_channel_width_unit;
    GtkWidget *l_tzsp_band;
    GtkWidget *box_tzsp_band;
    GtkWidget *r_tzsp_band_2g;
    GtkWidget *r_tzsp_band_5g;
    GtkWidget *box_tzsp_info;
    GtkWidget *i_tzsp_info;
    GtkWidget *l_tzsp_info;

    GtkWidget *page_gps;
    GtkWidget *table_gps;
    GtkWidget *l_gps_hostname;
    GtkWidget *e_gps_hostname;
    GtkWidget *l_gps_tcp_port;
    GtkWidget *s_gps_tcp_port;
    GtkWidget *x_gps_show_altitude;
    GtkWidget *x_gps_show_errors;

    ui_preferences_list_t blacklist;
    ui_preferences_list_t highlightlist;
    ui_preferences_list_t alarmlist;

    GtkWidget *page_location;
    GtkWidget *table_location;
    GtkWidget *l_location_latitude;
    GtkWidget *s_location_latitude;
    GtkWidget *l_location_longitude;
    GtkWidget *s_location_longitude;
    GtkWidget *b_location_acquire;
    GtkWidget *x_location_mtscan;
    GtkWidget *v_location_mtscan;
    GtkWidget *s_location_mtscan;
    GtkWidget *b_location_buttons;
    GtkWidget *b_location_add;
    GtkWidget *b_location_remove;
    GtkWidget *b_location_clear;
    GtkWidget *x_location_wigle;
    GtkWidget *l_location_wigle_api_url;
    GtkWidget *e_location_wigle_api_url;
    GtkWidget *l_location_wigle_api_key;
    GtkWidget *e_location_wigle_api_key;
    GtkWidget *l_location_azimuth_error;
    GtkWidget *s_location_azimuth_error;
    GtkWidget *l_location_min_distance;
    GtkWidget *s_location_min_distance;
    GtkWidget *l_location_max_distance;
    GtkWidget *s_location_max_distance;

    GtkWidget *box_button;
    GtkWidget *b_apply;
    GtkWidget *b_cancel;
} ui_preferences_t;

static void ui_preferences_view_toggled(GtkCellRendererToggle*, gchar*, gpointer);

static void ui_preferences_list_create(ui_preferences_list_t*, GtkWidget*, const gchar*, const gchar*, const gchar*);
static void ui_preferences_list_format(GtkTreeViewColumn*, GtkCellRenderer*, GtkTreeModel*, GtkTreeIter*, gpointer);

static gboolean ui_preferences_key(GtkWidget*, GdkEventKey*, gpointer);
static gboolean ui_preferences_key_list(GtkWidget*, GdkEventKey*, gpointer);
static gboolean ui_preferences_key_list_add(GtkWidget*, GdkEventKey*, gpointer);
static void ui_preferences_list_add(GtkWidget*, gpointer);
static void ui_preferences_list_remove(GtkWidget*, gpointer);
static void ui_preferences_list_clear(GtkWidget*, gpointer);
static void ui_preferences_list_ext_toggled(GtkWidget*, gpointer);
static void ui_preferences_file_add(GtkWidget*, gpointer);
static void ui_preferences_location_acquire(GtkWidget*, gpointer);
static void ui_preferences_load(ui_preferences_t*);
static void ui_preferences_load_view(ui_preferences_t*, const gchar* const*, const gchar* const*);
static void ui_preferences_apply(GtkWidget*, gpointer);
static void ui_preferences_apply_view(ui_preferences_t*);

void
ui_preferences_dialog(void)
{
    static ui_preferences_t p;
    guint row;
    GtkTreeViewColumn *column;
    GtkCellRenderer *renderer;

    p.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_modal(GTK_WINDOW(p.window), TRUE);
    gtk_window_set_resizable(GTK_WINDOW(p.window), FALSE);
    gtk_window_set_title(GTK_WINDOW(p.window), "Preferences");
    gtk_window_set_destroy_with_parent(GTK_WINDOW(p.window), TRUE);
    gtk_container_set_border_width(GTK_CONTAINER(p.window), 2);
    gtk_window_set_transient_for(GTK_WINDOW(p.window), GTK_WINDOW(ui.window));
    gtk_window_set_position(GTK_WINDOW(p.window), GTK_WIN_POS_CENTER_ON_PARENT);

    p.content = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(p.window), p.content);

    p.notebook = gtk_notebook_new();
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(p.notebook), GTK_POS_LEFT);
    gtk_notebook_popup_enable(GTK_NOTEBOOK(p.notebook));
    gtk_container_add(GTK_CONTAINER(p.content), p.notebook);

    /* General */
    p.page_general = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(p.page_general), 4);
    gtk_notebook_append_page(GTK_NOTEBOOK(p.notebook), p.page_general, gtk_label_new("General"));
    gtk_container_child_set(GTK_CONTAINER(p.notebook), p.page_general, "tab-expand", FALSE, "tab-fill", FALSE, NULL);

    p.table_general = gtk_table_new(11, 3, TRUE);
    gtk_table_set_homogeneous(GTK_TABLE(p.table_general), FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(p.table_general), 4);
    gtk_table_set_col_spacings(GTK_TABLE(p.table_general), 4);
    gtk_container_add(GTK_CONTAINER(p.page_general), p.table_general);

    row = 0;
    p.l_general_icon_size = gtk_label_new("Icon size:");
    gtk_misc_set_alignment(GTK_MISC(p.l_general_icon_size), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(p.table_general), p.l_general_icon_size, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    p.s_general_icon_size = gtk_spin_button_new(GTK_ADJUSTMENT(gtk_adjustment_new(0.0, 16.0, 64.0, 1.0, 10.0, 0.0)), 0, 0);
    gtk_table_attach(GTK_TABLE(p.table_general), p.s_general_icon_size, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    p.l_general_icon_size_unit = gtk_label_new("px");
    gtk_table_attach(GTK_TABLE(p.table_general), p.l_general_icon_size_unit, 2, 3, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    p.l_general_autosave_interval = gtk_label_new("Autosave interval:");
    gtk_misc_set_alignment(GTK_MISC(p.l_general_autosave_interval), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(p.table_general), p.l_general_autosave_interval, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    p.s_general_autosave_interval = gtk_spin_button_new(GTK_ADJUSTMENT(gtk_adjustment_new(0.0, 1.0, 60.0, 1.0, 5.0, 0.0)), 0, 0);
    gtk_table_attach(GTK_TABLE(p.table_general), p.s_general_autosave_interval, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    p.l_general_autosave_interval_unit = gtk_label_new("min");
    gtk_table_attach(GTK_TABLE(p.table_general), p.l_general_autosave_interval_unit, 2, 3, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    p.l_general_autosave_directory = gtk_label_new("Autosave path:");
    gtk_misc_set_alignment(GTK_MISC(p.l_general_autosave_directory), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(p.table_general), p.l_general_autosave_directory, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    p.c_general_autosave_directory = gtk_file_chooser_button_new("Autosave", GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
    gtk_table_attach(GTK_TABLE(p.table_general), p.c_general_autosave_directory, 1, 3, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    p.l_general_screenshot_directory = gtk_label_new("Screenshot path:");
    gtk_misc_set_alignment(GTK_MISC(p.l_general_screenshot_directory), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(p.table_general), p.l_general_screenshot_directory, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    p.c_general_screenshot_directory = gtk_file_chooser_button_new("Screenshots", GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
    gtk_table_attach(GTK_TABLE(p.table_general), p.c_general_screenshot_directory, 1, 3, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    p.l_general_search_column = gtk_label_new("Search column:");
    gtk_misc_set_alignment(GTK_MISC(p.l_general_search_column), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(p.table_general), p.l_general_search_column, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    p.c_general_search_column = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(p.c_general_search_column), "Address");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(p.c_general_search_column), "SSID");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(p.c_general_search_column), "Radio name");
    gtk_table_attach(GTK_TABLE(p.table_general), p.c_general_search_column, 1, 3, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    p.l_general_fallback_encoding = gtk_label_new("Fallback encoding:");
    gtk_misc_set_alignment(GTK_MISC(p.l_general_fallback_encoding), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(p.table_general), p.l_general_fallback_encoding, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    p.e_general_fallback_encoding = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(p.table_general), p.e_general_fallback_encoding, 1, 3, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    p.x_general_no_style_override = gtk_check_button_new_with_label("Do not override styles from system theme");
    gtk_table_attach(GTK_TABLE(p.table_general), p.x_general_no_style_override, 0, 3, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    p.x_general_signals = gtk_check_button_new_with_label("Record all signal samples");
    gtk_table_attach(GTK_TABLE(p.table_general), p.x_general_signals, 0, 3, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    p.x_general_display_time_only = gtk_check_button_new_with_label("Display time only");
    gtk_table_attach(GTK_TABLE(p.table_general), p.x_general_display_time_only, 0, 3, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    p.x_general_compact_status = gtk_check_button_new_with_label("Compact statusbar information");
    gtk_table_attach(GTK_TABLE(p.table_general), p.x_general_compact_status, 0, 3, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    p.x_general_reconnect = gtk_check_button_new_with_label("Automatic reconnect");
    gtk_table_attach(GTK_TABLE(p.table_general), p.x_general_reconnect, 0, 3, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    /* View */
    p.page_view = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(p.page_view), 4);
    gtk_notebook_append_page(GTK_NOTEBOOK(p.notebook), p.page_view, gtk_label_new("View"));
    gtk_container_child_set(GTK_CONTAINER(p.notebook), p.page_view, "tab-expand", FALSE, "tab-fill", FALSE, NULL);

    p.v_view = gtk_tree_view_new();
    gtk_tree_view_set_reorderable(GTK_TREE_VIEW(p.v_view), TRUE);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Column name", renderer, "text", VIEW_MODEL_NAME, NULL);
    gtk_tree_view_column_set_expand(column, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(p.v_view), column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Title", renderer, "text", VIEW_MODEL_TITLE, NULL);
    gtk_tree_view_column_set_expand(column, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(p.v_view), column);

    renderer = gtk_cell_renderer_toggle_new();
    g_signal_connect(renderer, "toggled", G_CALLBACK(ui_preferences_view_toggled), p.v_view);
    column = gtk_tree_view_column_new_with_attributes("", renderer, "active", VIEW_MODEL_SHOW, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(p.v_view), column);

    p.s_view = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(p.s_view), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(p.s_view), p.v_view);
    gtk_widget_set_size_request(p.s_view, -1, 150);
    gtk_container_add(GTK_CONTAINER(p.page_view), p.s_view);


    /* Sounds */
    p.page_sounds = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(p.page_sounds), 4);
    gtk_notebook_append_page(GTK_NOTEBOOK(p.notebook), p.page_sounds, gtk_label_new("Sounds"));
    gtk_container_child_set(GTK_CONTAINER(p.notebook), p.page_sounds, "tab-expand", FALSE, "tab-fill", FALSE, NULL);

    p.table_sounds = gtk_table_new(5, 1, TRUE);
    gtk_table_set_homogeneous(GTK_TABLE(p.table_sounds), FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(p.table_sounds), 4);
    gtk_table_set_col_spacings(GTK_TABLE(p.table_sounds), 4);
    gtk_container_add(GTK_CONTAINER(p.page_sounds), p.table_sounds);

    row = 0;
    p.x_sounds_new_network = gtk_check_button_new_with_label("New network");
    gtk_table_attach(GTK_TABLE(p.table_sounds), p.x_sounds_new_network, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    p.x_sounds_new_network_hi = gtk_check_button_new_with_label("New network from highlight list");
    gtk_table_attach(GTK_TABLE(p.table_sounds), p.x_sounds_new_network_hi, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    p.x_sounds_new_network_al = gtk_check_button_new_with_label("New network from alarm list");
    gtk_table_attach(GTK_TABLE(p.table_sounds), p.x_sounds_new_network_al, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    p.x_sounds_no_data = gtk_check_button_new_with_label("No data");
    gtk_table_attach(GTK_TABLE(p.table_sounds), p.x_sounds_no_data, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    p.x_sounds_no_gps_data = gtk_check_button_new_with_label("No GPS data");
    gtk_table_attach(GTK_TABLE(p.table_sounds), p.x_sounds_no_gps_data, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);


    /* Events */
    p.page_events = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(p.page_events), 4);
    gtk_notebook_append_page(GTK_NOTEBOOK(p.notebook), p.page_events, gtk_label_new("Events"));
    gtk_container_child_set(GTK_CONTAINER(p.notebook), p.page_events, "tab-expand", FALSE, "tab-fill", FALSE, NULL);

    p.table_events = gtk_table_new(2, 2, TRUE);
    gtk_table_set_homogeneous(GTK_TABLE(p.table_events), FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(p.table_events), 4);
    gtk_table_set_col_spacings(GTK_TABLE(p.table_events), 4);
    gtk_container_add(GTK_CONTAINER(p.page_events), p.table_events);

    row = 0;
    p.x_events_new_network = gtk_check_button_new_with_label("New network event");
    gtk_table_attach(GTK_TABLE(p.table_events), p.x_events_new_network, 0, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    p.l_events_new_network = gtk_label_new("Execute:");
    gtk_misc_set_alignment(GTK_MISC(p.l_events_new_network), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(p.table_events), p.l_events_new_network, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    p.c_events_new_network = gtk_file_chooser_button_new("New network event", GTK_FILE_CHOOSER_ACTION_OPEN);
    gtk_table_attach(GTK_TABLE(p.table_events), p.c_events_new_network, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);


    /* TZSP */
    p.page_tzsp = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(p.page_tzsp), 4);
    gtk_notebook_append_page(GTK_NOTEBOOK(p.notebook), p.page_tzsp, gtk_label_new("TZSP"));
    gtk_container_child_set(GTK_CONTAINER(p.notebook), p.page_tzsp, "tab-expand", FALSE, "tab-fill", FALSE, NULL);

    p.table_tzsp = gtk_table_new(5, 3, TRUE);
    gtk_table_set_homogeneous(GTK_TABLE(p.table_tzsp), FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(p.table_tzsp), 4);
    gtk_table_set_col_spacings(GTK_TABLE(p.table_tzsp), 4);
    gtk_box_pack_start(GTK_BOX(p.page_tzsp), p.table_tzsp, TRUE, TRUE, 1);

    row = 0;
    p.l_tzsp_udp_port = gtk_label_new("UDP port:");
    gtk_misc_set_alignment(GTK_MISC(p.l_tzsp_udp_port), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(p.table_tzsp), p.l_tzsp_udp_port, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    p.s_tzsp_udp_port = gtk_spin_button_new(GTK_ADJUSTMENT(gtk_adjustment_new(0.0, 1024.0, 65535.0, 1.0, 10.0, 0.0)), 0, 0);
    gtk_table_attach(GTK_TABLE(p.table_tzsp), p.s_tzsp_udp_port, 1, 3, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    p.l_tzsp_channel_width = gtk_label_new("Channel width:");
    gtk_misc_set_alignment(GTK_MISC(p.l_tzsp_channel_width), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(p.table_tzsp), p.l_tzsp_channel_width, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    p.s_tzsp_channel_width = gtk_spin_button_new(GTK_ADJUSTMENT(gtk_adjustment_new(0.0, 5.0, 80.0, 5.0, 10.0, 0.0)), 0, 0);
    gtk_table_attach(GTK_TABLE(p.table_tzsp), p.s_tzsp_channel_width, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    p.l_tzsp_channel_width_unit = gtk_label_new("MHz");
    gtk_table_attach(GTK_TABLE(p.table_tzsp), p.l_tzsp_channel_width_unit, 2, 3, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    p.l_tzsp_band = gtk_label_new("Frequency band:");
    gtk_misc_set_alignment(GTK_MISC(p.l_tzsp_band), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(p.table_tzsp), p.l_tzsp_band, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    p.box_tzsp_band = gtk_hbox_new(FALSE, 6);
    p.r_tzsp_band_2g = gtk_radio_button_new_with_label(NULL, "2.4 GHz");
    gtk_box_pack_start(GTK_BOX(p.box_tzsp_band), p.r_tzsp_band_2g, FALSE, FALSE, 0);
    p.r_tzsp_band_5g = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(p.r_tzsp_band_2g), "5 GHz");
    gtk_box_pack_start(GTK_BOX(p.box_tzsp_band), p.r_tzsp_band_5g, FALSE, FALSE, 0);
    gtk_table_attach(GTK_TABLE(p.table_tzsp), p.box_tzsp_band, 1, 3, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    p.box_tzsp_info = gtk_hbox_new(FALSE, 5);
    p.i_tzsp_info = gtk_image_new_from_icon_name("dialog-warning", GTK_ICON_SIZE_MENU);
    gtk_box_pack_start(GTK_BOX(p.box_tzsp_info), p.i_tzsp_info, FALSE, FALSE, 1);
    p.l_tzsp_info = gtk_label_new(NULL);
    gtk_label_set_line_wrap(GTK_LABEL(p.l_tzsp_info), TRUE);
    gtk_label_set_markup(GTK_LABEL(p.l_tzsp_info), "<b>RouterOS v6.41 or later is required.\nMake sure to enable streaming\nand set server to your IP address\nin Wireless Sniffer Settings.</b>");
    gtk_box_pack_start(GTK_BOX(p.box_tzsp_info), p.l_tzsp_info, FALSE, FALSE, 1);
    gtk_box_pack_start(GTK_BOX(p.page_tzsp), p.box_tzsp_info, FALSE, FALSE, 1);

    /* GPS */
    p.page_gps = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(p.page_gps), 4);
    gtk_notebook_append_page(GTK_NOTEBOOK(p.notebook), p.page_gps, gtk_label_new("GPSd"));
    gtk_container_child_set(GTK_CONTAINER(p.notebook), p.page_gps, "tab-expand", FALSE, "tab-fill", FALSE, NULL);

    p.table_gps = gtk_table_new(4, 2, TRUE);
    gtk_table_set_homogeneous(GTK_TABLE(p.table_gps), FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(p.table_gps), 4);
    gtk_table_set_col_spacings(GTK_TABLE(p.table_gps), 4);
    gtk_container_add(GTK_CONTAINER(p.page_gps), p.table_gps);

    row = 0;
    p.l_gps_hostname = gtk_label_new("Hostname:");
    gtk_misc_set_alignment(GTK_MISC(p.l_gps_hostname), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(p.table_gps), p.l_gps_hostname, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    p.e_gps_hostname = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(p.table_gps), p.e_gps_hostname, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    p.l_gps_tcp_port = gtk_label_new("TCP port:");
    gtk_misc_set_alignment(GTK_MISC(p.l_gps_tcp_port), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(p.table_gps), p.l_gps_tcp_port, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    p.s_gps_tcp_port = gtk_spin_button_new(GTK_ADJUSTMENT(gtk_adjustment_new(2947.0, 1024.0, 65535.0, 1.0, 10.0, 0.0)), 0, 0);
    gtk_table_attach(GTK_TABLE(p.table_gps), p.s_gps_tcp_port, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    p.x_gps_show_altitude = gtk_check_button_new_with_label("Show altitude");
    gtk_table_attach(GTK_TABLE(p.table_gps), p.x_gps_show_altitude, 0, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    p.x_gps_show_errors = gtk_check_button_new_with_label("Show error estimates");
    gtk_table_attach(GTK_TABLE(p.table_gps), p.x_gps_show_errors, 0, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);


    /* Blacklist */
    ui_preferences_list_create(&p.blacklist, p.notebook, "Blacklist", "Enable blacklist", "Invert to whitelist");

    /* Highlight list */
    ui_preferences_list_create(&p.highlightlist, p.notebook, "Highlight", "Enable highlight list", "Invert highlight list");

    /* Alarm list */
    ui_preferences_list_create(&p.alarmlist, p.notebook, "Alarm", "Enable alarm list", NULL);

    /* Location */
    p.page_location = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(p.page_location), 4);
    gtk_notebook_append_page(GTK_NOTEBOOK(p.notebook), p.page_location, gtk_label_new("Location"));
    gtk_container_child_set(GTK_CONTAINER(p.notebook), p.page_location, "tab-expand", FALSE, "tab-fill", FALSE, NULL);

    p.table_location = gtk_table_new(12, 3, TRUE);
    gtk_table_set_homogeneous(GTK_TABLE(p.table_location), FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(p.table_location), 4);
    gtk_table_set_col_spacings(GTK_TABLE(p.table_location), 4);
    gtk_container_add(GTK_CONTAINER(p.page_location), p.table_location);

    row = 0;
    p.l_location_latitude = gtk_label_new("Latitude:");
    gtk_misc_set_alignment(GTK_MISC(p.l_location_latitude), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(p.table_location), p.l_location_latitude, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    p.s_location_latitude = gtk_spin_button_new(GTK_ADJUSTMENT(gtk_adjustment_new(0.0, -90.0, 90.0, 0.1, 1.0, 0.0)), 0, 6);
    gtk_table_attach(GTK_TABLE(p.table_location), p.s_location_latitude, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    p.l_location_longitude = gtk_label_new("Longitude:");
    gtk_misc_set_alignment(GTK_MISC(p.l_location_longitude), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(p.table_location), p.l_location_longitude, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    p.s_location_longitude = gtk_spin_button_new(GTK_ADJUSTMENT(gtk_adjustment_new(0.0, -180.0, 180.0, 0.1, 1.0, 0.0)), 0, 6);
    gtk_table_attach(GTK_TABLE(p.table_location), p.s_location_longitude, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    p.b_location_acquire = gtk_button_new_with_label("Acquire");
    gtk_table_attach(GTK_TABLE(p.table_location), p.b_location_acquire, 2, 3, row-1, row+1, GTK_EXPAND|GTK_FILL, GTK_EXPAND|GTK_FILL, 0, 0);
    g_signal_connect(p.b_location_acquire, "clicked", G_CALLBACK(ui_preferences_location_acquire), &p);

    row++;
    p.x_location_mtscan = gtk_check_button_new_with_label("Use MTscan logs for geolocation");
    gtk_table_attach(GTK_TABLE(p.table_location), p.x_location_mtscan, 0, 3, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    p.b_location_buttons = gtk_hbox_new(TRUE, 0);
    p.b_location_add = gtk_button_new_from_stock(GTK_STOCK_ADD);
    gtk_box_pack_start(GTK_BOX(p.b_location_buttons), p.b_location_add, TRUE, TRUE, 0);
    p.b_location_remove = gtk_button_new_from_stock(GTK_STOCK_REMOVE);
    gtk_box_pack_start(GTK_BOX(p.b_location_buttons), p.b_location_remove, TRUE, TRUE, 0);
    p.b_location_clear = gtk_button_new_from_stock(GTK_STOCK_CLEAR);
    gtk_box_pack_start(GTK_BOX(p.b_location_buttons), p.b_location_clear, TRUE, TRUE, 0);
    gtk_table_attach(GTK_TABLE(p.table_location), p.b_location_buttons, 0, 3, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    p.v_location_mtscan = gtk_tree_view_new();
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(p.v_location_mtscan), FALSE);
    gtk_tree_view_set_reorderable(GTK_TREE_VIEW(p.v_location_mtscan), TRUE);
    renderer = gtk_cell_renderer_text_new();
    gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT(renderer), 1);
    column = gtk_tree_view_column_new_with_attributes("Path", renderer, "text", 0, NULL);
    g_object_set(renderer, "ellipsize", PANGO_ELLIPSIZE_START, "ellipsize-set", TRUE, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(p.v_location_mtscan), column);
    g_signal_connect(p.b_location_add, "clicked", G_CALLBACK(ui_preferences_file_add), p.v_location_mtscan);
    g_signal_connect(p.b_location_remove, "clicked", G_CALLBACK(ui_preferences_list_remove), p.v_location_mtscan);
    g_signal_connect(p.b_location_clear, "clicked", G_CALLBACK(ui_preferences_list_clear), p.v_location_mtscan);
    g_signal_connect(p.v_location_mtscan, "key-press-event", G_CALLBACK(ui_preferences_key_list), p.b_location_remove);

    p.s_location_mtscan = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(p.s_location_mtscan), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
    gtk_container_add(GTK_CONTAINER(p.s_location_mtscan), p.v_location_mtscan);
    gtk_widget_set_size_request(p.s_location_mtscan, -1, 100);
    gtk_table_attach(GTK_TABLE(p.table_location), p.s_location_mtscan, 0, 3, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    p.x_location_wigle = gtk_check_button_new_with_label("Use WiGLE for geolocation");
    gtk_table_attach(GTK_TABLE(p.table_location), p.x_location_wigle, 0, 3, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    p.l_location_wigle_api_url = gtk_label_new("WiGLE API URL:");
    gtk_misc_set_alignment(GTK_MISC(p.l_location_wigle_api_url), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(p.table_location), p.l_location_wigle_api_url, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    p.e_location_wigle_api_url = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(p.table_location), p.e_location_wigle_api_url, 1, 3, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    p.l_location_wigle_api_key = gtk_label_new("WiGLE API key:");
    gtk_misc_set_alignment(GTK_MISC(p.l_location_wigle_api_key), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(p.table_location), p.l_location_wigle_api_key, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    p.e_location_wigle_api_key = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(p.table_location), p.e_location_wigle_api_key, 1, 3, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    p.l_location_azimuth_error = gtk_label_new("Azimuth error ± [°]:");
    gtk_misc_set_alignment(GTK_MISC(p.l_location_azimuth_error), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(p.table_location), p.l_location_azimuth_error, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    p.s_location_azimuth_error = gtk_spin_button_new(GTK_ADJUSTMENT(gtk_adjustment_new(0.0, 0, 180.0, 1.0, 1.0, 0.0)), 0, 0);
    gtk_table_attach(GTK_TABLE(p.table_location), p.s_location_azimuth_error, 1, 3, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    p.l_location_min_distance = gtk_label_new("Min distance [km]:");
    gtk_misc_set_alignment(GTK_MISC(p.l_location_min_distance), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(p.table_location), p.l_location_min_distance, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    p.s_location_min_distance = gtk_spin_button_new(GTK_ADJUSTMENT(gtk_adjustment_new(0.0, 0, 9999.0, 1.0, 1.0, 0.0)), 0, 0);
    gtk_table_attach(GTK_TABLE(p.table_location), p.s_location_min_distance, 1, 3, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    p.l_location_max_distance = gtk_label_new("Max distance [km]:");
    gtk_misc_set_alignment(GTK_MISC(p.l_location_max_distance), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(p.table_location), p.l_location_max_distance, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    p.s_location_max_distance = gtk_spin_button_new(GTK_ADJUSTMENT(gtk_adjustment_new(0.0, 0, 9999.0, 1.0, 1.0, 0.0)), 0, 0);
    gtk_table_attach(GTK_TABLE(p.table_location), p.s_location_max_distance, 1, 3, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    /* --- */
    p.box_button = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(p.box_button), GTK_BUTTONBOX_END);
    gtk_box_set_spacing(GTK_BOX(p.box_button), 5);
    gtk_box_pack_start(GTK_BOX(p.content), p.box_button, FALSE, FALSE, 5);

    p.b_apply = gtk_button_new_from_stock(GTK_STOCK_APPLY);
    g_signal_connect(p.b_apply, "clicked", G_CALLBACK(ui_preferences_apply), &p);
    gtk_container_add(GTK_CONTAINER(p.box_button), p.b_apply);

    p.b_cancel = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
    g_signal_connect_swapped(p.b_cancel, "clicked", G_CALLBACK(gtk_widget_destroy), p.window);
    gtk_container_add(GTK_CONTAINER(p.box_button), p.b_cancel);

    g_signal_connect(p.window, "key-press-event", G_CALLBACK(ui_preferences_key), NULL);
    ui_preferences_load(&p);
    gtk_widget_show_all(p.window);
}

static void
ui_preferences_view_toggled(GtkCellRendererToggle *cell,
                            gchar                 *path_str,
                            gpointer               data)
{
    GtkTreeView *view = GTK_TREE_VIEW(data);
    GtkTreeModel *model = gtk_tree_view_get_model(view);
    GtkTreePath *path = gtk_tree_path_new_from_string(path_str);
    GtkTreeIter iter;
    gboolean state;

    gtk_tree_model_get_iter(model, &iter, path);
    gtk_tree_model_get(model, &iter, VIEW_MODEL_SHOW, &state, -1);
    gtk_list_store_set(GTK_LIST_STORE(model), &iter, VIEW_MODEL_SHOW, !state, -1);
    gtk_tree_path_free(path);
}

static void
ui_preferences_list_create(ui_preferences_list_t *l,
                           GtkWidget             *notebook,
                           const gchar           *title,
                           const gchar           *enable_title,
                           const gchar           *invert_title)
{
    GtkTreeViewColumn *column;
    GtkCellRenderer *renderer;
    GtkFileFilter *filter;
    GtkFileFilter *filter_all;


    l->page = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(l->page), 4);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), l->page, gtk_label_new(title));
    gtk_container_child_set(GTK_CONTAINER(notebook), l->page, "tab-expand", FALSE, "tab-fill", FALSE, NULL);

    l->box = gtk_vbox_new(FALSE, 4);
    gtk_container_add(GTK_CONTAINER(l->page), l->box);

    l->x_enabled = gtk_check_button_new_with_label(enable_title);
    gtk_box_pack_start(GTK_BOX(l->box), l->x_enabled, FALSE, FALSE, 0);

    if(invert_title)
    {
        l->x_inverted = gtk_check_button_new_with_label(invert_title);
        gtk_box_pack_start(GTK_BOX(l->box), l->x_inverted, FALSE, FALSE, 0);
    }
    else
    {
        l->x_inverted = NULL;
    }

    l->view = gtk_tree_view_new();
    renderer = gtk_cell_renderer_text_new();
    gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT(renderer), 1);
    column = gtk_tree_view_column_new_with_attributes("Address", renderer, NULL);
    gtk_tree_view_column_set_cell_data_func(column, renderer, ui_preferences_list_format, NULL, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(l->view), column);
    gtk_tree_view_set_enable_search(GTK_TREE_VIEW(l->view), TRUE);
    gtk_tree_view_set_search_column(GTK_TREE_VIEW(l->view), 0);
    gtk_tree_view_set_search_equal_func(GTK_TREE_VIEW(l->view), ui_view_compare_address, NULL, NULL);

    l->scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(l->scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(l->scroll), l->view);
    gtk_widget_set_size_request(l->scroll, -1, 200);
    gtk_box_pack_start(GTK_BOX(l->box), l->scroll, TRUE, TRUE, 0);

    l->box_buttons = gtk_hbox_new(TRUE, 0);
    gtk_box_pack_start(GTK_BOX(l->box), l->box_buttons, FALSE, FALSE, 0);

    l->b_add = gtk_button_new_from_stock(GTK_STOCK_ADD);
    g_signal_connect(l->b_add, "clicked", G_CALLBACK(ui_preferences_list_add), l->view);
    gtk_box_pack_start(GTK_BOX(l->box_buttons), l->b_add, TRUE, TRUE, 0);

    l->b_remove = gtk_button_new_from_stock(GTK_STOCK_REMOVE);
    g_signal_connect(l->b_remove, "clicked", G_CALLBACK(ui_preferences_list_remove), l->view);
    gtk_box_pack_start(GTK_BOX(l->box_buttons), l->b_remove, TRUE, TRUE, 0);
    g_signal_connect(l->view, "key-press-event", G_CALLBACK(ui_preferences_key_list), l->b_remove);

    l->b_clear = gtk_button_new_from_stock(GTK_STOCK_CLEAR);
    g_signal_connect(l->b_clear, "clicked", G_CALLBACK(ui_preferences_list_clear), l->view);
    gtk_box_pack_start(GTK_BOX(l->box_buttons), l->b_clear, TRUE, TRUE, 0);

    l->x_external = gtk_check_button_new_with_label("Load networks from a MTscan log on startup");
    gtk_box_pack_start(GTK_BOX(l->box), l->x_external, FALSE, FALSE, 0);
    g_signal_connect(l->x_external, "toggled", G_CALLBACK(ui_preferences_list_ext_toggled), (gpointer)l);

    l->c_ext_path = gtk_file_chooser_button_new("MTscan log", GTK_FILE_CHOOSER_ACTION_OPEN);
    gtk_box_pack_start(GTK_BOX(l->box), l->c_ext_path, FALSE, FALSE, 0);

    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, APP_NAME " file");
    gtk_file_filter_add_pattern(filter, "*" APP_FILE_EXT);
    gtk_file_filter_add_pattern(filter, "*" APP_FILE_EXT APP_FILE_COMPRESS);
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(l->c_ext_path), filter);

    filter_all = gtk_file_filter_new();
    gtk_file_filter_set_name(filter_all, "All files");
    gtk_file_filter_add_pattern(filter_all, "*");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(l->c_ext_path), filter_all);
}

static void
ui_preferences_list_format(GtkTreeViewColumn *col,
                           GtkCellRenderer   *renderer,
                           GtkTreeModel      *store,
                           GtkTreeIter       *iter,
                           gpointer           data)
{
    gint64 address;
    gtk_tree_model_get(store, iter, 0, &address, -1);
    g_object_set(renderer, "text", model_format_address(address, FALSE), NULL);
}

static gboolean
ui_preferences_key(GtkWidget   *widget,
                   GdkEventKey *event,
                   gpointer     data)
{
    guint current = gdk_keyval_to_upper(event->keyval);
    if(current == GDK_KEY_Escape)
    {
        gtk_widget_destroy(widget);
        return TRUE;
    }
    return FALSE;
}

static gboolean
ui_preferences_key_list(GtkWidget   *widget,
                        GdkEventKey *event,
                        gpointer     data)
{
    GtkButton *b_remove = GTK_BUTTON(data);
    if(event->keyval == GDK_KEY_Delete)
    {
        gtk_button_clicked(b_remove);
        return TRUE;
    }
    return FALSE;
}

static gboolean
ui_preferences_key_list_add(GtkWidget   *widget,
                            GdkEventKey *event,
                            gpointer     data)
{
    GtkDialog *dialog = GTK_DIALOG(data);

    if(event->keyval == GDK_KEY_Return)
    {
        gtk_dialog_response(dialog, GTK_RESPONSE_OK);
        return TRUE;
    }
    return FALSE;
}

static void
ui_preferences_list_add(GtkWidget *widget,
                        gpointer   data)
{
    GtkTreeView *treeview = GTK_TREE_VIEW(data);
    GtkWidget *toplevel = gtk_widget_get_toplevel(widget);
    GtkWidget *dialog;
    GtkWidget *entry;
    const gchar *value;
    GError *err = NULL;
    GMatchInfo *matchInfo;
    GRegex *regex;
    gchar *match = NULL;
    gint64 addr = -1;

    dialog = gtk_message_dialog_new_with_markup(GTK_WINDOW(toplevel),
                                                GTK_DIALOG_MODAL,
                                                GTK_MESSAGE_QUESTION,
                                                GTK_BUTTONS_OK_CANCEL,
                                                "<big>Enter network address</big>\n"
                                                "eg. 01:23:45:67:89:AB\n"
                                                "or 0123456789AB");

    gtk_window_set_title(GTK_WINDOW(dialog), "New network entry");
    entry = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(entry), 17);
    gtk_container_add(GTK_CONTAINER(gtk_message_dialog_get_message_area(GTK_MESSAGE_DIALOG(dialog))), entry);
    g_signal_connect(entry, "key-press-event", G_CALLBACK(ui_preferences_key_list_add), dialog);
    gtk_widget_show_all(dialog);

    if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK)
    {
        value = gtk_entry_get_text(GTK_ENTRY(entry));
        regex = g_regex_new("^([0-9A-Fa-f]{2}:){5}([0-9A-Fa-f]{2})", 0, 0, &err);
        if(regex)
        {
            g_regex_match(regex, value, 0, &matchInfo);
            if(g_match_info_matches(matchInfo))
            {
                match = g_match_info_fetch(matchInfo, 0);
                if(match)
                    remove_char(match, ':');
            }
            g_match_info_free(matchInfo);
            g_regex_unref(regex);
        }

        if(!match)
        {
            regex = g_regex_new("^([0-9A-Fa-f]{12})", 0, 0, &err);
            if(regex)
            {
                g_regex_match(regex, value, 0, &matchInfo);
                if(g_match_info_matches(matchInfo))
                    match = g_match_info_fetch(matchInfo, 0);
                g_match_info_free(matchInfo);
                g_regex_unref(regex);
            }
        }

        if(!match ||
           ((addr = str_addr_to_gint64(match, strlen(match))) < 0))
        {
            ui_dialog(GTK_WINDOW(toplevel),
                      GTK_MESSAGE_ERROR,
                      "New network entry",
                      "<big>Invalid address format:</big>\n%s", value);
        }
        g_free(match);
    }

    gtk_widget_destroy(dialog);

    if(addr >= 0)
        gtk_list_store_insert_with_values(GTK_LIST_STORE(gtk_tree_view_get_model(treeview)), NULL, -1, 0, addr, -1);
}

static void
ui_preferences_list_remove(GtkWidget *widget,
                           gpointer   data)
{
    GtkTreeView *treeview = GTK_TREE_VIEW(data);
    GtkTreeIter iter;
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    selection = gtk_tree_view_get_selection(treeview);
    if(gtk_tree_selection_get_selected(selection, &model, &iter))
        gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
}

static void
ui_preferences_list_clear(GtkWidget *widget,
                          gpointer   data)
{
    GtkTreeView *treeview = GTK_TREE_VIEW(data);
    GtkTreeModel *model = gtk_tree_view_get_model(treeview);
    gtk_list_store_clear(GTK_LIST_STORE(model));
}

static void
ui_preferences_list_ext_toggled(GtkWidget *widget,
                                gpointer   data)
{
    ui_preferences_list_t *list = (ui_preferences_list_t*)data;
    gboolean state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    gtk_widget_set_sensitive(list->b_add, !state);
    gtk_widget_set_sensitive(list->b_remove, !state);
    gtk_widget_set_sensitive(list->b_clear, !state);
    gtk_widget_set_sensitive(list->view, !state);

    gtk_widget_set_sensitive(list->c_ext_path, state);
}

static void
ui_preferences_file_add(GtkWidget *widget,
                        gpointer   data)
{
    GtkTreeView *treeview = GTK_TREE_VIEW(data);
    GtkTreeModel *model = gtk_tree_view_get_model(treeview);
    GtkWidget *toplevel = gtk_widget_get_toplevel(widget);
    ui_dialog_open_t *o = ui_dialog_open(GTK_WINDOW(toplevel), UI_DIALOG_OPEN, FALSE);
    GSList *it;

    if(o)
    {
        for(it = o->filenames; it; it = it->next)
            gtk_list_store_insert_with_values(GTK_LIST_STORE(model), NULL, -1, 0, (gchar*)it->data, -1);
        g_slist_free_full(o->filenames, g_free);
        g_free(o);
    }
}

static void
ui_preferences_location_acquire(GtkWidget *widget,
                                gpointer   data)
{
    ui_preferences_t *p = (ui_preferences_t*)data;
    const mtscan_gps_data_t *gps_data;
    GtkWidget *toplevel = gtk_widget_get_toplevel(widget);

    if(gps_get_data(&gps_data) != GPS_OK)
    {
        ui_dialog(GTK_WINDOW(toplevel),
                  GTK_MESSAGE_WARNING,
                  "GPS position",
                  "Failed to acquire GPS position.\nNo current position fix available.");
    }
    else
    {
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(p->s_location_latitude), gps_data->lat);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(p->s_location_longitude), gps_data->lon);
    }
}

static void
ui_preferences_load(ui_preferences_t *p)
{
    GtkListStore *model;

    /* General */
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(p->s_general_icon_size), conf_get_preferences_icon_size());
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(p->s_general_autosave_interval), conf_get_preferences_autosave_interval());
    gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(p->c_general_autosave_directory), conf_get_path_autosave());
    gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(p->c_general_screenshot_directory), conf_get_path_screenshot());
    gtk_combo_box_set_active(GTK_COMBO_BOX(p->c_general_search_column), conf_get_preferences_search_column());
    gtk_entry_set_text(GTK_ENTRY(p->e_general_fallback_encoding), conf_get_preferences_fallback_encoding());
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->x_general_no_style_override), conf_get_preferences_no_style_override());
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->x_general_signals), conf_get_preferences_signals());
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->x_general_display_time_only), conf_get_preferences_display_time_only());
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->x_general_compact_status), conf_get_preferences_compact_status());
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->x_general_reconnect), conf_get_preferences_reconnect());

    /* View */
    ui_preferences_load_view(p, conf_get_preferences_view_cols_order(), conf_get_preferences_view_cols_hidden());

    /* Sounds */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->x_sounds_new_network), conf_get_preferences_sounds_new_network());
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->x_sounds_new_network_hi), conf_get_preferences_sounds_new_network_hi());
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->x_sounds_new_network_al), conf_get_preferences_sounds_new_network_al());
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->x_sounds_no_data), conf_get_preferences_sounds_no_data());
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->x_sounds_no_gps_data), conf_get_preferences_sounds_no_gps_data());

    /* Events */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->x_events_new_network), conf_get_preferences_events_new_network());
    if(strlen(conf_get_preferences_events_new_network_exec()))
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(p->c_events_new_network), conf_get_preferences_events_new_network_exec());

    /* TZSP */
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(p->s_tzsp_udp_port), conf_get_preferences_tzsp_udp_port());
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(p->s_tzsp_channel_width), conf_get_preferences_tzsp_channel_width());
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->r_tzsp_band_2g), conf_get_preferences_tzsp_band() == MTSCAN_CONF_TZSP_BAND_2GHZ);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->r_tzsp_band_5g), conf_get_preferences_tzsp_band() == MTSCAN_CONF_TZSP_BAND_5GHZ);

    /* GPS */
    gtk_entry_set_text(GTK_ENTRY(p->e_gps_hostname), conf_get_preferences_gps_hostname());
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(p->s_gps_tcp_port), conf_get_preferences_gps_tcp_port());
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->x_gps_show_altitude), conf_get_preferences_gps_show_altitude());
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->x_gps_show_errors), conf_get_preferences_gps_show_errors());

    /* Blacklist */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->blacklist.x_enabled), conf_get_preferences_blacklist_enabled());
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->blacklist.x_inverted), conf_get_preferences_blacklist_inverted());
    model = conf_get_preferences_blacklist_as_liststore();
    gtk_tree_view_set_model(GTK_TREE_VIEW(p->blacklist.view), GTK_TREE_MODEL(model));
    g_object_unref(model);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->blacklist.x_external), conf_get_preferences_blacklist_external());
    if(strlen(conf_get_preferences_blacklist_ext_path()))
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(p->blacklist.c_ext_path), conf_get_preferences_blacklist_ext_path());

    /* Highlight list */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->highlightlist.x_enabled), conf_get_preferences_highlightlist_enabled());
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->highlightlist.x_inverted), conf_get_preferences_highlightlist_inverted());
    model = conf_get_preferences_highlightlist_as_liststore();
    gtk_tree_view_set_model(GTK_TREE_VIEW(p->highlightlist.view), GTK_TREE_MODEL(model));
    g_object_unref(model);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->highlightlist.x_external), conf_get_preferences_highlightlist_external());
    if(strlen(conf_get_preferences_highlightlist_ext_path()))
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(p->highlightlist.c_ext_path), conf_get_preferences_highlightlist_ext_path());

    /* Alarm list */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->alarmlist.x_enabled), conf_get_preferences_alarmlist_enabled());
    model = conf_get_preferences_alarmlist_as_liststore();
    gtk_tree_view_set_model(GTK_TREE_VIEW(p->alarmlist.view), GTK_TREE_MODEL(model));
    g_object_unref(model);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->alarmlist.x_external), conf_get_preferences_alarmlist_external());
    if(strlen(conf_get_preferences_alarmlist_ext_path()))
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(p->alarmlist.c_ext_path), conf_get_preferences_alarmlist_ext_path());

    /* Location */
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(p->s_location_latitude), conf_get_preferences_location_latitude());
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(p->s_location_longitude), conf_get_preferences_location_longitude());
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->x_location_mtscan), conf_get_preferences_location_mtscan());
    model = conf_get_preferences_location_mtscan_data_as_liststore();
    gtk_tree_view_set_model(GTK_TREE_VIEW(p->v_location_mtscan), GTK_TREE_MODEL(model));
    g_object_unref(model);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->x_location_wigle), conf_get_preferences_location_wigle());
    gtk_entry_set_text(GTK_ENTRY(p->e_location_wigle_api_url), conf_get_preferences_location_wigle_api_url());
    gtk_entry_set_text(GTK_ENTRY(p->e_location_wigle_api_key), conf_get_preferences_location_wigle_api_key());
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(p->s_location_azimuth_error), conf_get_preferences_location_azimuth_error());
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(p->s_location_min_distance), conf_get_preferences_location_min_distance());
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(p->s_location_max_distance), conf_get_preferences_location_max_distance());
}

static void
ui_preferences_load_view(ui_preferences_t   *p,
                         const gchar *const *order,
                         const gchar *const *hidden)
{
    GtkListStore *model;
    const gchar *const *ptr;
    const gchar *title;

    model = gtk_list_store_new(VIEW_MODEL_COLS,
                               G_TYPE_STRING,
                               G_TYPE_STRING,
                               G_TYPE_BOOLEAN);

    for(ptr=order; *ptr; ptr++)
    {
        title = ui_view_get_column_title(ui_view_get_column_id(*ptr));
        gtk_list_store_insert_with_values(model, NULL, -1,
                                          VIEW_MODEL_NAME, *ptr,
                                          VIEW_MODEL_TITLE, title,
                                          VIEW_MODEL_SHOW, !g_strv_contains(hidden, *ptr),
                                          -1);
    }

    gtk_tree_view_set_model(GTK_TREE_VIEW(p->v_view), GTK_TREE_MODEL(model));
    g_object_unref(model);
}

static void
ui_preferences_apply(GtkWidget *widget,
                     gpointer   data)
{
    ui_preferences_t *p = (ui_preferences_t*)data;
    gint new_icon_size;
    gchar *new_autosave_directory;
    gchar *new_screenshot_directory;
    gint new_search_column;
    gboolean new_no_style_override;
    gchar *new_network_exec;
    gint new_tzsp_udp_port;
    gint new_tzsp_channel_width;
    mtscan_conf_tzsp_band_t new_tzsp_band;
    const gchar *new_gps_hostname;
    gint new_gps_tcp_port;
    gchar *ext_path;
    gdouble new_location_latitude;
    gdouble new_location_longitude;
    gboolean new_location_mtscan;
    gchar** new_location_mtscan_data;
    gboolean is_new_location_mtscan_data;
    gboolean new_location_wigle;
    gint new_location_azimuth_error;
    gint new_location_min_distance;
    gint new_location_max_distance;
    const gchar *new_location_wigle_api_url;
    const gchar *new_location_wigle_api_key;

    /* General */
    new_icon_size = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(p->s_general_icon_size));
    if(new_icon_size != conf_get_preferences_icon_size())
    {
        conf_set_preferences_icon_size(new_icon_size);
        ui_view_set_icon_size(ui.treeview, new_icon_size);
    }

    conf_set_preferences_autosave_interval(gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(p->s_general_autosave_interval)));

    new_autosave_directory = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(p->c_general_autosave_directory));
    conf_set_path_autosave(new_autosave_directory ? new_autosave_directory : "");
    g_free(new_autosave_directory);

    new_screenshot_directory = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(p->c_general_screenshot_directory));
    conf_set_path_screenshot(new_screenshot_directory ? new_screenshot_directory : "");
    g_free(new_screenshot_directory);

    new_search_column = gtk_combo_box_get_active(GTK_COMBO_BOX(p->c_general_search_column));
    if(new_search_column != conf_get_preferences_search_column())
    {
        conf_set_preferences_search_column(new_search_column);
        ui_view_configure(ui.treeview);
    }

    conf_set_preferences_fallback_encoding(gtk_entry_get_text(GTK_ENTRY(p->e_general_fallback_encoding)));

    new_no_style_override = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p->x_general_no_style_override));
    if(new_no_style_override != conf_get_preferences_no_style_override())
    {
        conf_set_preferences_no_style_override(new_no_style_override);
        ui_view_dark_mode(ui.treeview, conf_get_interface_dark_mode());
    }

    conf_set_preferences_signals(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p->x_general_signals)));
    conf_set_preferences_display_time_only(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p->x_general_display_time_only)));
    conf_set_preferences_compact_status(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p->x_general_compact_status)));
    conf_set_preferences_reconnect(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p->x_general_reconnect)));

    /* View */
    ui_preferences_apply_view(p);

    /* Sounds */
    conf_set_preferences_sounds_new_network(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p->x_sounds_new_network)));
    conf_set_preferences_sounds_new_network_hi(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p->x_sounds_new_network_hi)));
    conf_set_preferences_sounds_new_network_al(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p->x_sounds_new_network_al)));
    conf_set_preferences_sounds_no_data(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p->x_sounds_no_data)));
    conf_set_preferences_sounds_no_gps_data(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p->x_sounds_no_gps_data)));

    /* Events */
    conf_set_preferences_events_new_network(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p->x_events_new_network)));
    new_network_exec = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(p->c_events_new_network));
    conf_set_preferences_events_new_network_exec((new_network_exec ? new_network_exec : ""));
    g_free(new_network_exec);

    /* TZSP */
    new_tzsp_udp_port = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(p->s_tzsp_udp_port));
    new_tzsp_channel_width = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(p->s_tzsp_channel_width));
    new_tzsp_band = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p->r_tzsp_band_2g)) ? MTSCAN_CONF_TZSP_BAND_2GHZ : MTSCAN_CONF_TZSP_BAND_5GHZ;

    if(ui.tzsp_rx &&
        ((conf_get_preferences_tzsp_udp_port() != new_tzsp_udp_port) ||
        (conf_get_preferences_tzsp_channel_width() != new_tzsp_channel_width) ||
        (conf_get_preferences_tzsp_band() != new_tzsp_band)))
    {
        /* TODO */
    }

    conf_set_preferences_tzsp_udp_port(new_tzsp_udp_port);
    conf_set_preferences_tzsp_channel_width(new_tzsp_channel_width);
    conf_set_preferences_tzsp_band(new_tzsp_band);

    /* GPS */
    new_gps_hostname = gtk_entry_get_text(GTK_ENTRY(p->e_gps_hostname));
    new_gps_tcp_port = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(p->s_gps_tcp_port));
    if(new_gps_tcp_port !=  conf_get_preferences_gps_tcp_port() ||
       strcmp(new_gps_hostname, conf_get_preferences_gps_hostname()))
    {
        conf_set_preferences_gps_hostname(new_gps_hostname);
        conf_set_preferences_gps_tcp_port(new_gps_tcp_port);
        if(conf_get_interface_gps())
        {
            gps_stop();
            gps_start(new_gps_hostname, new_gps_tcp_port);
        }
    }
    conf_set_preferences_gps_show_altitude(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p->x_gps_show_altitude)));
    conf_set_preferences_gps_show_errors(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p->x_gps_show_errors)));


    /* Blacklist */
    conf_set_preferences_blacklist_enabled(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p->blacklist.x_enabled)));
    conf_set_preferences_blacklist_inverted(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p->blacklist.x_inverted)));
    conf_set_preferences_blacklist_from_liststore(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(p->blacklist.view))));
    conf_set_preferences_blacklist_external(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p->blacklist.x_external)));
    ext_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(p->blacklist.c_ext_path));
    conf_set_preferences_blacklist_ext_path((ext_path ? ext_path : ""));
    g_free(ext_path);

    /* Highlight list */
    conf_set_preferences_highlightlist_enabled(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p->highlightlist.x_enabled)));
    conf_set_preferences_highlightlist_inverted(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p->highlightlist.x_inverted)));
    conf_set_preferences_highlightlist_from_liststore(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(p->highlightlist.view))));
    conf_set_preferences_highlightlist_external(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p->highlightlist.x_external)));
    ext_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(p->highlightlist.c_ext_path));
    conf_set_preferences_highlightlist_ext_path((ext_path ? ext_path : ""));
    g_free(ext_path);

    /* Alarm list */
    conf_set_preferences_alarmlist_enabled(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p->alarmlist.x_enabled)));
    conf_set_preferences_alarmlist_from_liststore(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(p->alarmlist.view))));
    conf_set_preferences_alarmlist_external(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p->alarmlist.x_external)));
    ext_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(p->alarmlist.c_ext_path));
    conf_set_preferences_alarmlist_ext_path((ext_path ? ext_path : ""));
    g_free(ext_path);

    /* Location */
    new_location_latitude = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->s_location_latitude));
    new_location_longitude = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->s_location_longitude));
    new_location_mtscan = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p->x_location_mtscan));
    new_location_mtscan_data = create_strv_from_liststore(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(p->v_location_mtscan))));
    new_location_wigle = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p->x_location_wigle));
    new_location_azimuth_error = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(p->s_location_azimuth_error));
    new_location_min_distance = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(p->s_location_min_distance));
    new_location_max_distance = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(p->s_location_max_distance));

    is_new_location_mtscan_data = !strv_equal((const gchar* const*)new_location_mtscan_data, conf_get_preferences_location_mtscan_data());

    if(is_new_location_mtscan_data)
    {
        conf_set_preferences_location_mtscan_data((const gchar* const*)new_location_mtscan_data);
        if(conf_get_interface_geoloc())
            geoloc_reinit((const gchar* const*)new_location_mtscan_data);
    }

    g_strfreev(new_location_mtscan_data);

    if(new_location_latitude != conf_get_preferences_location_latitude() ||
       new_location_longitude != conf_get_preferences_location_longitude() ||
       new_location_mtscan != conf_get_preferences_location_mtscan() ||
       new_location_wigle != conf_get_preferences_location_wigle() ||
       new_location_azimuth_error != conf_get_preferences_location_azimuth_error() ||
       new_location_min_distance != conf_get_preferences_location_min_distance() ||
       new_location_max_distance != conf_get_preferences_location_max_distance())
    {

        conf_set_preferences_location_latitude(new_location_latitude);
        conf_set_preferences_location_longitude(new_location_longitude);
        conf_set_preferences_location_mtscan(new_location_mtscan);
        conf_set_preferences_location_wigle(new_location_wigle);
        conf_set_preferences_location_azimuth_error(new_location_azimuth_error);
        conf_set_preferences_location_min_distance(new_location_min_distance);
        conf_set_preferences_location_max_distance(new_location_max_distance);

        if(conf_get_interface_geoloc() &&
           !is_new_location_mtscan_data)
        {
            ui_callback_geoloc(-1);
        }
    }

    new_location_wigle_api_url = gtk_entry_get_text(GTK_ENTRY(p->e_location_wigle_api_url));
    new_location_wigle_api_key = gtk_entry_get_text(GTK_ENTRY(p->e_location_wigle_api_key));

    if((strcmp(conf_get_preferences_location_wigle_api_url(), new_location_wigle_api_url) != 0) ||
       (strcmp(conf_get_preferences_location_wigle_api_key(), new_location_wigle_api_key) != 0))
    {
        conf_set_preferences_location_wigle_api_url(new_location_wigle_api_url);
        conf_set_preferences_location_wigle_api_key(new_location_wigle_api_key);
        if(conf_get_interface_geoloc())
            geoloc_wigle(new_location_wigle_api_url, new_location_wigle_api_key);
    }

    /* --- */
    gtk_widget_destroy(p->window);
}

static void
ui_preferences_apply_view(ui_preferences_t *p)
{
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(p->v_view));
    gchar **order;
    gchar **hidden;
    GtkTreeIter iter;
    gint length, i;
    gchar *name;
    gboolean is_visible;
    gint hidden_count;

    length = gtk_tree_model_iter_n_children(model, NULL);

    order = g_new(gchar*, length+1);
    i = 0;
    hidden_count = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(model), NULL);
    if(gtk_tree_model_get_iter_first(model, &iter))
    {
        do
        {
            gtk_tree_model_get(model, &iter,
                               VIEW_MODEL_NAME, &name,
                               VIEW_MODEL_SHOW, &is_visible,
                               -1);
            order[i++] = name;
            hidden_count -= (gint)is_visible;
        } while(gtk_tree_model_iter_next(model, &iter));
    }
    order[i] = NULL;
    conf_set_preferences_view_cols_order((const gchar* const*)order);
    ui_view_set_columns_order(ui.treeview, (const gchar* const*)order);
    g_strfreev(order);

    hidden = g_new(gchar*, hidden_count+1);
    i = 0;
    if(gtk_tree_model_get_iter_first(model, &iter))
    {
        do
        {
            gtk_tree_model_get(model, &iter,
                               VIEW_MODEL_NAME, &name,
                               VIEW_MODEL_SHOW, &is_visible,
                               -1);
            if(is_visible)
            {
                g_free(name);
                continue;
            }

            hidden[i++] = name;
        } while(gtk_tree_model_iter_next(model, &iter));
    }
    hidden[i] = NULL;
    conf_set_preferences_view_cols_hidden((const gchar* const*)hidden);
    ui_view_set_columns_hidden(ui.treeview, (const gchar* const*)hidden);
    g_strfreev(hidden);
}
