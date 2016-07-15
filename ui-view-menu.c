#include <string.h>
#include <math.h>
#include "ui.h"
#include "ui-view.h"
#include "conn.h"
#include "model.h"

static void ui_view_popup_addr(GtkWidget*, gpointer);
static void ui_view_popup_oui(GtkWidget*, gpointer);
static void ui_view_popup_map(GtkWidget*, gpointer);
static void ui_view_popup_lock(GtkWidget*, gpointer);
static void ui_view_popup_remove(GtkWidget*, gpointer);

void
ui_view_popup_menu(GtkWidget      *treeview,
                   GdkEventButton *event,
                   gpointer        data)
{
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
    GtkTreeModel *store;
    GtkTreeIter iter;
    gchar *address;
    gint frequency;
    gdouble latitude, longitude;

    if(!gtk_tree_selection_get_selected(selection, &store, &iter))
        return;

    gtk_tree_model_get(store, &iter, COL_ADDRESS, &address,
                                     COL_FREQUENCY, &frequency,
                                     COL_LATITUDE, &latitude,
                                     COL_LONGITUDE, &longitude,
                                     -1);

    GtkWidget *menu = gtk_menu_new();

    GtkWidget *item_header = gtk_image_menu_item_new_with_label(model_format_address(address));
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item_header), gtk_image_new_from_stock(GTK_STOCK_NETWORK, GTK_ICON_SIZE_MENU));
    g_signal_connect(item_header, "activate", (GCallback)ui_view_popup_addr, treeview);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_header);

    GtkWidget *item_lookup_oui = gtk_image_menu_item_new_with_label("Look up OUI");
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item_lookup_oui), gtk_image_new_from_stock(GTK_STOCK_INFO, GTK_ICON_SIZE_MENU));
    g_signal_connect(item_lookup_oui, "activate", (GCallback)ui_view_popup_oui, treeview);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_lookup_oui);

    g_free(address);

    if(frequency)
    {
        gchar *freq = g_strdup_printf("Lock to %s MHz", model_format_frequency(frequency));
        GtkWidget *item_lock_to_freq = gtk_menu_item_new_with_label(freq);
        g_signal_connect(item_lock_to_freq, "activate", (GCallback)ui_view_popup_lock, treeview);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_lock_to_freq);
        g_free(freq);
    }

    if(!isnan(latitude) && !isnan(longitude))
    {
        GtkWidget *item_show_on_map = gtk_menu_item_new_with_label("Show on a map");
        g_signal_connect(item_show_on_map, "activate", (GCallback)ui_view_popup_map, treeview);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_show_on_map);
    }

    GtkWidget *item_remove = gtk_image_menu_item_new_with_label("Remove");
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item_remove), gtk_image_new_from_stock(GTK_STOCK_REMOVE, GTK_ICON_SIZE_MENU));
    g_signal_connect(item_remove, "activate", (GCallback)ui_view_popup_remove, treeview);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_remove);

    gtk_widget_show_all(menu);

    gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
                   (event != NULL) ? event->button : 0,
                   gdk_event_get_time((GdkEvent*)event));
}

static void
ui_view_popup_addr(GtkWidget *menuitem,
                   gpointer   data)
{
    GtkWidget *treeview = GTK_WIDGET(data);
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
    GtkTreeModel *store;
    GtkTreeIter iter;
    gchar *address;
    GtkClipboard *clipboard;

    if(!gtk_tree_selection_get_selected(selection, &store, &iter))
        return;

    gtk_tree_model_get(store, &iter, COL_ADDRESS, &address, -1);

    clipboard = gtk_widget_get_clipboard(treeview, GDK_SELECTION_CLIPBOARD);
    gtk_clipboard_set_text(clipboard, model_format_address(address), -1);

    g_free(address);
}

static void
ui_view_popup_oui(GtkWidget *menuitem,
                  gpointer   data)
{
    GtkTreeView *treeview = GTK_TREE_VIEW(data);
    GtkTreeSelection *selection = gtk_tree_view_get_selection(treeview);
    GtkTreeModel *store;
    GtkTreeIter iter;
    gchar *address;

    if(!gtk_tree_selection_get_selected(selection, &store, &iter))
        return;

    gtk_tree_model_get(store, &iter, COL_ADDRESS, &address, -1);
    if(address && strlen(address) == 12)
    {
        gchar *uri = g_strdup_printf("https://standards.ieee.org/cgi-bin/ouisearch?%c%c%c%c%c%c",
                                     address[0], address[1],
                                     address[2], address[3],
                                     address[4], address[5]);
        ui_show_uri(uri);
        g_free(uri);
    }
    g_free(address);
}

static void
ui_view_popup_map(GtkWidget *menuitem,
                  gpointer   data)
{
    GtkTreeView *treeview = GTK_TREE_VIEW(data);
    GtkTreeSelection *selection = gtk_tree_view_get_selection(treeview);
    GtkTreeModel *store;
    GtkTreeIter iter;
    gdouble latitude, longitude;

    if(!gtk_tree_selection_get_selected(selection, &store, &iter))
        return;

    gtk_tree_model_get(store, &iter,
                       COL_LATITUDE, &latitude,
                       COL_LONGITUDE, &longitude,
                       -1);

    if(!isnan(latitude) && !isnan(longitude))
    {
        gchar *uri = g_strdup_printf("https://maps.google.com/maps?z=12&q=loc:%.6f+%.6f",
                                     latitude, longitude);
        ui_show_uri(uri);
        g_free(uri);
    }
}

static void
ui_view_popup_lock(GtkWidget *menuitem,
                   gpointer   data)
{
    GtkTreeView *treeview = GTK_TREE_VIEW(data);
    GtkTreeSelection *selection = gtk_tree_view_get_selection(treeview);
    GtkTreeModel *store;
    GtkTreeIter iter;
    gint frequency;

    if(!gtk_tree_selection_get_selected(selection, &store, &iter))
        return;

    gtk_tree_model_get(store, &iter, COL_FREQUENCY, &frequency, -1);

    if(!frequency || !conn)
        return;

    g_async_queue_push(conn->queue,
                       conn_command_new(COMMAND_SET_SCANLIST,
                                        g_strdup(model_format_frequency(frequency))));
}

static void
ui_view_popup_remove(GtkWidget *menuitem,
                     gpointer   data)
{
    GtkTreeView *treeview = GTK_TREE_VIEW(data);
    GtkTreeSelection *selection = gtk_tree_view_get_selection(treeview);
    GtkTreeModel *store;
    GtkTreeIter iter;

    if(!gtk_tree_selection_get_selected(selection, &store, &iter))
        return;

    ui_view_remove_iter(treeview, store, &iter);
}
