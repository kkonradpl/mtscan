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
#include <stdlib.h>
#include "conf.h"
#include "ui-connection.h"
#include "ui-dialogs.h"
#include "ui-callbacks.h"
#include "callbacks.h"
#include "conf.h"

typedef struct ui_connection
{
    GtkWidget *dialog;
    GtkWidget *parent;
    GtkWidget *content;
    GtkWidget *table;

    GtkWidget *box_profile;
    GtkWidget *l_profile;
    GtkWidget *c_profile;
    GtkWidget *b_profile_add;
    GtkWidget *b_profile_remove;
    GtkWidget *b_profile_clear;

    GtkWidget *l_host;
    GtkWidget *e_host;
    GtkWidget *s_port;

    GtkWidget *l_login;
    GtkWidget *e_login;

    GtkWidget *l_password;
    GtkWidget *e_password;
    GtkWidget *c_password;

    GtkWidget *l_interface;
    GtkWidget *e_interface;

    GtkWidget *l_mode;
    GtkWidget *box_mode;
    GtkWidget *r_scanner;
    GtkWidget *r_sniffer;

    GtkWidget *c_duration;
    GtkWidget *box_duration;
    GtkWidget *s_duration;
    GtkWidget *l_duration_sec;

    GtkWidget *c_remote;

    GtkWidget *c_background;

    GtkWidget *box_status_wrapper;
    GtkWidget *box_status;
    GtkWidget *spinner;
    GtkWidget *l_status;

    GtkWidget *box_button;
    GtkWidget *b_connect;
    GtkWidget *b_cancel;
    GtkWidget *b_close;

    gboolean profile_reset_flag;
    guint reconnect;
    gboolean reconnect_idle;
} ui_connection_t;

static void ui_connection_destroy(GtkWidget*, gpointer);
static void ui_connection_format_desc(GtkCellLayout*, GtkCellRenderer*, GtkTreeModel*, GtkTreeIter*, gpointer);
static void ui_connection_profile_changed(GtkComboBox*, gpointer);
static void ui_connection_profile_unset(GtkWidget*, gpointer);
static void ui_connection_mode_toggled(GtkWidget*, gpointer);
static void ui_connection_duration_toggled(GtkWidget*, gpointer);
static void ui_connection_remote_toggled(GtkWidget*, gpointer);
static void ui_connection_background_toggled(GtkWidget*, gpointer);
static gboolean ui_connection_key(GtkWidget*, GdkEventKey*, gpointer);
static void ui_connection_unlock(ui_connection_t*, gboolean);
static void ui_connection_profile_add(GtkWidget*, gpointer);
static gboolean ui_connection_profile_add_key(GtkWidget*, GdkEventKey*, gpointer);
static void ui_connection_profile_remove(GtkWidget*, gpointer);
static void ui_connection_profile_clear(GtkWidget*, gpointer);
static void ui_connection_connect(GtkWidget*, gpointer);
static void ui_connection_cancel(GtkWidget*, gpointer);

static void ui_connection_from_profile(ui_connection_t *, const conf_profile_t *);
static conf_profile_t* ui_connection_to_profile(ui_connection_t *, const gchar *);

static gboolean ui_connection_reconnect(gpointer);

