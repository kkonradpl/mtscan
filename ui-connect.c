#include <gdk/gdkkeysyms.h>
#include <string.h>
#include <stdlib.h>
#include "conf.h"
#include "ui-connect.h"
#include "ui-dialogs.h"
#include "ui-toolbar.h"
#include "ui-update.h"
#include "conn.h"
#include "mt-ssh.h"

static GtkWidget *dialog = NULL;
static GtkWidget *content, *table;
static GtkWidget *hbox_profile, *l_profile, *c_profile, *b_profile_add, *b_profile_remove, *b_profile_clear;
static GtkWidget *l_host, *e_host, *s_port;
static GtkWidget *l_login, *e_login;
static GtkWidget *l_password, *e_password, *c_password;
static GtkWidget *l_interface, *e_interface;
static GtkWidget *c_duration, *hbox_duration, *s_duration, *l_duration_sec;
static GtkWidget *c_remote;
static GtkWidget *c_background;
static GtkWidget *box_status_wrapper, *box_status, *spinner, *l_status;
static GtkWidget *box_button, *b_connect, *b_cancel, *b_close;
static gboolean profile_reset_flag = TRUE;

static void connection_dialog_destroy(GtkWidget*, gpointer);
static void connection_dialog_profile_changed(GtkComboBox*, gpointer);
static void connection_dialog_profile_unset(GtkWidget*, gpointer);
static void connection_dialog_duration_toggled(GtkWidget*, gpointer);
static void connection_dialog_remote_toggled(GtkWidget*, gpointer);
static void connection_dialog_background_toggled(GtkWidget*, gpointer);
static gboolean connection_dialog_key(GtkWidget*, GdkEventKey*, gpointer);
static void connection_dialog_unlock(gboolean);
static void connection_dialog_status(const gchar*);
static void connection_dialog_profile_add(GtkWidget*, gpointer);
static gboolean connection_dialog_profile_add_key(GtkWidget*, GdkEventKey*, gpointer);
static void connection_dialog_profile_remove(GtkWidget*, gpointer);
static void connection_dialog_profile_clear(GtkWidget*, gpointer);
static void connection_dialog_connect(GtkWidget*, gpointer);
static void connection_dialog_cancel(GtkWidget*, gpointer);
static void connection_cancel();

