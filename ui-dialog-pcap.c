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
#include <pcap/pcap.h>
#include "ui-dialogs.h"

#define UI_DIALOG_PCAP_DEFAULT_WIDTH  500
#define UI_DIALOG_PCAP_DEFAULT_HEIGHT 300

typedef struct ui_dialog_pcap
{
    GtkWidget *window;
    GtkWidget *content;
    GtkWidget *view;
    GtkWidget *scrolled;
    GtkWidget *box_button;
    GtkWidget *b_ok;
    GtkWidget *b_close;
    void (*callback)(const gchar*, gpointer);
    gpointer user_data;
} ui_dialog_pcap_t;

enum
{
    UI_DIALOG_PCAP_COL_NAME,
    UI_DIALOG_PCAP_COL_DESC,
    UI_DIALOG_PCAP_COLS
};

static void ui_dialog_pcap_destroy(GtkWidget*, gpointer);
static void ui_dialog_pcap_ok(GtkWidget*, gpointer);
static void ui_dialog_pcap_row_activated(GtkTreeView*, GtkTreePath*, GtkTreeViewColumn*, gpointer);
static GtkListStore* ui_dialog_pcap_model_new();


void
ui_dialog_pcap_new(GtkWindow  *parent,
                   void      (*callback)(const gchar*, gpointer),
                   gpointer    user_data)
{
    ui_dialog_pcap_t *context;
    GtkListStore *model;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    model = ui_dialog_pcap_model_new();
    if(!model)
    {
        ui_dialog(parent, GTK_MESSAGE_ERROR, "Pcap interface", "Failed to enumerate pcap devices.");
        return;
    }

    context = g_malloc(sizeof(ui_dialog_pcap_t));

    context->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_modal(GTK_WINDOW(context->window), TRUE);
    gtk_window_set_title(GTK_WINDOW(context->window), "Pcap interface");
    gtk_window_set_destroy_with_parent(GTK_WINDOW(context->window), TRUE);
    gtk_container_set_border_width(GTK_CONTAINER(context->window), 2);
    gtk_window_set_transient_for(GTK_WINDOW(context->window), GTK_WINDOW(parent));
    gtk_window_set_position(GTK_WINDOW(context->window), GTK_WIN_POS_CENTER_ON_PARENT);

    context->content = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(context->window), context->content);

    context->view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(model));
    g_object_unref(model);

    gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(context->view)), GTK_SELECTION_BROWSE);
    g_signal_connect(context->view, "row-activated", (GCallback)ui_dialog_pcap_row_activated, context);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Name", renderer, "text", UI_DIALOG_PCAP_COL_NAME, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(context->view), column);

    renderer = gtk_cell_renderer_text_new();
    g_object_set(renderer, "ellipsize", PANGO_ELLIPSIZE_END, "ellipsize-set", TRUE, NULL);
    column = gtk_tree_view_column_new_with_attributes("Description", renderer, "text", UI_DIALOG_PCAP_COL_DESC, NULL);
    gtk_tree_view_column_set_expand(column, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(context->view), column);

    context->scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(context->scrolled), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(context->scrolled), context->view);
    gtk_widget_set_size_request(context->scrolled, UI_DIALOG_PCAP_DEFAULT_WIDTH, UI_DIALOG_PCAP_DEFAULT_HEIGHT);
    gtk_container_add(GTK_CONTAINER(context->content), context->scrolled);

    context->box_button = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(context->box_button), GTK_BUTTONBOX_END);
    gtk_box_set_spacing(GTK_BOX(context->box_button), 5);
    gtk_box_pack_start(GTK_BOX(context->content), context->box_button, FALSE, FALSE, 5);

    context->b_ok = gtk_button_new_from_stock(GTK_STOCK_OK);
    g_signal_connect(context->b_ok, "clicked", G_CALLBACK(ui_dialog_pcap_ok), context);
    gtk_container_add(GTK_CONTAINER(context->box_button), context->b_ok);

    context->b_close = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
    g_signal_connect_swapped(context->b_close, "clicked", G_CALLBACK(gtk_widget_destroy), context->window);
    gtk_container_add(GTK_CONTAINER(context->box_button), context->b_close);

    g_signal_connect(context->window, "destroy", G_CALLBACK(ui_dialog_pcap_destroy), context);
    gtk_widget_show_all(context->window);

    context->callback = callback;
    context->user_data = user_data;
}

static void
ui_dialog_pcap_destroy(GtkWidget *widget,
                       gpointer   user_data)
{
    ui_dialog_pcap_t *context = (ui_dialog_pcap_t*)user_data;
    g_free(context);
}

static void
ui_dialog_pcap_ok(GtkWidget *widget,
                  gpointer   user_data)
{
    ui_dialog_pcap_t *context = (ui_dialog_pcap_t *) user_data;
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    gchar *name;

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(context->view));
    if(gtk_tree_selection_get_selected(selection, &model, &iter))
    {
        gtk_tree_model_get(model, &iter, UI_DIALOG_PCAP_COL_NAME, &name, -1);
        context->callback(name, context->user_data);
        g_free(name);
        gtk_widget_destroy(context->window);
    }
}

static void
ui_dialog_pcap_row_activated(GtkTreeView       *treeview,
                             GtkTreePath       *path,
                             GtkTreeViewColumn *col,
                             gpointer          user_data)
{
    ui_dialog_pcap_t *context = (ui_dialog_pcap_t *)user_data;
    gtk_button_clicked(GTK_BUTTON(context->b_ok));
}

static GtkListStore*
ui_dialog_pcap_model_new()
{
    GtkListStore *model;
    GtkTreeIter iter;
    pcap_if_t *devlist;
    pcap_if_t *dev;
    gchar errbuf[PCAP_ERRBUF_SIZE];

    if(pcap_findalldevs(&devlist, errbuf) != 0)
        return NULL;

    model = gtk_list_store_new(UI_DIALOG_PCAP_COLS,
                               G_TYPE_STRING,  /* UI_DIALOG_PCAP_COL_NAME */
                               G_TYPE_STRING); /* UI_DIALOG_PCAP_COL_DESC */

    for(dev = devlist; dev; dev=dev->next)
    {
        gtk_list_store_append(model, &iter);
        gtk_list_store_set(model, &iter,
                           UI_DIALOG_PCAP_COL_NAME, dev->name,
                           UI_DIALOG_PCAP_COL_DESC, (dev->description ? dev->description : ""),
                          -1);
    }

    pcap_freealldevs(devlist);
    return model;
}