ui_connection_t*
ui_connection_new(gint                 profile,
                  ui_connection_mode_t mode)
{
    ui_connection_t *c;
    guint row;
    GtkCellRenderer *renderer;

    c = g_malloc0(sizeof(ui_connection_t));

    c->parent = ui.window;
    c->dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(c->dialog), "Connection");
    gtk_window_set_transient_for(GTK_WINDOW(c->dialog), GTK_WINDOW(c->parent));
    gtk_window_set_destroy_with_parent(GTK_WINDOW(c->dialog), TRUE);
    gtk_window_set_resizable(GTK_WINDOW(c->dialog), FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(c->dialog), 4);

    c->content = gtk_vbox_new(FALSE, 3);
    gtk_container_add(GTK_CONTAINER(c->dialog), c->content);

    c->box_profile = gtk_hbox_new(FALSE, 0);
    c->l_profile = gtk_label_new("Profile:");
    gtk_misc_set_alignment(GTK_MISC(c->l_profile), 0.0, 0.5);
    gtk_box_pack_start(GTK_BOX(c->box_profile), c->l_profile, FALSE, FALSE, 0);

    c->c_profile = gtk_combo_box_new_with_model(GTK_TREE_MODEL(conf_get_profiles()));
    renderer = gtk_cell_renderer_text_new();
    g_object_set(renderer, "ellipsize", PANGO_ELLIPSIZE_END, "ellipsize-set", TRUE, NULL);
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(c->c_profile), renderer, TRUE);
    gtk_cell_layout_set_cell_data_func(GTK_CELL_LAYOUT(c->c_profile), renderer, ui_connection_format_desc, NULL, NULL);
    gtk_box_pack_start(GTK_BOX(c->box_profile), c->c_profile, TRUE, TRUE, 4);

    c->b_profile_add = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(c->b_profile_add), gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_BUTTON));
    gtk_box_pack_start(GTK_BOX(c->box_profile), c->b_profile_add, FALSE, FALSE, 0);
    c->b_profile_remove = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(c->b_profile_remove), gtk_image_new_from_stock(GTK_STOCK_REMOVE, GTK_ICON_SIZE_BUTTON));
    gtk_box_pack_start(GTK_BOX(c->box_profile), c->b_profile_remove, FALSE, FALSE, 0);
    c->b_profile_clear = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(c->b_profile_clear), gtk_image_new_from_stock(GTK_STOCK_CLEAR, GTK_ICON_SIZE_BUTTON));
    gtk_box_pack_start(GTK_BOX(c->box_profile), c->b_profile_clear, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(c->content), c->box_profile, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(c->content), gtk_hseparator_new(), FALSE, FALSE, 4);

    c->table = gtk_table_new(8, 3, TRUE);
    gtk_table_set_homogeneous(GTK_TABLE(c->table), FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(c->table), 2);
    gtk_table_set_col_spacings(GTK_TABLE(c->table), 2);
    gtk_container_add(GTK_CONTAINER(c->content), c->table);

    row = 0;
    c->l_host = gtk_label_new("Address:");
    gtk_misc_set_alignment(GTK_MISC(c->l_host), 0.0, 0.5);
    c->e_host = gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(c->e_host), 20);
    c->s_port = gtk_spin_button_new(GTK_ADJUSTMENT(gtk_adjustment_new((gdouble)22.0, 1.0, 65535.0, 1.0, 10.0, 0.0)), 0, 0);
    gtk_table_attach(GTK_TABLE(c->table), c->l_host, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    gtk_table_attach(GTK_TABLE(c->table), c->e_host, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    gtk_table_attach(GTK_TABLE(c->table), c->s_port, 2, 3, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    c->l_login = gtk_label_new("Login:");
    gtk_misc_set_alignment(GTK_MISC(c->l_login), 0.0, 0.5);
    c->e_login = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(c->table), c->l_login, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    gtk_table_attach(GTK_TABLE(c->table), c->e_login, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    c->l_password = gtk_label_new("Password:");
    gtk_misc_set_alignment(GTK_MISC(c->l_password), 0.0, 0.5);
    c->e_password = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(c->e_password), FALSE);
    c->c_password = gtk_check_button_new_with_label("Keep");
    gtk_table_attach(GTK_TABLE(c->table), c->l_password, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    gtk_table_attach(GTK_TABLE(c->table), c->e_password, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    gtk_table_attach(GTK_TABLE(c->table), c->c_password, 2, 3, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    c->l_interface = gtk_label_new("Interface:");
    gtk_misc_set_alignment(GTK_MISC(c->l_interface), 0.0, 0.5);
    c->e_interface = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(c->table), c->l_interface, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    gtk_table_attach(GTK_TABLE(c->table), c->e_interface, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    c->l_mode = gtk_label_new("Mode:");
    gtk_misc_set_alignment(GTK_MISC(c->l_mode), 0.0, 0.5);
    c->box_mode = gtk_hbox_new(FALSE, 6);
    c->r_scanner = gtk_radio_button_new_with_label(NULL, "scanner");
    gtk_box_pack_start(GTK_BOX(c->box_mode), c->r_scanner, FALSE, FALSE, 0);
    c->r_sniffer = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(c->r_scanner), "sniffer (tzsp)");
    gtk_box_pack_start(GTK_BOX(c->box_mode), c->r_sniffer, FALSE, FALSE, 0);
    gtk_table_attach(GTK_TABLE(c->table), c->l_mode, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    gtk_table_attach(GTK_TABLE(c->table), c->box_mode, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    c->c_duration = gtk_check_button_new_with_label("Duration:");
    c->box_duration = gtk_hbox_new(FALSE, 0);
    c->s_duration = gtk_spin_button_new(GTK_ADJUSTMENT(gtk_adjustment_new((gdouble)10.0, 1.0, 99999.0, 1.0, 10.0, 0.0)), 0, 0);
    gtk_box_pack_start(GTK_BOX(c->box_duration), c->s_duration, FALSE, FALSE, 1);
    c->l_duration_sec = gtk_label_new("seconds");
    gtk_misc_set_alignment(GTK_MISC(c->l_duration_sec), 0.0, 0.5);
    gtk_box_pack_start(GTK_BOX(c->box_duration), c->l_duration_sec, FALSE, FALSE, 5);
    gtk_table_attach(GTK_TABLE(c->table), c->c_duration, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    gtk_table_attach(GTK_TABLE(c->table), c->box_duration, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    c->c_remote = gtk_check_button_new_with_label("Remote continuous scanning");
    gtk_table_attach(GTK_TABLE(c->table), c->c_remote, 0, 3, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    c->c_background = gtk_check_button_new_with_label("Background scanning");
    gtk_table_attach(GTK_TABLE(c->table), c->c_background, 0, 3, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    c->box_status_wrapper = gtk_hbox_new(TRUE, 5);
    gtk_box_pack_start(GTK_BOX(c->content), c->box_status_wrapper, FALSE, FALSE, 1);
    c->box_status = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(c->box_status_wrapper), c->box_status, FALSE, FALSE, 1);
    c->spinner = gtk_spinner_new();
    gtk_box_pack_start(GTK_BOX(c->box_status), c->spinner, FALSE, FALSE, 1);
    c->l_status = gtk_label_new(NULL);
    gtk_label_set_line_wrap(GTK_LABEL(c->l_status), TRUE);
    gtk_box_pack_start(GTK_BOX(c->box_status), c->l_status, FALSE, FALSE, 1);
    c->box_button = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(c->box_button), GTK_BUTTONBOX_END);
    gtk_box_set_spacing(GTK_BOX(c->box_button), 5);
    gtk_box_pack_start(GTK_BOX(c->content), c->box_button, FALSE, FALSE, 5);

    c->b_connect = gtk_button_new_from_stock(GTK_STOCK_OK);
    gtk_container_add(GTK_CONTAINER(c->box_button), c->b_connect);
    c->b_cancel = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
    gtk_container_add(GTK_CONTAINER(c->box_button), c->b_cancel);
    c->b_close = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
    gtk_container_add(GTK_CONTAINER(c->box_button), c->b_close);

    g_signal_connect(c->c_profile, "changed", G_CALLBACK(ui_connection_profile_changed), c);
    g_signal_connect(c->b_profile_add, "clicked", G_CALLBACK(ui_connection_profile_add), c);
    g_signal_connect(c->b_profile_remove, "clicked", G_CALLBACK(ui_connection_profile_remove), c);
    g_signal_connect(c->b_profile_clear, "clicked", G_CALLBACK(ui_connection_profile_clear), c);

    g_signal_connect(c->e_host, "changed", G_CALLBACK(ui_connection_profile_unset), c);
    g_signal_connect(c->s_port, "changed", G_CALLBACK(ui_connection_profile_unset), c);
    g_signal_connect(c->e_login, "changed", G_CALLBACK(ui_connection_profile_unset), c);
    g_signal_connect(c->e_password, "changed", G_CALLBACK(ui_connection_profile_unset), c);
    g_signal_connect(c->e_interface, "changed", G_CALLBACK(ui_connection_profile_unset), c);
    g_signal_connect(c->r_scanner, "toggled", G_CALLBACK(ui_connection_profile_unset), c);
    g_signal_connect(c->r_sniffer, "toggled", G_CALLBACK(ui_connection_profile_unset), c);
    g_signal_connect(c->s_duration, "changed", G_CALLBACK(ui_connection_profile_unset), c);
    g_signal_connect(c->c_duration, "toggled", G_CALLBACK(ui_connection_profile_unset), c);
    g_signal_connect(c->c_remote, "toggled", G_CALLBACK(ui_connection_profile_unset), c);
    g_signal_connect(c->c_background, "toggled", G_CALLBACK(ui_connection_profile_unset), c);

    g_signal_connect(c->r_scanner, "toggled", G_CALLBACK(ui_connection_mode_toggled), c);
    g_signal_connect(c->r_sniffer, "toggled", G_CALLBACK(ui_connection_mode_toggled), c);
    g_signal_connect(c->c_duration, "toggled", G_CALLBACK(ui_connection_duration_toggled), c);
    g_signal_connect(c->c_remote, "toggled", G_CALLBACK(ui_connection_remote_toggled), c);
    g_signal_connect(c->c_background, "toggled", G_CALLBACK(ui_connection_background_toggled), c);

    g_signal_connect(c->b_connect, "clicked", G_CALLBACK(ui_connection_connect), c);
    g_signal_connect(c->b_cancel, "clicked", G_CALLBACK(ui_connection_cancel), c);
    g_signal_connect_swapped(c->b_close, "clicked", G_CALLBACK(gtk_widget_destroy), c->dialog);

    g_signal_connect(c->dialog, "key-press-event", G_CALLBACK(ui_connection_key), c);
    g_signal_connect(c->dialog, "destroy", G_CALLBACK(ui_connection_destroy), c);

    if(profile > 0)
        gtk_combo_box_set_active(GTK_COMBO_BOX(c->c_profile), profile-1);
    else
        gtk_combo_box_set_active(GTK_COMBO_BOX(c->c_profile), conf_get_interface_last_profile()-1);

    if(gtk_combo_box_get_active(GTK_COMBO_BOX(c->c_profile)) < 0)
    {
        ui_connection_from_profile(c, conf_get_profile_default());
        if(profile > 0)
            mode = UI_CONNECTION_MODE_NONE;
    }

    gtk_widget_grab_focus(c->e_host);

    if(mode == UI_CONNECTION_MODE_RECONNECT ||
       mode == UI_CONNECTION_MODE_RECONNECT_IDLE)
        gtk_window_set_focus_on_map(GTK_WINDOW(c->dialog), FALSE);
    else
        gtk_window_set_modal(GTK_WINDOW(c->dialog), TRUE);

    gtk_widget_show_all(c->dialog);
    ui_connection_unlock(c, TRUE);

    if(mode == UI_CONNECTION_MODE_RECONNECT ||
       mode == UI_CONNECTION_MODE_RECONNECT_IDLE)
        gtk_window_set_modal(GTK_WINDOW(c->dialog), TRUE);

    if(mode != UI_CONNECTION_MODE_NONE)
    {
        if(mode == UI_CONNECTION_MODE_RECONNECT_IDLE)
            c->reconnect_idle = TRUE;

        gtk_button_clicked(GTK_BUTTON(c->b_connect));
    }

    return c;
}

static void
ui_connection_destroy(GtkWidget *widget,
                      gpointer   user_data)
{
    ui_connection_t *c = (ui_connection_t*)user_data;

    conf_set_profile_default(ui_connection_to_profile(c, "default"));
    conf_set_interface_last_profile(gtk_combo_box_get_active(GTK_COMBO_BOX(c->c_profile)) + 1);

    gtk_button_clicked(GTK_BUTTON(c->b_cancel));

    if(c->reconnect)
    {
        g_source_remove(c->reconnect);
        c->reconnect = 0;
    }

    if(ui.conn_dialog == c)
        ui.conn_dialog = NULL;

    g_free(c);
}

static void
ui_connection_format_desc(GtkCellLayout   *cell,
                          GtkCellRenderer *renderer,
                          GtkTreeModel    *model,
                          GtkTreeIter     *iter,
                          gpointer         user_data)
{
    gchar *name;
    gchar *iter_str;
    gchar *text;

    iter_str = gtk_tree_model_get_string_from_iter(model, iter);
    gtk_tree_model_get(model, iter, CONF_PROFILE_COL_NAME, &name, -1);
    text = g_strdup_printf("%d. %s", atoi(iter_str)+1, name);
    g_object_set(renderer, "text", text, NULL);

    g_free(iter_str);
    g_free(text);
    g_free(name);
}

static void
ui_connection_profile_changed(GtkComboBox *widget,
                              gpointer     user_data)
{
    ui_connection_t *c = (ui_connection_t*)user_data;
    GtkTreeIter iter;
    conf_profile_t *p;
    GtkTreeModel *model;

    if(!gtk_combo_box_get_active_iter(widget, &iter))
        return;

    model = gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
    p = conf_profile_list_get(GTK_LIST_STORE(model), &iter);
    ui_connection_from_profile(c, p);
    conf_profile_free(p);
}

static void
ui_connection_profile_unset(GtkWidget *widget,
                            gpointer user_data)
{
    ui_connection_t *c = (ui_connection_t*)user_data;

    if(c->profile_reset_flag)
        gtk_combo_box_set_active(GTK_COMBO_BOX(c->c_profile), -1);
}

static void
ui_connection_mode_toggled(GtkWidget *widget,
                           gpointer   user_data)
{
    ui_connection_t *c = (ui_connection_t*)user_data;
    ui_connection_unlock(c, TRUE);
}

static void
ui_connection_duration_toggled(GtkWidget *widget,
                               gpointer   user_data)
{
    ui_connection_t *c = (ui_connection_t*)user_data;

    if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(c->c_duration)) &&
       gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(c->c_remote)))
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(c->c_remote), FALSE);
    }
}

static void
ui_connection_remote_toggled(GtkWidget *widget,
                             gpointer   user_data)
{
    ui_connection_t *c = (ui_connection_t*)user_data;

    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(c->c_remote)))
    {
        if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(c->c_duration)))
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(c->c_duration), TRUE);
        if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(c->c_background)))
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(c->c_background), FALSE);
    }
}

