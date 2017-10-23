#include <string.h>
#include <math.h>
#include "ui.h"
#include "ui-view.h"
#include "ui-dialogs.h"
#include "log.h"
#include "conn.h"
#include "scanlist.h"
#include "conf.h"
#include "misc.h"

static GtkWidget* ui_view_menu_create(GtkTreeView*, gint, const gchar*, gint, gboolean, gboolean);
static void ui_view_menu_addr(GtkWidget*, gpointer);
static void ui_view_menu_lock(GtkWidget*, gpointer);
static gboolean ui_view_menu_lock_foreach(gpointer, gpointer, gpointer);
static void ui_view_menu_oui(GtkWidget*, gpointer);
static void ui_view_menu_map(GtkWidget*, gpointer);
static void ui_view_menu_scanlist(GtkWidget*, gpointer);
static void ui_view_menu_blacklist(GtkWidget*, gpointer);
static void ui_view_menu_save_as(GtkWidget*, gpointer);
static void ui_view_menu_remove(GtkWidget*, gpointer);

void
ui_view_menu(GtkWidget      *treeview,
             GdkEventButton *event,
             gpointer        user_data)
{
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
    GtkTreeModel *model;
    GtkTreeIter iter;
    GList *list = gtk_tree_selection_get_selected_rows(selection, &model);
    gint count;
    gchar *address;
    gint frequency;
    gdouble latitude, longitude;
    GtkWidget *menu;

    if(!list)
        return;

    count = g_list_length(list);
    if(count == 1)
    {
        gtk_tree_model_get_iter(model, &iter, (GtkTreePath*)list->data);
        gtk_tree_model_get(model, &iter, COL_ADDRESS, &address,
                                         COL_FREQUENCY, &frequency,
                                         COL_LATITUDE, &latitude,
                                         COL_LONGITUDE, &longitude,
                                         -1);

        menu = ui_view_menu_create(GTK_TREE_VIEW(treeview),
                                   count,
                                   address,
                                   frequency,
                                   conf_get_preferences_blacklist(address),
                                   (!isnan(latitude) && !isnan(longitude)));
        g_free(address);
    }
    else
    {
        menu = ui_view_menu_create(GTK_TREE_VIEW(treeview), count, NULL, 0, FALSE, FALSE);
    }

    gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
                   (event != NULL) ? event->button : 0,
                   gdk_event_get_time((GdkEvent*)event));

    g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
    g_list_free(list);
}

static GtkWidget*
ui_view_menu_create(GtkTreeView *treeview,
                    gint         count,
                    const gchar *address,
                    gint         frequency,
                    gboolean     blacklist,
                    gboolean     position)
{
    GtkWidget *menu = gtk_menu_new();
    GtkWidget *item_header;
    GtkWidget *item_sep;
    GtkWidget *item_lookup_oui;
    GtkWidget *item_show_on_map;
    GtkWidget *item_lock_to_freq;
    GtkWidget *item_scanlist;
    GtkWidget *item_blacklist;
    GtkWidget *item_save_as;
    GtkWidget *item_remove;
    gchar* string;

    /* Header */
    string = (!address ? g_strdup_printf("%d networks", count) : NULL);
    item_header = gtk_image_menu_item_new_with_label(string ? string : model_format_address(address));
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item_header), gtk_image_new_from_stock(GTK_STOCK_NETWORK, GTK_ICON_SIZE_MENU));
    g_signal_connect(item_header, "activate", (GCallback)ui_view_menu_addr, treeview);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_header);
    gtk_widget_set_sensitive(item_header, GPOINTER_TO_INT(address));
    g_free(string);

    /* Separator */
    item_sep = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_sep);

    /* Frequency lock */
    string = (count == 1 ? g_strdup_printf("Lock to %s MHz",  model_format_frequency(frequency)) : NULL);
    item_lock_to_freq = gtk_image_menu_item_new_with_label(string ? string : "Lock to frequencies");
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item_lock_to_freq), gtk_image_new_from_stock(GTK_STOCK_APPLY, GTK_ICON_SIZE_MENU));
    g_signal_connect(item_lock_to_freq, "activate", (GCallback)ui_view_menu_lock, treeview);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_lock_to_freq);
    g_free(string);

    /* OUI lookup */
    if(count == 1)
    {
        item_lookup_oui = gtk_image_menu_item_new_with_label("Look up OUI");
        gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item_lookup_oui), gtk_image_new_from_stock(GTK_STOCK_INFO, GTK_ICON_SIZE_MENU));
        g_signal_connect(item_lookup_oui, "activate", (GCallback)ui_view_menu_oui, treeview);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_lookup_oui);
    }

    /* Show network on a map */
    if(position)
    {
        item_show_on_map = gtk_image_menu_item_new_with_label("Show on a map");
        gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item_show_on_map), gtk_image_new_from_icon_name("mtscan-gps", GTK_ICON_SIZE_MENU));
        g_signal_connect(item_show_on_map, "activate", (GCallback)ui_view_menu_map, treeview);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_show_on_map);
    }

    /* Select in custom scan-list */
    item_scanlist = gtk_image_menu_item_new_with_label("Select in scan-list");
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item_scanlist), gtk_image_new_from_stock(GTK_STOCK_EDIT, GTK_ICON_SIZE_MENU));
    g_signal_connect(item_scanlist, "activate", (GCallback)ui_view_menu_scanlist, treeview);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_scanlist);

    /* Add or remove from blacklist */
    item_blacklist = gtk_image_menu_item_new_with_label(blacklist ? "Remove from blacklist" : "Add to blacklist");
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item_blacklist), gtk_image_new_from_stock((blacklist ? GTK_STOCK_UNDELETE : GTK_STOCK_STOP), GTK_ICON_SIZE_MENU));
    g_signal_connect(item_blacklist, "activate", (GCallback)ui_view_menu_blacklist, treeview);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_blacklist);

    /* Save selected networks as */
    item_save_as = gtk_image_menu_item_new_with_label("Save as...");
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item_save_as), gtk_image_new_from_stock(GTK_STOCK_SAVE_AS, GTK_ICON_SIZE_MENU));
    g_signal_connect(item_save_as, "activate", (GCallback)ui_view_menu_save_as, treeview);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_save_as);

    /* Remove selected networks */
    item_remove = gtk_image_menu_item_new_with_label("Remove");
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item_remove), gtk_image_new_from_stock(GTK_STOCK_REMOVE, GTK_ICON_SIZE_MENU));
    g_signal_connect(item_remove, "activate", (GCallback)ui_view_menu_remove, treeview);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_remove);

    gtk_widget_show_all(menu);
    return menu;
}

