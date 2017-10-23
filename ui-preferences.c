#include <gdk/gdkkeysyms.h>
#include <string.h>
#include "conf.h"
#include "ui.h"
#include "ui-view.h"
#include "gps.h"
#include "ui-dialogs.h"

typedef struct ui_preferences
{
    GtkWidget *window;
    GtkWidget *content;
    GtkWidget *notebook;

    GtkWidget *page_general;
    GtkWidget *table_general;
    GtkWidget *l_general_icon_size;
    GtkWidget *s_general_icon_size;
    GtkWidget *l_general_search_column;
    GtkWidget *c_general_search_column;
    GtkWidget *x_general_latlon_column;
    GtkWidget *x_general_azimuth_column;
    GtkWidget *x_general_signals;

    GtkWidget *page_gps;
    GtkWidget *table_gps;
    GtkWidget *l_gps_hostname;
    GtkWidget *e_gps_hostname;
    GtkWidget *l_gps_tcp_port;
    GtkWidget *s_gps_tcp_port;

    GtkWidget *page_blacklist;
    GtkWidget *box_blacklist;
    GtkWidget *box_blacklist_buttons;
    GtkWidget *x_blacklist_enabled;
    GtkWidget *t_blacklist;
    GtkWidget *s_blacklist;
    GtkWidget *b_blacklist_add;
    GtkWidget *b_blacklist_remove;
    GtkWidget *b_blacklist_clear;

    GtkWidget *box_button;
    GtkWidget *b_apply;
    GtkWidget *b_cancel;
} ui_preferences_t;

enum
{
    P_COL_ID,
    P_COL_NAME,
    P_COL_HIDDEN,
    P_COL_COUNT
};

static gboolean ui_preferences_key(GtkWidget*, GdkEventKey*, gpointer);
static gboolean ui_preferences_key_blacklist(GtkWidget*, GdkEventKey*, gpointer);
static gboolean ui_preferences_key_blacklist_add(GtkWidget*, GdkEventKey*, gpointer);
static void ui_preferences_blacklist_add(GtkWidget*, gpointer);
static void ui_preferences_blacklist_remove(GtkWidget*, gpointer);
static void ui_preferences_blacklist_clear(GtkWidget*, gpointer);
static void ui_preferences_load(ui_preferences_t*);
static void ui_preferences_apply(GtkWidget*, gpointer);
static void remove_char(gchar*, gchar);