static void
ui_connection_background_toggled(GtkWidget *widget,
                                 gpointer   user_data)
{
    ui_connection_t *c = (ui_connection_t*)user_data;

    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(c->c_background)) &&
       gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(c->c_remote)))
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(c->c_remote), FALSE);
    }
}

static gboolean
ui_connection_key(GtkWidget   *widget,
                  GdkEventKey *event,
                  gpointer     user_data)
{
    ui_connection_t *c = (ui_connection_t*)user_data;
    switch(gdk_keyval_to_upper(event->keyval))
    {
    case GDK_KEY_Escape:
        gtk_widget_destroy(c->dialog);
        return TRUE;
    case GDK_KEY_Return:
        if(gtk_widget_get_sensitive(c->b_connect))
            gtk_button_clicked(GTK_BUTTON(c->b_connect));
        return TRUE;
    }
    return FALSE;
}

static void
ui_connection_unlock(ui_connection_t *c,
                     gboolean         value)
{
    gboolean scanner = value && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(c->r_scanner));

    gtk_widget_set_sensitive(c->c_profile, value);
    gtk_widget_set_sensitive(c->b_profile_add, value);
    gtk_widget_set_sensitive(c->b_profile_remove, value);
    gtk_widget_set_sensitive(c->b_profile_clear, value);
    gtk_widget_set_sensitive(c->e_host, value);
    gtk_widget_set_sensitive(c->s_port, value);
    gtk_widget_set_sensitive(c->e_login, value);
    gtk_widget_set_sensitive(c->e_password, value);
    gtk_widget_set_sensitive(c->c_password, value);
    gtk_widget_set_sensitive(c->e_interface, value);
    gtk_widget_set_sensitive(c->r_scanner, value);
    gtk_widget_set_sensitive(c->r_sniffer, value);

    gtk_widget_set_sensitive(c->c_duration, scanner);
    gtk_widget_set_sensitive(c->s_duration, scanner);
    gtk_widget_set_sensitive(c->c_remote, scanner);
    gtk_widget_set_sensitive(c->c_background, scanner);

    gtk_widget_set_sensitive(c->b_connect, value);
    gtk_widget_set_sensitive(c->b_cancel, !value);

    if(value)
    {
        gtk_spinner_stop(GTK_SPINNER(c->spinner));
        gtk_widget_hide(c->spinner);
        c->reconnect_idle = FALSE;
    }
    else
    {
        gtk_spinner_start(GTK_SPINNER(c->spinner));
        gtk_widget_show(c->spinner);
    }
}