static void
ui_view_menu_addr(GtkWidget *menuitem,
                  gpointer   user_data)
{
    GtkTreeView *treeview = GTK_TREE_VIEW(user_data);
    GtkTreeSelection *selection = gtk_tree_view_get_selection(treeview);
    GtkTreeModel *model;
    GtkTreeIter iter;
    GList *list = gtk_tree_selection_get_selected_rows(selection, &model);
    GtkClipboard *clipboard;
    gchar *address;

    if(!list)
        return;

    gtk_tree_model_get_iter(model, &iter, (GtkTreePath*)list->data);
    gtk_tree_model_get(model, &iter, COL_ADDRESS, &address, -1);

    clipboard = gtk_widget_get_clipboard(GTK_WIDGET(treeview), GDK_SELECTION_CLIPBOARD);
    gtk_clipboard_set_text(clipboard, model_format_address(address), -1);

    g_free(address);
    g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
    g_list_free(list);
}

static void
ui_view_menu_lock(GtkWidget *menuitem,
                  gpointer   user_data)
{
    GtkTreeView *treeview = GTK_TREE_VIEW(user_data);
    GtkTreeSelection *selection = gtk_tree_view_get_selection(treeview);
    GtkTreeModel *model;
    GtkTreeIter iter;
    GTree *tree;
    GList *list;
    GList *i;
    GString *str;
    gint frequency;
    gchar *output;

    if(!conn)
        return;

    list = gtk_tree_selection_get_selected_rows(selection, &model);
    if(!list)
        return;

    tree = g_tree_new((GCompareFunc)gintcmp);
    for(i=list; i; i=i->next)
    {
        gtk_tree_model_get_iter(model, &iter, (GtkTreePath*)i->data);
        gtk_tree_model_get(model, &iter, COL_FREQUENCY, &frequency, -1);
        if(frequency)
            g_tree_insert(tree, GINT_TO_POINTER(frequency), NULL);
    }

    str = g_string_new("");
    g_tree_foreach(tree, ui_view_menu_lock_foreach, str);

    output = g_string_free(str, FALSE);
    if(output[0] != '\0')
    {
        /* Remove ending comma */
        output[strlen(output) - 1] = '\0';
        g_async_queue_push(conn->queue,
                           conn_command_new(COMMAND_SET_SCANLIST, output));
    }
    else
    {
        /* No frequencies selected */
        g_free(output);
    }

    g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
    g_list_free(list);
    g_tree_unref(tree);
}

static gboolean
ui_view_menu_lock_foreach(gpointer key,
                          gpointer value,
                          gpointer data)
{
    GString *str = (GString*)data;
    gint frequency = GPOINTER_TO_INT(key);
    g_string_append_printf(str, "%d,", frequency/1000);
    return FALSE;
}