void
connection_dialog()
{
    gint row;

    dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(dialog), "Connection");
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(ui.window));
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_destroy_with_parent(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(dialog), 4);

    content = gtk_vbox_new(FALSE, 3);
    gtk_container_add(GTK_CONTAINER(dialog), content);

    hbox_profile = gtk_hbox_new(FALSE, 0);
    l_profile = gtk_label_new("Profile:");
    gtk_misc_set_alignment(GTK_MISC(l_profile), 0.0, 0.5);
    gtk_box_pack_start(GTK_BOX(hbox_profile), l_profile, FALSE, FALSE, 0);

    c_profile = gtk_combo_box_new_with_model(GTK_TREE_MODEL(conf_get_profiles()));
    GtkCellRenderer *cell = gtk_cell_renderer_text_new();
    g_object_set(cell, "ellipsize", PANGO_ELLIPSIZE_END, "ellipsize-set", TRUE, NULL);
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(c_profile), cell, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(c_profile), cell, "text", PROFILE_COL_NAME, NULL);
    gtk_box_pack_start(GTK_BOX(hbox_profile), c_profile, TRUE, TRUE, 4);

    b_profile_add = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(b_profile_add), gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_BUTTON));
    gtk_box_pack_start(GTK_BOX(hbox_profile), b_profile_add, FALSE, FALSE, 0);
    b_profile_remove = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(b_profile_remove), gtk_image_new_from_stock(GTK_STOCK_REMOVE, GTK_ICON_SIZE_BUTTON));
    gtk_box_pack_start(GTK_BOX(hbox_profile), b_profile_remove, FALSE, FALSE, 0);
    b_profile_clear = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(b_profile_clear), gtk_image_new_from_stock(GTK_STOCK_CLEAR, GTK_ICON_SIZE_BUTTON));
    gtk_box_pack_start(GTK_BOX(hbox_profile), b_profile_clear, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content), hbox_profile, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(content), gtk_hseparator_new(), FALSE, FALSE, 4);

    table = gtk_table_new(7, 3, TRUE);
    gtk_table_set_homogeneous(GTK_TABLE(table), FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(table), 2);
    gtk_table_set_col_spacings(GTK_TABLE(table), 2);
    gtk_container_add(GTK_CONTAINER(content), table);

    row = 0;
    l_host = gtk_label_new("Address:");
    gtk_misc_set_alignment(GTK_MISC(l_host), 0.0, 0.5);
    e_host = gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(e_host), 20);
    s_port = gtk_spin_button_new(GTK_ADJUSTMENT(gtk_adjustment_new((gdouble)22.0, 1.0, 65535.0, 1.0, 10.0, 0.0)), 0, 0);
    gtk_table_attach(GTK_TABLE(table), l_host, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    gtk_table_attach(GTK_TABLE(table), e_host, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    gtk_table_attach(GTK_TABLE(table), s_port, 2, 3, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    l_login = gtk_label_new("Login:");
    gtk_misc_set_alignment(GTK_MISC(l_login), 0.0, 0.5);
    e_login = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(table), l_login, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    gtk_table_attach(GTK_TABLE(table), e_login, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    l_password = gtk_label_new("Password:");
    gtk_misc_set_alignment(GTK_MISC(l_password), 0.0, 0.5);
    e_password = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(e_password), FALSE);
    c_password = gtk_check_button_new_with_label("Keep");
    gtk_table_attach(GTK_TABLE(table), l_password, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    gtk_table_attach(GTK_TABLE(table), e_password, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    gtk_table_attach(GTK_TABLE(table), c_password, 2, 3, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    l_interface = gtk_label_new("Interface:");
    gtk_misc_set_alignment(GTK_MISC(l_interface), 0.0, 0.5);
    e_interface = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(table), l_interface, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    gtk_table_attach(GTK_TABLE(table), e_interface, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    c_duration = gtk_check_button_new_with_label("Duration:");
    hbox_duration = gtk_hbox_new(FALSE, 0);
    s_duration = gtk_spin_button_new(GTK_ADJUSTMENT(gtk_adjustment_new((gdouble)10.0, 1.0, 99999.0, 1.0, 10.0, 0.0)), 0, 0);
    gtk_box_pack_start(GTK_BOX(hbox_duration), s_duration, FALSE, FALSE, 1);
    l_duration_sec = gtk_label_new("seconds");
    gtk_misc_set_alignment(GTK_MISC(l_duration_sec), 0.0, 0.5);
    gtk_box_pack_start(GTK_BOX(hbox_duration), l_duration_sec, FALSE, FALSE, 5);
    gtk_table_attach(GTK_TABLE(table), c_duration, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    gtk_table_attach(GTK_TABLE(table), hbox_duration, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    c_remote = gtk_check_button_new_with_label("Remote continuous scanning");
    gtk_table_attach(GTK_TABLE(table), c_remote, 0, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    c_background = gtk_check_button_new_with_label("Background scanning");
    gtk_table_attach(GTK_TABLE(table), c_background, 0, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    box_status_wrapper = gtk_hbox_new(TRUE, 5);
    gtk_box_pack_start(GTK_BOX(content), box_status_wrapper, FALSE, FALSE, 1);
    box_status = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(box_status_wrapper), box_status, FALSE, FALSE, 1);
    spinner = gtk_spinner_new();
    gtk_box_pack_start(GTK_BOX(box_status), spinner, FALSE, FALSE, 1);
    l_status = gtk_label_new(NULL);
    gtk_label_set_line_wrap(GTK_LABEL(l_status), TRUE);
    gtk_box_pack_start(GTK_BOX(box_status), l_status, FALSE, FALSE, 1);
    box_button = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(box_button), GTK_BUTTONBOX_END);
    gtk_box_set_spacing(GTK_BOX(box_button), 5);
    gtk_box_pack_start(GTK_BOX(content), box_button, FALSE, FALSE, 5);

    b_connect = gtk_button_new_from_stock(GTK_STOCK_OK);
    gtk_container_add(GTK_CONTAINER(box_button), b_connect);
    b_cancel = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
    gtk_container_add(GTK_CONTAINER(box_button), b_cancel);
    b_close = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
    gtk_container_add(GTK_CONTAINER(box_button), b_close);

    g_signal_connect(c_profile, "changed", G_CALLBACK(connection_dialog_profile_changed), NULL);
    g_signal_connect(b_profile_add, "clicked", G_CALLBACK(connection_dialog_profile_add), NULL);
    g_signal_connect(b_profile_remove, "clicked", G_CALLBACK(connection_dialog_profile_remove), NULL);
    g_signal_connect(b_profile_clear, "clicked", G_CALLBACK(connection_dialog_profile_clear), NULL);

    g_signal_connect(e_host, "changed", G_CALLBACK(connection_dialog_profile_unset), NULL);
    g_signal_connect(s_port, "changed", G_CALLBACK(connection_dialog_profile_unset), NULL);
    g_signal_connect(e_login, "changed", G_CALLBACK(connection_dialog_profile_unset), NULL);
    g_signal_connect(e_password, "changed", G_CALLBACK(connection_dialog_profile_unset), NULL);
    g_signal_connect(e_interface, "changed", G_CALLBACK(connection_dialog_profile_unset), NULL);
    g_signal_connect(s_duration, "changed", G_CALLBACK(connection_dialog_profile_unset), NULL);
    g_signal_connect(c_duration, "toggled", G_CALLBACK(connection_dialog_profile_unset), NULL);
    g_signal_connect(c_remote, "toggled", G_CALLBACK(connection_dialog_profile_unset), NULL);
    g_signal_connect(c_background, "toggled", G_CALLBACK(connection_dialog_profile_unset), NULL);

    g_signal_connect(c_duration, "toggled", G_CALLBACK(connection_dialog_duration_toggled), NULL);
    g_signal_connect(c_remote, "toggled", G_CALLBACK(connection_dialog_remote_toggled), NULL);
    g_signal_connect(c_background, "toggled", G_CALLBACK(connection_dialog_background_toggled), NULL);

    g_signal_connect(b_connect, "clicked", G_CALLBACK(connection_dialog_connect), NULL);
    g_signal_connect(b_cancel, "clicked", G_CALLBACK(connection_dialog_cancel), NULL);
    g_signal_connect_swapped(b_close, "clicked", G_CALLBACK(gtk_widget_destroy), dialog);

    g_signal_connect(dialog, "key-press-event", G_CALLBACK(connection_dialog_key), b_connect);
    g_signal_connect(dialog, "destroy", G_CALLBACK(connection_dialog_destroy), NULL);

    gtk_combo_box_set_active(GTK_COMBO_BOX(c_profile), conf_get_interface_last_profile());

    gtk_widget_show_all(dialog);
    connection_dialog_unlock(TRUE);
    gtk_widget_grab_focus(e_host);
}

static void
connection_dialog_destroy(GtkWidget *widget,
                          gpointer   data)
{
    connection_cancel();
    /*
    conf_set_conn_host(gtk_entry_get_text(GTK_ENTRY(e_host)));
    conf_set_conn_port(gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s_port)));
    conf_set_conn_login(gtk_entry_get_text(GTK_ENTRY(e_login)));
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(c_password)))
    {
        conf_set_conn_password(gtk_entry_get_text(GTK_ENTRY(e_password)));
    }
    else
    {
        conf_set_conn_password("");
    }
    conf_set_conn_interface(gtk_entry_get_text(GTK_ENTRY(e_interface)));
    conf_set_conn_duration(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(c_duration)));
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(c_duration)))
    {
        conf_set_conn_duration_time(gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s_duration)));
    }
    conf_set_conn_remote(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(c_remote)));
    */
    conf_set_interface_last_profile(gtk_combo_box_get_active(GTK_COMBO_BOX(c_profile)));
    dialog = NULL;
}

static void
connection_dialog_profile_changed(GtkComboBox *widget,
                                  gpointer     data)
{
    GtkTreeIter iter;
    mtscan_profile_t profile;
    if(gtk_combo_box_get_active_iter(widget, &iter))
    {
        profile = conf_profile_get(&iter);
        profile_reset_flag = FALSE;

        gtk_entry_set_text(GTK_ENTRY(e_host), profile.host);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(s_port), (gdouble)profile.port);
        gtk_entry_set_text(GTK_ENTRY(e_login), profile.login);
        gtk_entry_set_text(GTK_ENTRY(e_password), profile.password);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(c_password), (gboolean)strlen(profile.password));
        gtk_entry_set_text(GTK_ENTRY(e_interface), profile.iface);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(s_duration), (gdouble)profile.duration_time);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(c_duration), profile.duration);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(c_remote), profile.remote);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(c_background), profile.background);

        profile_reset_flag = TRUE;
        conf_profile_free(&profile);
    }
}

static void
connection_dialog_profile_unset(GtkWidget *widget,
                                gpointer   pointer)
{
    if(profile_reset_flag)
    {
        gtk_combo_box_set_active(GTK_COMBO_BOX(c_profile), -1);
    }
}

static void
connection_dialog_duration_toggled(GtkWidget *widget,
                                   gpointer   data)
{
    if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(c_duration)) &&
       gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(c_remote)))
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(c_remote), FALSE);
    }
}