static void
ui_connection_profile_add(GtkWidget *widget,
                          gpointer   user_data)
{
    ui_connection_t *c = (ui_connection_t*)user_data;
    GtkWidget *dialog_profile, *entry;
    gchar *default_name;
    conf_profile_t *p;
    GtkTreeIter iter;
    GtkTreeModel *model;

    dialog_profile = gtk_message_dialog_new(GTK_WINDOW(c->dialog),
                                            GTK_DIALOG_MODAL,
                                            GTK_MESSAGE_QUESTION,
                                            GTK_BUTTONS_OK_CANCEL,
                                            NULL);
    gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG(dialog_profile), "<big>Profile name:</big>");
    gtk_window_set_title(GTK_WINDOW(dialog_profile), "Add a profile");

    entry = gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(entry), 20);
    gtk_container_add(GTK_CONTAINER(gtk_message_dialog_get_message_area(GTK_MESSAGE_DIALOG(dialog_profile))), entry);

    default_name = g_strdup_printf("%s/%s",
                                   gtk_entry_get_text(GTK_ENTRY(c->e_host)),
                                   gtk_entry_get_text(GTK_ENTRY(c->e_interface)));
    gtk_entry_set_text(GTK_ENTRY(entry), default_name);
    g_free(default_name);

    g_signal_connect(dialog_profile, "key-press-event", G_CALLBACK(ui_connection_profile_add_key), NULL);
    gtk_widget_show_all(dialog_profile);

    if(gtk_dialog_run(GTK_DIALOG(dialog_profile)) == GTK_RESPONSE_OK)
    {
        p = ui_connection_to_profile(c, gtk_entry_get_text(GTK_ENTRY(entry)));
        model = gtk_combo_box_get_model(GTK_COMBO_BOX(c->c_profile));
        iter = conf_profile_list_add(GTK_LIST_STORE(model), p);
        conf_profile_free(p);

        g_signal_handlers_block_by_func(G_OBJECT(c->c_profile), GINT_TO_POINTER(ui_connection_profile_changed), NULL);
        gtk_combo_box_set_active_iter(GTK_COMBO_BOX(c->c_profile), &iter);
        g_signal_handlers_unblock_by_func(G_OBJECT(c->c_profile), GINT_TO_POINTER(ui_connection_profile_changed), NULL);

    }
    gtk_widget_destroy(dialog_profile);
}