void
ui_preferences_dialog()
{
    static ui_preferences_t p;
    gint row;

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

    p.table_general = gtk_table_new(6, 2, TRUE);
    gtk_table_set_homogeneous(GTK_TABLE(p.table_general), FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(p.table_general), 4);
    gtk_table_set_col_spacings(GTK_TABLE(p.table_general), 4);
    gtk_container_add(GTK_CONTAINER(p.page_general), p.table_general);

    row = 0;
    p.l_general_icon_size = gtk_label_new("Icon size [px]:");
    gtk_misc_set_alignment(GTK_MISC(p.l_general_icon_size), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(p.table_general), p.l_general_icon_size, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    p.s_general_icon_size = gtk_spin_button_new(GTK_ADJUSTMENT(gtk_adjustment_new(0.0, 16.0, 64.0, 1.0, 10.0, 0.0)), 0, 0);
    gtk_table_attach(GTK_TABLE(p.table_general), p.s_general_icon_size, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    p.l_general_search_column = gtk_label_new("Search column:");
    gtk_misc_set_alignment(GTK_MISC(p.l_general_search_column), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(p.table_general), p.l_general_search_column, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    p.c_general_search_column = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(p.c_general_search_column), "Address");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(p.c_general_search_column), "SSID");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(p.c_general_search_column), "Radio name");
    gtk_table_attach(GTK_TABLE(p.table_general), p.c_general_search_column, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    p.x_general_latlon_column = gtk_check_button_new_with_label("Show latitude & longitude column");
    gtk_table_attach(GTK_TABLE(p.table_general), p.x_general_latlon_column, 0, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    p.x_general_azimuth_column = gtk_check_button_new_with_label("Show azimuth column");
    gtk_table_attach(GTK_TABLE(p.table_general), p.x_general_azimuth_column, 0, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    gtk_table_attach(GTK_TABLE(p.table_general), gtk_hseparator_new(), 0, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    p.x_general_signals = gtk_check_button_new_with_label("Record all signal samples");
    gtk_table_attach(GTK_TABLE(p.table_general), p.x_general_signals, 0, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    /* GPS */
    p.page_gps = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(p.page_gps), 4);
    gtk_notebook_append_page(GTK_NOTEBOOK(p.notebook), p.page_gps, gtk_label_new("GPSd"));
    gtk_container_child_set(GTK_CONTAINER(p.notebook), p.page_gps, "tab-expand", FALSE, "tab-fill", FALSE, NULL);

    p.table_gps = gtk_table_new(2, 2, TRUE);
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
    p.l_gps_tcp_port = gtk_label_new("TCP Port:");
    gtk_misc_set_alignment(GTK_MISC(p.l_gps_tcp_port), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(p.table_gps), p.l_gps_tcp_port, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    p.s_gps_tcp_port = gtk_spin_button_new(GTK_ADJUSTMENT(gtk_adjustment_new(2947.0, 1024.0, 65535.0, 1.0, 10.0, 0.0)), 0, 0);
    gtk_table_attach(GTK_TABLE(p.table_gps), p.s_gps_tcp_port, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    /* Blacklist */
    p.page_blacklist = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(p.page_blacklist), 4);
    gtk_notebook_append_page(GTK_NOTEBOOK(p.notebook), p.page_blacklist, gtk_label_new("Blacklist"));
    gtk_container_child_set(GTK_CONTAINER(p.notebook), p.page_blacklist, "tab-expand", FALSE, "tab-fill", FALSE, NULL);

    p.box_blacklist = gtk_vbox_new(FALSE, 4);
    gtk_container_add(GTK_CONTAINER(p.page_blacklist), p.box_blacklist);

    p.x_blacklist_enabled = gtk_check_button_new_with_label("Enable blacklist");
    gtk_box_pack_start(GTK_BOX(p.box_blacklist), p.x_blacklist_enabled, FALSE, FALSE, 0);

    p.t_blacklist = gtk_tree_view_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(p.t_blacklist), -1, "Address", gtk_cell_renderer_text_new(), "text", 0, NULL);
    g_signal_connect(p.t_blacklist, "key-press-event", G_CALLBACK(ui_preferences_key_blacklist), &p);

    p.s_blacklist = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(p.s_blacklist), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(p.s_blacklist), p.t_blacklist);
    gtk_box_pack_start(GTK_BOX(p.box_blacklist), p.s_blacklist, TRUE, TRUE, 0);

    p.box_blacklist_buttons = gtk_hbox_new(TRUE, 0);
    gtk_box_pack_start(GTK_BOX(p.box_blacklist), p.box_blacklist_buttons, FALSE, FALSE, 0);

    p.b_blacklist_add = gtk_button_new_from_stock(GTK_STOCK_ADD);
    g_signal_connect(p.b_blacklist_add, "clicked", G_CALLBACK(ui_preferences_blacklist_add), p.t_blacklist);
    gtk_box_pack_start(GTK_BOX(p.box_blacklist_buttons), p.b_blacklist_add, TRUE, TRUE, 0);

    p.b_blacklist_remove = gtk_button_new_from_stock(GTK_STOCK_REMOVE);
    g_signal_connect(p.b_blacklist_remove, "clicked", G_CALLBACK(ui_preferences_blacklist_remove), p.t_blacklist);
    gtk_box_pack_start(GTK_BOX(p.box_blacklist_buttons), p.b_blacklist_remove, TRUE, TRUE, 0);

    p.b_blacklist_clear = gtk_button_new_from_stock(GTK_STOCK_CLEAR);
    g_signal_connect(p.b_blacklist_clear, "clicked", G_CALLBACK(ui_preferences_blacklist_clear), p.t_blacklist);
    gtk_box_pack_start(GTK_BOX(p.box_blacklist_buttons), p.b_blacklist_clear, TRUE, TRUE, 0);

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
ui_preferences_key_blacklist(GtkWidget   *widget,
                             GdkEventKey *event,
                             gpointer     data)
{
    ui_preferences_t *p = (ui_preferences_t*)data;
    if(event->keyval == GDK_Delete)
    {
        gtk_button_clicked(GTK_BUTTON(p->b_blacklist_remove));
        return TRUE;
    }
    return FALSE;
}

static gboolean
ui_preferences_key_blacklist_add(GtkWidget   *widget,
                                 GdkEventKey *event,
                                 gpointer     data)
{
    GtkDialog *dialog = GTK_DIALOG(data);

    if(event->keyval == GDK_Return)
    {
        gtk_dialog_response(dialog, GTK_RESPONSE_OK);
        return TRUE;
    }
    return FALSE;
}

static void
ui_preferences_blacklist_add(GtkWidget *widget,
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

    dialog = gtk_message_dialog_new_with_markup(GTK_WINDOW(toplevel),
                                                GTK_DIALOG_MODAL,
                                                GTK_MESSAGE_QUESTION,
                                                GTK_BUTTONS_OK_CANCEL,
                                                "<big>Enter network address</big>\neg. 01:23:45:67:89:AB\nor 0123456789AB");
    gtk_window_set_title(GTK_WINDOW(dialog), "Blacklist");
    entry = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(entry), 17);
    gtk_container_add(GTK_CONTAINER(gtk_message_dialog_get_message_area(GTK_MESSAGE_DIALOG(dialog))), entry);
    g_signal_connect(entry, "key-press-event", G_CALLBACK(ui_preferences_key_blacklist_add), dialog);
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

        if(!match)
        {
            ui_dialog(GTK_WINDOW(toplevel),
                      GTK_MESSAGE_ERROR,
                      "Blacklist",
                      "<big>Invalid address format:</big>\n%s", value);
        }
    }

    gtk_widget_destroy(dialog);

    if(match)
    {
        gtk_list_store_insert_with_values(GTK_LIST_STORE(gtk_tree_view_get_model(treeview)), NULL, -1, 0, match, -1);
        g_free(match);
    }
}

static void
ui_preferences_blacklist_remove(GtkWidget *widget,
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
ui_preferences_blacklist_clear(GtkWidget *widget,
                               gpointer   data)
{
    GtkTreeView *treeview = GTK_TREE_VIEW(data);
    GtkTreeModel *model = gtk_tree_view_get_model(treeview);
    gtk_list_store_clear(GTK_LIST_STORE(model));
}

static void
ui_preferences_load(ui_preferences_t *p)
{
    GtkListStore *model;

    /* General */
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(p->s_general_icon_size), conf_get_preferences_icon_size());
    gtk_combo_box_set_active(GTK_COMBO_BOX(p->c_general_search_column), conf_get_preferences_search_column());
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->x_general_latlon_column), conf_get_preferences_latlon_column());
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->x_general_azimuth_column), conf_get_preferences_azimuth_column());
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->x_general_signals), conf_get_preferences_signals());

    /* GPS */
    gtk_entry_set_text(GTK_ENTRY(p->e_gps_hostname), conf_get_preferences_gps_hostname());
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(p->s_gps_tcp_port), conf_get_preferences_gps_tcp_port());

    /* Blacklist */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->x_blacklist_enabled), conf_get_preferences_blacklist_enabled());
    model = conf_get_preferences_blacklist_as_liststore();
    gtk_tree_view_set_model(GTK_TREE_VIEW(p->t_blacklist), GTK_TREE_MODEL(model));
    g_object_unref(model);
}