static void
connection_dialog_remote_toggled(GtkWidget *widget,
                                 gpointer   data)
{
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(c_remote)))
    {
        if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(c_duration)))
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(c_duration), TRUE);
        if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(c_background)))
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(c_background), FALSE);
    }
}

static void
connection_dialog_background_toggled(GtkWidget *widget,
                                     gpointer   data)
{
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(c_background)) &&
       gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(c_remote)))
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(c_remote), FALSE);
    }
}

static gboolean
connection_dialog_key(GtkWidget   *widget,
                      GdkEventKey *event,
                      gpointer     button)
{
    switch(gdk_keyval_to_upper(event->keyval))
    {
    case GDK_KEY_Escape:
        gtk_widget_destroy(widget);
        return TRUE;
    case GDK_KEY_Return:
        gtk_button_clicked(GTK_BUTTON(button));
        return TRUE;
    }
    return FALSE;
}

static void
connection_dialog_unlock(gboolean value)
{
    gtk_widget_set_sensitive(c_profile, value);
    gtk_widget_set_sensitive(b_profile_add, value);
    gtk_widget_set_sensitive(b_profile_remove, value);
    gtk_widget_set_sensitive(b_profile_clear, value);
    gtk_widget_set_sensitive(e_host, value);
    gtk_widget_set_sensitive(s_port, value);
    gtk_widget_set_sensitive(e_login, value);
    gtk_widget_set_sensitive(e_password, value);
    gtk_widget_set_sensitive(c_password, value);
    gtk_widget_set_sensitive(e_interface, value);
    gtk_widget_set_sensitive(c_duration, value);
    gtk_widget_set_sensitive(s_duration, value);
    gtk_widget_set_sensitive(c_remote, value);
    gtk_widget_set_sensitive(c_background, value);
    gtk_widget_set_sensitive(b_connect, value);
    gtk_widget_set_sensitive(b_cancel, !value);
    if(value)
    {
        gtk_spinner_stop(GTK_SPINNER(spinner));
        gtk_widget_hide(spinner);
    }
    else
    {
        gtk_spinner_start(GTK_SPINNER(spinner));
        gtk_widget_show(spinner);
    }
}