static gboolean
ui_connection_profile_add_key(GtkWidget   *widget,
                              GdkEventKey *event,
                              gpointer     user_data)
{
    switch(gdk_keyval_to_upper(event->keyval))
    {
    case GDK_KEY_Escape:
        gtk_dialog_response(GTK_DIALOG(widget), GTK_RESPONSE_CANCEL);
        return TRUE;
    case GDK_KEY_Return:
        gtk_dialog_response(GTK_DIALOG(widget), GTK_RESPONSE_OK);
        return TRUE;
    }
    return FALSE;
}

static void
ui_connection_profile_remove(GtkWidget *widget,
                             gpointer   user_data)
{
    ui_connection_t *c = (ui_connection_t*)user_data;
    GtkTreeIter iter;

    if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(c->c_profile), &iter))
    {
        if(ui_dialog_yesno(GTK_WINDOW(c->dialog), "<big>Remove a profile</big>\nAre you sure you want to remove the selected profile?") == UI_DIALOG_YES)
            gtk_list_store_remove(GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(c->c_profile))), &iter);
    }
}

static void
ui_connection_profile_clear(GtkWidget *widget,
                            gpointer   user_data)
{
    ui_connection_t *c = (ui_connection_t*)user_data;

    if(ui_dialog_yesno(GTK_WINDOW(c->dialog), "<big><b>Remove all profiles</b></big>\nAre you sure you want to remove all profiles?") == UI_DIALOG_YES)
        gtk_list_store_clear(GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(c->c_profile))));
}