static void
ui_preferences_apply(GtkWidget *widget,
                     gpointer   data)
{
    ui_preferences_t *p = (ui_preferences_t*)data;
    gint new_icon_size;
    gint new_search_column;
    gboolean new_latlon_column;
    gboolean new_azimuth_column;
    const gchar *new_gps_hostname;
    gint new_gps_tcp_port;

    /* General */
    new_icon_size = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(p->s_general_icon_size));
    if(new_icon_size != conf_get_preferences_icon_size())
    {
        conf_set_preferences_icon_size(new_icon_size);
        ui_view_set_icon_size(ui.treeview, new_icon_size);
    }

    new_search_column = gtk_combo_box_get_active(GTK_COMBO_BOX(p->c_general_search_column));
    if(new_search_column != conf_get_preferences_search_column())
    {
        conf_set_preferences_search_column(new_search_column);
        ui_view_configure(ui.treeview);
    }

    new_latlon_column = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p->x_general_latlon_column));
    if(new_latlon_column != conf_get_preferences_latlon_column())
    {
        conf_set_preferences_latlon_column(new_latlon_column);
        ui_view_latlon_column(ui.treeview, new_latlon_column);
    }

    new_azimuth_column = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p->x_general_azimuth_column));
    if(new_azimuth_column != conf_get_preferences_azimuth_column())
    {
        conf_set_preferences_azimuth_column(new_azimuth_column);
        ui_view_azimuth_column(ui.treeview, new_azimuth_column);
    }

    conf_set_preferences_signals(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p->x_general_signals)));

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

    /* Blacklist */
    conf_set_preferences_blacklist_enabled(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p->x_blacklist_enabled)));
    conf_set_preferences_blacklist_from_liststore(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(p->t_blacklist))));

    /* --- */
    gtk_widget_destroy(p->window);
}

static void
remove_char(gchar *str,
            gchar  c)
{
    gchar *pr = str;
    gchar *pw = str;
    while(*pr)
    {
        *pw = *pr++;
        pw += (*pw != c);
    }
    *pw = 0;
}