static void
connection_dialog_status(const gchar *string)
{
    gchar *markup;
    gtk_widget_show(l_status);
    markup = g_markup_printf_escaped("<b>%s</b>", string);
    gtk_label_set_markup(GTK_LABEL(l_status), markup);
    g_free(markup);
}

static void
connection_dialog_profile_add(GtkWidget *widget,
                              gpointer   data)
{
    GtkWidget *dialog_profile, *entry;
    gchar *default_name;
    mtscan_profile_t profile;
    GtkTreeIter iter;

    dialog_profile = gtk_message_dialog_new(GTK_WINDOW(dialog),
                                            GTK_DIALOG_MODAL,
                                            GTK_MESSAGE_QUESTION,
                                            GTK_BUTTONS_OK_CANCEL,
                                            "Profile name:");
    gtk_window_set_title(GTK_WINDOW(dialog_profile), "Add a profile");

    entry = gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(entry), 20);
    gtk_container_add(GTK_CONTAINER(gtk_message_dialog_get_message_area(GTK_MESSAGE_DIALOG(dialog_profile))), entry);

    default_name = g_strdup_printf("%s/%s",
                                   gtk_entry_get_text(GTK_ENTRY(e_host)),
                                   gtk_entry_get_text(GTK_ENTRY(e_interface)));
    gtk_entry_set_text(GTK_ENTRY(entry), default_name);
    g_free(default_name);

    g_signal_connect(dialog_profile, "key-press-event", G_CALLBACK(connection_dialog_profile_add_key), dialog_profile);
    gtk_widget_show_all(dialog_profile);

    if(gtk_dialog_run(GTK_DIALOG(dialog_profile)) == GTK_RESPONSE_OK)
    {
        profile.name = (gchar*)gtk_entry_get_text(GTK_ENTRY(entry));
        profile.host = (gchar*)gtk_entry_get_text(GTK_ENTRY(e_host));
        profile.port = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s_port));
        profile.login = (gchar*)gtk_entry_get_text(GTK_ENTRY(e_login));
        if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(c_password)))
            profile.password = (gchar*)gtk_entry_get_text(GTK_ENTRY(e_password));
        else
            profile.password = "";
        profile.iface = (gchar*)gtk_entry_get_text(GTK_ENTRY(e_interface));
        profile.duration = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(c_duration));
        profile.duration_time = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s_duration));
        profile.remote = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(c_remote));
        profile.background = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(c_background));
        iter = conf_profile_add(&profile);
        g_signal_handlers_block_by_func(G_OBJECT(c_profile), GINT_TO_POINTER(connection_dialog_profile_changed), NULL);
        gtk_combo_box_set_active_iter(GTK_COMBO_BOX(c_profile), &iter);
        g_signal_handlers_unblock_by_func(G_OBJECT(c_profile), GINT_TO_POINTER(connection_dialog_profile_changed), NULL);
    }
    gtk_widget_destroy(dialog_profile);
}