static void
ui_connection_connect(GtkWidget *widget,
                      gpointer   user_data)
{
    ui_connection_t *c = (ui_connection_t*)user_data;
    gchar *name = NULL;
    const gchar *hostname = gtk_entry_get_text(GTK_ENTRY(c->e_host));
    gint port = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(c->s_port));
    const gchar *login = gtk_entry_get_text(GTK_ENTRY(c->e_login));
    const gchar *password = gtk_entry_get_text(GTK_ENTRY(c->e_password));
    const gchar *iface = gtk_entry_get_text(GTK_ENTRY(c->e_interface));
    gboolean sniffer = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(c->r_sniffer));
    gboolean remote = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(c->c_remote));
    gboolean background = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(c->c_background));
    gboolean duration_enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(c->c_duration));
    gint duration = (duration_enabled ? gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(c->s_duration)) : 0);
    mt_ssh_mode_t mode;
    gboolean skip_verification;
    GtkTreeIter iter;
    GtkTreeModel *model;

    if(ui.conn)
        return;

    if(!strlen(hostname))
    {
        ui_connection_set_status(c, "No hostname specified.", NULL);
        gtk_widget_grab_focus(c->e_host);
        return;
    }

    if(!strlen(login))
    {
        ui_connection_set_status(c, "No login specified.", NULL);
        gtk_widget_grab_focus(c->e_login);
        return;
    }

    if(!strlen(iface))
    {
        ui_connection_set_status(c, "No interface specified.", NULL);
        gtk_widget_grab_focus(c->e_interface);
        return;
    }

    ui_connection_set_status(c, "Initializing...", NULL);
    ui_connection_unlock(c, FALSE);

    if(sniffer)
    {
        ui.mode = MTSCAN_MODE_SNIFFER;
        mode = MT_SSH_MODE_SNIFFER;
        mtscan_model_set_active_timeout(ui.model, MODEL_DEFAULT_ACTIVE_TIMEOUT);
    }
    else
    {
        ui.mode = MTSCAN_MODE_SCANNER;
        mode = MT_SSH_MODE_SCANNER;
        mtscan_model_set_active_timeout(ui.model, (remote ? duration : 2));
    }

    skip_verification = conf_get_runtime_skip_verification();

    model = gtk_combo_box_get_model(GTK_COMBO_BOX(c->c_profile));
    if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(c->c_profile), &iter))
        gtk_tree_model_get(model, &iter, CONF_PROFILE_COL_NAME, &name, -1);

    ui.conn = mt_ssh_new(callback_mt_ssh,
                         callback_mt_ssh_msg,
                         (c->reconnect_idle ? MT_SSH_MODE_NONE : mode),
                         name,
                         hostname,
                         port,
                         login,
                         password,
                         iface,
                         duration,
                         remote,
                         background,
                         skip_verification);

    g_free(name);
}