static void
ui_view_menu_oui(GtkWidget *menuitem,
                 gpointer   user_data)
{
    GtkTreeView *treeview = GTK_TREE_VIEW(user_data);
    GtkTreeSelection *selection = gtk_tree_view_get_selection(treeview);
    GtkTreeModel *model;
    GtkTreeIter iter;
    GList *list = gtk_tree_selection_get_selected_rows(selection, &model);
    gchar *address;

    if(!list)
        return;

    gtk_tree_model_get_iter(model, &iter, (GtkTreePath*)list->data);
    gtk_tree_model_get(model, &iter, COL_ADDRESS, &address, -1);

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
    g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
    g_list_free(list);
}

static void
ui_view_menu_map(GtkWidget *menuitem,
                 gpointer   user_data)
{
    GtkTreeView *treeview = GTK_TREE_VIEW(user_data);
    GtkTreeSelection *selection = gtk_tree_view_get_selection(treeview);
    GtkTreeModel *model;
    GtkTreeIter iter;
    GList *list = gtk_tree_selection_get_selected_rows(selection, &model);
    gdouble latitude, longitude;

    if(!list)
        return;

    gtk_tree_model_get_iter(model, &iter, (GtkTreePath*)list->data);
    gtk_tree_model_get(model, &iter,
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
    g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
    g_list_free(list);
}

static void
ui_view_menu_scanlist(GtkWidget *menuitem,
                      gpointer   user_data)
{
    GtkTreeView *treeview = GTK_TREE_VIEW(user_data);
    GtkTreeSelection *selection = gtk_tree_view_get_selection(treeview);
    GtkTreeModel *model;
    GtkTreeIter iter;
    GList *list = gtk_tree_selection_get_selected_rows(selection, &model);
    GList *i;
    gint frequency;

    if(!list)
        return;

    scanlist_dialog();

    for(i=list; i; i=i->next)
    {
        gtk_tree_model_get_iter(model, &iter, (GtkTreePath*)i->data);
        gtk_tree_model_get(model, &iter, COL_FREQUENCY, &frequency, -1);
        if(frequency)
            scanlist_add(frequency/1000);
    }

    g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
    g_list_free(list);
}

static void
ui_view_menu_blacklist(GtkWidget *menuitem,
                       gpointer   user_data)
{
    GtkTreeView *treeview = GTK_TREE_VIEW(user_data);
    GtkTreeSelection *selection = gtk_tree_view_get_selection(treeview);
    GtkTreeModel *model;
    GtkTreeIter iter;
    GList *list = gtk_tree_selection_get_selected_rows(selection, &model);
    GList *i;
    gchar *address;

    if(!list)
        return;

    if(g_list_length(list) == 1)
    {
        gtk_tree_model_get_iter(model, &iter, (GtkTreePath*)list->data);
        gtk_tree_model_get(model, &iter, COL_ADDRESS, &address, -1);

        if(!conf_get_preferences_blacklist(address))
            conf_set_preferences_blacklist(address);
        else
            conf_del_preferences_blacklist(address);
    }
    else
    {
        for(i = list; i; i = i->next)
        {
            gtk_tree_model_get_iter(model, &iter, (GtkTreePath*)i->data);
            gtk_tree_model_get(model, &iter, COL_ADDRESS, &address, -1);
            if (address)
                conf_set_preferences_blacklist(address);
        }
    }

    g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
    g_list_free(list);
}

static void
ui_view_menu_save_as(GtkWidget *menuitem,
                     gpointer   user_data)
{
    GtkTreeView *treeview = GTK_TREE_VIEW(user_data);
    GtkTreeSelection *selection = gtk_tree_view_get_selection(treeview);
    GtkTreeModel *model;
    GtkTreeIter iter;
    GList *list = gtk_tree_selection_get_selected_rows(selection, &model);
    GList *i, *iterlist = NULL;
    ui_dialog_save_t *s;

    if(!list)
        return;

    if((s = ui_dialog_save(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(treeview))))))
    {
        for(i=list; i; i=i->next)
        {
            gtk_tree_model_get_iter(model, &iter, (GtkTreePath*)i->data);
            iterlist = g_list_append(iterlist, (gpointer)gtk_tree_iter_copy(&iter));
        }
        log_save(s->filename, s->strip_signals, s->strip_gps, s->strip_azi, iterlist);
        g_free(s->filename);
        g_free(s);
    }

    g_list_foreach(iterlist, (GFunc)gtk_tree_iter_free, NULL);
    g_list_free(iterlist);
    g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
    g_list_free(list);
}

static void
ui_view_menu_remove(GtkWidget *menuitem,
                    gpointer   user_data)
{
    ui_view_remove_selection(GTK_WIDGET(user_data));
}
