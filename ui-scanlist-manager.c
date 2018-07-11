/*
 *  MTscan - MikroTik RouterOS wireless scanner
 *  Copyright (c) 2015-2018  Konrad Kosmatka
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

#include <gtk/gtk.h>
#include "ui-scanlist-manager.h"
#include "conf-scanlist.h"
#include "ui-dialogs.h"

#define UI_SCANLIST_MANAGER_DEFAULT_WIDTH (-1)
#define UI_SCANLIST_MANAGER_DEFAULT_HEIGHT 300

typedef struct scanlist_manager
{
    GtkWidget *window;
    GtkWidget *content;
    GtkWidget *view;
    GtkWidget *scrolled;
    GtkWidget *box_button;
    GtkWidget *b_clear;
    GtkWidget *b_remove;
    GtkWidget *b_add;
    GtkWidget *b_close;
} scanlist_manager_t;

static void ui_scanlist_manager_destroy(GtkWidget*, gpointer);

static void ui_scanlist_manager_edited(GtkCellRendererText*, gchar*, gchar*, gpointer);
static void ui_scanlist_manager_toggled(GtkCellRendererToggle*, gchar*, gpointer);
gboolean ui_scanlist_manager_toggled_foreach(GtkTreeModel*, GtkTreePath*, GtkTreeIter*, gpointer);

static void ui_scanlist_manager_clear(GtkWidget*, gpointer);
static void ui_scanlist_manager_remove(GtkWidget*, gpointer);
static void ui_scanlist_manager_add(GtkWidget*, gpointer);


void
ui_scanlist_manager(GtkWidget    *parent,
                    GtkListStore *model)
{
    scanlist_manager_t *m;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    m = g_malloc(sizeof(scanlist_manager_t));
    m->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_modal(GTK_WINDOW(m->window), FALSE);
    gtk_window_set_title(GTK_WINDOW(m->window), "Scan-list manager");
    gtk_window_set_destroy_with_parent(GTK_WINDOW(m->window), TRUE);
    gtk_container_set_border_width(GTK_CONTAINER(m->window), 2);
    gtk_window_set_transient_for(GTK_WINDOW(m->window), GTK_WINDOW(parent));
    gtk_window_set_position(GTK_WINDOW(m->window), GTK_WIN_POS_CENTER_ON_PARENT);

    m->content = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(m->window), m->content);

    m->view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(model));
    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(m->view), TRUE);
    gtk_tree_view_set_reorderable(GTK_TREE_VIEW(m->view), TRUE);

    renderer = gtk_cell_renderer_text_new();
    g_object_set(renderer, "editable", TRUE, NULL);
    g_object_set_data(G_OBJECT(renderer), "mtscan_scanlist_column", GUINT_TO_POINTER(CONF_SCANLIST_COL_NAME));
    g_signal_connect(renderer, "edited", G_CALLBACK(ui_scanlist_manager_edited), m);
    column = gtk_tree_view_column_new_with_attributes("Name", renderer, "text", CONF_SCANLIST_COL_NAME, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(m->view), column);

    renderer = gtk_cell_renderer_text_new();
    g_object_set(renderer, "ellipsize", PANGO_ELLIPSIZE_END, "ellipsize-set", TRUE, NULL);
    g_object_set(renderer, "editable", TRUE, NULL);
    g_object_set_data(G_OBJECT(renderer), "mtscan_scanlist_column", GUINT_TO_POINTER(CONF_SCANLIST_COL_DATA));
    g_signal_connect(renderer, "edited", G_CALLBACK(ui_scanlist_manager_edited), m);
    column = gtk_tree_view_column_new_with_attributes("Value", renderer, "text", CONF_SCANLIST_COL_DATA, NULL);
    gtk_tree_view_column_set_expand(column, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(m->view), column);

    renderer = gtk_cell_renderer_toggle_new();
    gtk_cell_renderer_toggle_set_radio(GTK_CELL_RENDERER_TOGGLE(renderer), TRUE);
    g_signal_connect(renderer, "toggled", G_CALLBACK(ui_scanlist_manager_toggled), m);
    column = gtk_tree_view_column_new_with_attributes("*", renderer, "active", CONF_SCANLIST_COL_MAIN, NULL);
    gtk_tree_view_column_set_alignment(column, 0.5);
    gtk_tree_view_append_column(GTK_TREE_VIEW(m->view), column);

    m->scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(m->scrolled), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(m->scrolled), m->view);
    gtk_widget_set_size_request(m->scrolled, UI_SCANLIST_MANAGER_DEFAULT_WIDTH, UI_SCANLIST_MANAGER_DEFAULT_HEIGHT);
    gtk_container_add(GTK_CONTAINER(m->content), m->scrolled);

    m->box_button = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(m->box_button), GTK_BUTTONBOX_END);
    gtk_box_set_spacing(GTK_BOX(m->box_button), 5);
    gtk_box_pack_start(GTK_BOX(m->content), m->box_button, FALSE, FALSE, 5);

    m->b_clear = gtk_button_new_from_stock(GTK_STOCK_CLEAR);
    g_signal_connect(m->b_clear, "clicked", G_CALLBACK(ui_scanlist_manager_clear), m);
    gtk_container_add(GTK_CONTAINER(m->box_button), m->b_clear);

    m->b_remove = gtk_button_new_from_stock(GTK_STOCK_REMOVE);
    g_signal_connect(m->b_remove, "clicked", G_CALLBACK(ui_scanlist_manager_remove), m);
    gtk_container_add(GTK_CONTAINER(m->box_button), m->b_remove);

    m->b_add = gtk_button_new_from_stock(GTK_STOCK_ADD);
    g_signal_connect(m->b_add, "clicked", G_CALLBACK(ui_scanlist_manager_add), m);
    gtk_container_add(GTK_CONTAINER(m->box_button), m->b_add);

    m->b_close = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
    g_signal_connect_swapped(m->b_close, "clicked", G_CALLBACK(gtk_widget_destroy), m->window);
    gtk_container_add(GTK_CONTAINER(m->box_button), m->b_close);

    g_signal_connect(m->window, "destroy", G_CALLBACK(ui_scanlist_manager_destroy), m);
    gtk_widget_show_all(m->window);
}

static void
ui_scanlist_manager_destroy(GtkWidget *widget,
                            gpointer   user_data)
{
    scanlist_manager_t *m = (scanlist_manager_t*)user_data;
    g_free(m);
}


static void
ui_scanlist_manager_edited(GtkCellRendererText *renderer,
                           gchar               *path_string,
                           gchar               *new_text,
                           gpointer             user_data)
{
    scanlist_manager_t *m = (scanlist_manager_t*)user_data;
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(m->view));
    GtkTreeIter iter;
    guint column;

    column = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(renderer), "mtscan_scanlist_column"));
    if(gtk_tree_model_get_iter_from_string(model, &iter, path_string))
        gtk_list_store_set(GTK_LIST_STORE(model), &iter, column, new_text, -1);
}

static void
ui_scanlist_manager_toggled(GtkCellRendererToggle *renderer,
                            gchar                 *path_string,
                            gpointer               user_data)
{
    scanlist_manager_t *m = (scanlist_manager_t *) user_data;
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(m->view));
    GtkTreeIter iter;

    if(gtk_tree_model_get_iter_from_string(model, &iter, path_string))
    {
        gtk_tree_model_foreach(model, ui_scanlist_manager_toggled_foreach, NULL);
        gtk_list_store_set(GTK_LIST_STORE(model), &iter, CONF_SCANLIST_COL_MAIN, TRUE, -1);
    }
}

gboolean
ui_scanlist_manager_toggled_foreach(GtkTreeModel *model,
                                    GtkTreePath  *path,
                                    GtkTreeIter  *iter,
                                    gpointer      user_data)
{
    gtk_list_store_set(GTK_LIST_STORE(model), iter, CONF_SCANLIST_COL_MAIN, FALSE, -1);
    return FALSE;
}

static void
ui_scanlist_manager_clear(GtkWidget *widget,
                          gpointer   user_data)
{
    scanlist_manager_t *m = (scanlist_manager_t*)user_data;
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(m->view));
    const gchar text[] = "<big>Remove all scan-lists</big>\n\n"
                         "Are you sure you want to remove <b>all</b> saved scan-lists?";

    if(ui_dialog_yesno(GTK_WINDOW(m->window), text) == UI_DIALOG_YES)
        gtk_list_store_clear(GTK_LIST_STORE(model));
}

static void
ui_scanlist_manager_remove(GtkWidget *widget,
                           gpointer   user_data)
{
    scanlist_manager_t *m = (scanlist_manager_t*)user_data;
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(m->view));
    GtkTreeModel *model;
    GtkTreeIter iter;
    conf_scanlist_t *sl;
    const gchar *value;
    gchar *scanlist = NULL;
    gchar *text;

    if(!gtk_tree_selection_get_selected(selection, &model, &iter))
        return;

    sl = conf_scanlist_list_get(GTK_LIST_STORE(model), &iter);
    value = conf_scanlist_get_data(sl);

    if(strlen(value) > 50)
        scanlist = g_utf8_substring(value, 0, 50);

    text = g_strdup_printf("<big>Remove scan-list</big>\n\n"
                           "Are you sure you want to remove the following scan-list?\n\n"
                           "Name: <b>%s</b>\n"
                           "<b>%s%s</b>",
                           conf_scanlist_get_name(sl),
                           (scanlist ? scanlist : value),
                           (scanlist ? "..." : ""));

    if(ui_dialog_yesno(GTK_WINDOW(m->window), text) == UI_DIALOG_YES)
        gtk_list_store_remove(GTK_LIST_STORE(model), &iter);

    conf_scanlist_free(sl);
    g_free(text);
}


static void
ui_scanlist_manager_add(GtkWidget *widget,
                        gpointer   user_data)
{
    scanlist_manager_t *m = (scanlist_manager_t*)user_data;
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(m->view));
    ui_dialog_scanlist_t *ret;
    conf_scanlist_t *sl;

    ret = ui_dialog_scanlist(GTK_WINDOW(m->window), TRUE);
    if(ret)
    {
        sl = conf_scanlist_new(g_strdup(ret->name), g_strdup(ret->value), FALSE);
        conf_scanlist_list_add(GTK_LIST_STORE(model), sl);
        conf_scanlist_free(sl);
        ui_dialog_scanlist_free(ret);
    }
}