static void
ui_connection_cancel(GtkWidget *widget,
                     gpointer   user_data)
{
    ui_connection_t *c = (ui_connection_t*)user_data;

    if(ui.conn && !ui.connected)
    {
        mt_ssh_cancel(ui.conn);
        ui.conn = NULL;
        ui_connection_unlock(c, TRUE);
        ui_connection_set_status(c, "Connection cancelled.", NULL);
    }

    if(c->reconnect)
    {
        g_source_remove(c->reconnect);
        c->reconnect = 0;
        ui_connection_unlock(c, TRUE);
    }
}

static void
ui_connection_from_profile(ui_connection_t      *c,
                           const conf_profile_t *p)
{
    c->profile_reset_flag = FALSE;

    gtk_entry_set_text(GTK_ENTRY(c->e_host), conf_profile_get_host(p));
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(c->s_port), (gdouble)conf_profile_get_port(p));
    gtk_entry_set_text(GTK_ENTRY(c->e_login), conf_profile_get_login(p));
    gtk_entry_set_text(GTK_ENTRY(c->e_password), conf_profile_get_password(p));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(c->c_password), (gboolean)strlen(conf_profile_get_password(p)));
    gtk_entry_set_text(GTK_ENTRY(c->e_interface), conf_profile_get_interface(p));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(c->r_scanner), !conf_profile_get_mode(p));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(c->r_sniffer), conf_profile_get_mode(p));
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(c->s_duration), (gdouble)conf_profile_get_duration_time(p));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(c->c_duration), conf_profile_get_duration(p));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(c->c_remote), conf_profile_get_remote(p));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(c->c_background), conf_profile_get_background(p));

    c->profile_reset_flag = TRUE;
}

