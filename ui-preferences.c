#include <gdk/gdkkeysyms.h>
#include <string.h>
#include "conf.h"
#include "ui.h"
#include "ui-view.h"
#include "gps.h"

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
static void ui_preferences_load(ui_preferences_t*);
static void ui_preferences_apply(GtkWidget*, gpointer);

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

static void
ui_preferences_load(ui_preferences_t *p)
{
    /* General */
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(p->s_general_icon_size), conf_get_preferences_icon_size());
    gtk_combo_box_set_active(GTK_COMBO_BOX(p->c_general_search_column), conf_get_preferences_search_column());
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->x_general_latlon_column), conf_get_preferences_latlon_column());
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->x_general_azimuth_column), conf_get_preferences_azimuth_column());
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->x_general_signals), conf_get_preferences_signals());

    /* GPS */
    gtk_entry_set_text(GTK_ENTRY(p->e_gps_hostname), conf_get_preferences_gps_hostname());
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(p->s_gps_tcp_port), conf_get_preferences_gps_tcp_port());
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

    /* --- */
    gtk_widget_destroy(p->window);
}