static gboolean
connection_dialog_profile_add_key(GtkWidget   *widget,
                                  GdkEventKey *event,
                                  gpointer     data)
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
connection_dialog_profile_remove(GtkWidget *widget,
                                 gpointer   data)
{
    GtkTreeIter iter;
    if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(c_profile), &iter))
    {
        if(ui_dialog_yesno(GTK_WINDOW(dialog), "Are you sure you want to remove the selected profile?") == UI_DIALOG_YES)
            gtk_list_store_remove(GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(c_profile))), &iter);
    }
}

static void
connection_dialog_profile_clear(GtkWidget *widget,
                                gpointer   data)
{
    if(ui_dialog_yesno(GTK_WINDOW(dialog), "Are you sure you want to remove ALL profiles?") == UI_DIALOG_YES)
        gtk_list_store_clear(GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(c_profile))));
}

static void
connection_dialog_connect(GtkWidget *widget,
                          gpointer   data)
{
    const gchar *hostname = gtk_entry_get_text(GTK_ENTRY(e_host));
    gint port = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s_port));
    const gchar *login = gtk_entry_get_text(GTK_ENTRY(e_login));
    const gchar *password = gtk_entry_get_text(GTK_ENTRY(e_password));
    const gchar *iface = gtk_entry_get_text(GTK_ENTRY(e_interface));
    gboolean remote = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(c_remote));
    gboolean background = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(c_background));
    gboolean duration_enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(c_duration));
    gint duration = (duration_enabled ? gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s_duration)) : 0);

    if(!strlen(hostname))
    {
        connection_dialog_status("No hostname specified.");
        gtk_widget_grab_focus(e_host);
        return;
    }

    if(!strlen(login))
    {
        connection_dialog_status("No login specified.");
        gtk_widget_grab_focus(e_login);
        return;
    }

    if(!strlen(iface))
    {
        connection_dialog_status("No interface specified.");
        gtk_widget_grab_focus(e_interface);
        return;
    }

    if(remote)
        mtscan_model_set_active_timeout(ui.model, duration);

    conn = conn_new(hostname, port, login, password, iface, duration, remote, background);
    connection_dialog_unlock(FALSE);
    g_thread_unref(g_thread_new("mt_ssh_thread", mt_ssh_thread, conn));
}