static conf_profile_t*
ui_connection_to_profile(ui_connection_t *c,
                         const gchar     *name)
{
    conf_profile_t *p;
    gboolean keep_pass;
    keep_pass = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(c->c_password));

    p = conf_profile_new(g_strdup(name),
                         g_strdup(gtk_entry_get_text(GTK_ENTRY(c->e_host))),
                         gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(c->s_port)),
                         g_strdup(gtk_entry_get_text(GTK_ENTRY(c->e_login))),
                         g_strdup(keep_pass ? gtk_entry_get_text(GTK_ENTRY(c->e_password)) : ""),
                         g_strdup(gtk_entry_get_text(GTK_ENTRY(c->e_interface))),
                         (mtscan_conf_profile_mode_t)gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(c->r_sniffer)),
                         gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(c->s_duration)),
                         gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(c->c_duration)),
                         gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(c->c_remote)),
                         gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(c->c_background)));

    return p;
}

void
ui_connection_set_status(ui_connection_t *c,
                         const gchar     *string,
                         const gchar     *extended_error)
{
    gchar *markup;

    if(extended_error)
    {
        ui_dialog(GTK_WINDOW(c->dialog),
                  GTK_MESSAGE_ERROR,
                  APP_NAME,
                  "<b><big>Error:</big></b>\n%s",
                  extended_error);
    }

    gtk_widget_show(c->l_status);
    markup = g_markup_printf_escaped("<b>%s</b>", string);
    gtk_label_set_markup(GTK_LABEL(c->l_status), markup);
    g_free(markup);
}

gboolean
ui_connection_verify(ui_connection_t *c,
                     const gchar     *info)
{
    return (ui_dialog_yesno(GTK_WINDOW(c->dialog), info) == UI_DIALOG_YES);
}

void
ui_connection_connected(ui_connection_t *c)
{
    gtk_widget_destroy(c->dialog);
}

void
ui_connection_disconnected(ui_connection_t *c)
{
    if(conf_get_preferences_reconnect())
        c->reconnect = g_timeout_add(3000, ui_connection_reconnect, c);
    else
        ui_connection_unlock(c, TRUE);
}

static gboolean
ui_connection_reconnect(gpointer user_data)
{
    ui_connection_t *c = (ui_connection_t*)user_data;
    gtk_button_clicked(GTK_BUTTON(c->b_connect));
    c->reconnect = 0;
    return FALSE;
}