static void
connection_dialog_cancel(GtkWidget *widget,
                         gpointer   data)
{
    connection_cancel();
    connection_dialog_unlock(TRUE);
    connection_dialog_status("Connection cancelled.");
}

static void
connection_cancel()
{
    if(conn && !ui.connected)
    {
        conn->canceled = TRUE;
        conn = NULL;
        ui_disconnected();
    }
}

gboolean
connection_callback(gpointer ptr)
{
    /* Gets final state of conn_t ptr and frees up the memory */
    mtscan_conn_t *data = (mtscan_conn_t*)ptr;
    if(conn == data)
    {
        ui_disconnected();
        conn = NULL;

        if(dialog)
        {
            connection_dialog_unlock(TRUE);
            if(!data->canceled)
            {
                switch(data->return_state)
                {
                case CONN_ERR_SSH_NEW:
                    connection_dialog_status("libssh init error");
                    break;
                case CONN_ERR_SSH_SET_OPTIONS:
                    connection_dialog_status("libssh set options error");
                    break;
                case CONN_ERR_SSH_CONNECT:
                    connection_dialog_status((data->err?data->err:"Unable to connect."));
                    break;
                case CONN_ERR_SSH_VERIFY:
                    connection_dialog_status("Unable to verifiy the SSH server.");
                    break;
                case CONN_ERR_SSH_AUTH:
                    connection_dialog_status("Permission denied, please try again.");
                    break;
                case CONN_ERR_SSH_CHANNEL_NEW:
                    connection_dialog_status("libssh channel init error");
                    break;
                case CONN_ERR_SSH_CHANNEL_OPEN:
                    connection_dialog_status("libssh channel open error");
                    break;
                case CONN_ERR_SSH_CHANNEL_REQ_PTY_SIZE:
                    connection_dialog_status("libssh channel request pty size error");
                    break;
                case CONN_ERR_SSH_CHANNEL_REQ_SHELL:
                    connection_dialog_status("libssh channel request shell error");
                    break;
                }
            }
        }
    }

    conn_free(data);
    return FALSE;
}

gboolean
connection_callback_info(gpointer ptr)
{
    mtscan_conn_msg_t *data = (mtscan_conn_msg_t*)ptr;
    gchar *string;

    if(data->conn == conn)
    {
        switch(data->type)
        {
        case CONN_MSG_CONNECTING:
            connection_dialog_status("Connecting...");
            break;

        case CONN_MSG_AUTHENTICATING:
            connection_dialog_status("Authenticating...");
            break;

        case CONN_MSG_AUTH_VERIFY:
            if(ui_dialog_yesno(GTK_WINDOW(dialog), (gchar*)data->data) == UI_DIALOG_YES)
                g_async_queue_push(conn->queue, conn_command_new(COMMAND_AUTH, NULL));
            else
                g_async_queue_push(conn->queue, conn_command_new(COMMAND_DISCONNECT, NULL));
            break;

        case CONN_MSG_AUTH_ERROR:
            ui_dialog(GTK_WINDOW(dialog), GTK_MESSAGE_ERROR, APP_NAME, "<b>SSH error:</b>\n%s", (gchar*)data->data);
            break;

        case CONN_MSG_CONNECTED:
            connection_dialog_status("Starting scan...");
            break;

        case CONN_MSG_SCANNING:
            ui_connected(conn->login, conn->hostname, conn->iface);
            gtk_widget_destroy(dialog);
            break;

        case CONN_MSG_SCANNING_ERROR:
            if(dialog)
            {
                g_async_queue_push(conn->queue, conn_command_new(COMMAND_DISCONNECT, NULL));
                string = g_strdup_printf("Scanning error: %s", (gchar*)data->data);
                connection_dialog_status(string);
                g_free(string);
            }
            else
            {
                ui_dialog(GTK_WINDOW(ui.window), GTK_MESSAGE_ERROR, APP_NAME, "<b>Scanning error:</b>\n%s", (gchar*)data->data);
                ui_update_scanning_state(FALSE);
            }
            break;
        }
    }
    conn_msg_free(data);
    return FALSE;
}
