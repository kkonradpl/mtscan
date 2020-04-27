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

#include <string.h>
#include <gdk/gdkkeysyms.h>
#include "ui.h"
#include "mt-ssh.h"
#include "ui-scanlist.h"
#include "misc.h"
#include "conf-scanlist.h"
#include "conf.h"
#include "ui-dialogs.h"

#define FREQ_2GHZ_START 2192
#define FREQ_2GHZ_END   2732
#define FREQ_2GHZ_INC_A    2
#define FREQ_2GHZ_INC_B    3

#define FREQ_5GHZ_START 4800
#define FREQ_5GHZ_END   6100
#define FREQ_5GHZ_INC      5

#define FREQS_PER_LINE 20

struct ui_scanlist
{
    GtkWidget *parent;
    GtkWidget *window;
    GtkWidget *content;
    GtkWidget *notebook;
    GtkWidget *page2;
    GtkWidget *freqs2;
    GtkWidget *page5;
    GtkWidget *freqs5;
    GtkWidget *box_custom;
    GtkWidget *l_custom;
    GtkWidget *e_custom;
    GtkWidget *l_current;
    GtkWidget *box_button;
    GtkWidget *b_clear;
    GtkWidget *b_revert;
    GtkWidget *b_save;
    GtkWidget *b_preset_a;
    GtkWidget *b_preset_b;
    GtkWidget *b_preset_c;
    GtkWidget *b_apply;
    GtkWidget *b_close;

    guint default_page;
    gchar *current;
    GTree *freq_tree;
} typedef ui_scanlist_t;

static const gint freq_2GHz_usa[] =
{
    2412, 2417, 2422, 2427, 2432, 2437, 2442, 2447, 2452, 2457, 2462, 0
};

static const gint freq_2GHz_eu[] =
{
    2412, 2417, 2422, 2427, 2432, 2437, 2442, 2447, 2452, 2457, 2462, 2467, 2472, 0
};

static const gint freq_2GHz_jp[] =
{
    2412, 2417, 2422, 2427, 2432, 2437, 2442, 2447, 2452, 2457, 2462, 2467, 2472, 2484, 0
};

static const gint freq_5GHz_5180_5320[] =
{
    5180, 5200, 5220, 5240, 5260, 5280, 5300, 5320, 0
};

static const gint freq_5GHz_5500_5700[] =
{
    5500, 5520, 5540, 5560, 5580, 5600, 5620, 5640, 5660, 5680, 5700, 0
};

static const gint freq_5GHz_5745_5825[] =
{
    5745, 5765, 5785, 5805, 5825, 0
};

static void ui_scanlist_destroy(GtkWidget*, gpointer);
static gboolean ui_scanlist_key(GtkWidget*, GdkEventKey*, gpointer);
static gboolean ui_scanlist_click(GtkWidget*, GdkEventButton*, gpointer);

static void ui_scanlist_switch_page(GtkWidget*, gpointer, guint, gpointer);
static void ui_scanlist_update_current(ui_scanlist_t*);

static void ui_scanlist_clear(GtkWidget*, gpointer);
static void ui_scanlist_revert(GtkWidget*, gpointer);
static void ui_scanlist_save(GtkWidget*, gpointer);
static void ui_scanlist_preset(GtkWidget*, gpointer);
static void ui_scanlist_apply(GtkWidget*, gpointer);

static gchar* ui_scanlist_string(GTree*, const gchar*);
static gboolean ui_scanlist_string_foreach(gpointer, gpointer, gpointer);

static gboolean ui_scanlist_set_foreach(gpointer, gpointer, gpointer);
static gboolean freq_list_contains(const gint*, gint);


ui_scanlist_t*
ui_scanlist(GtkWidget          *parent,
            ui_scanlist_page_t  page)
{
    ui_scanlist_t *s;
    GtkWidget *box_freq;
    GtkWidget *b_freq;
    GtkWidget *l_freq;
    gchar buff[100];
    guint freq;
    gint inc;
    gint i;

    s = g_malloc(sizeof(ui_scanlist_t));
    s->parent = parent;
    s->current = NULL;
    s->default_page = page;
    s->freq_tree = g_tree_new((GCompareFunc)gptrcmp);

    s->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_modal(GTK_WINDOW(s->window), FALSE);
    gtk_window_set_resizable(GTK_WINDOW(s->window), FALSE);
    gtk_window_set_title(GTK_WINDOW(s->window), "Scan-list");
    gtk_window_set_destroy_with_parent(GTK_WINDOW(s->window), TRUE);
    gtk_container_set_border_width(GTK_CONTAINER(s->window), 2);

    s->content = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(s->window), s->content);

    s->notebook = gtk_notebook_new();
    gtk_notebook_popup_enable(GTK_NOTEBOOK(s->notebook));
    gtk_container_add(GTK_CONTAINER(s->content), s->notebook);

    s->page2 = gtk_vbox_new(TRUE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(s->page2), 4);
    gtk_notebook_append_page(GTK_NOTEBOOK(s->notebook), s->page2, gtk_label_new("2 GHz"));
    gtk_container_child_set(GTK_CONTAINER(s->notebook), s->page2, "tab-expand", FALSE, "tab-fill", FALSE, NULL);
    s->freqs2 = gtk_hbox_new(TRUE, 0);
    gtk_container_add(GTK_CONTAINER(s->page2), s->freqs2);

    for(i = 0, inc = FREQ_2GHZ_INC_B, freq=FREQ_2GHZ_START; freq<=FREQ_2GHZ_END; freq+=inc, i++)
    {
        if(i % FREQS_PER_LINE == 0)
        {
            box_freq = gtk_vbox_new(FALSE, 0);
            gtk_box_pack_start(GTK_BOX(s->freqs2), box_freq, FALSE, FALSE, 0);
        }

        if(freq_list_contains(freq_2GHz_usa, freq))
            snprintf(buff, sizeof(buff), "<span color=\"#ff0000\"><b>%d</b></span>", freq);
        else if(freq_list_contains(freq_2GHz_eu, freq))
            snprintf(buff, sizeof(buff), "<span color=\"#ff00ff\"><b>%d</b></span>", freq);
        else if(freq_list_contains(freq_2GHz_jp, freq))
            snprintf(buff, sizeof(buff), "<span color=\"#0000ff\"><b>%d</b></span>", freq);
        else
            snprintf(buff, sizeof(buff), "%d", freq);

        b_freq = gtk_check_button_new();
        l_freq = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(l_freq), buff);
        gtk_container_add(GTK_CONTAINER(b_freq), l_freq);
        g_object_set_data(G_OBJECT(b_freq), "mtscan_scanlist_freq", GUINT_TO_POINTER(freq));
        g_signal_connect(b_freq, "button-press-event", G_CALLBACK(ui_scanlist_click), s);
        gtk_box_pack_start(GTK_BOX(box_freq), b_freq, FALSE, FALSE, 1);
        g_tree_insert(s->freq_tree, GINT_TO_POINTER(freq), b_freq);
        inc = (inc == FREQ_2GHZ_INC_A ? FREQ_2GHZ_INC_B : FREQ_2GHZ_INC_A);
    }

    s->page5 = gtk_vbox_new(TRUE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(s->page5), 4);
    gtk_notebook_append_page(GTK_NOTEBOOK(s->notebook), s->page5, gtk_label_new("5 GHz"));
    gtk_container_child_set(GTK_CONTAINER(s->notebook), s->page5, "tab-expand", FALSE, "tab-fill", FALSE, NULL);
    s->freqs5 = gtk_hbox_new(TRUE, 0);
    gtk_container_add(GTK_CONTAINER(s->page5), s->freqs5);

    for(i = 0, inc = FREQ_5GHZ_INC, freq=FREQ_5GHZ_START; freq<=FREQ_5GHZ_END; freq+=inc, i++)
    {
        if(i % FREQS_PER_LINE == 0)
        {
            box_freq = gtk_vbox_new(FALSE, 0);
            gtk_box_pack_start(GTK_BOX(s->freqs5), box_freq, FALSE, FALSE, 0);
        }

        if(freq_list_contains(freq_5GHz_5180_5320, freq))
            snprintf(buff, sizeof(buff), "<span color=\"#ff0000\"><b>%d</b></span>", freq);
        else if(freq_list_contains(freq_5GHz_5500_5700, freq))
            snprintf(buff, sizeof(buff), "<span color=\"#ff00ff\"><b>%d</b></span>", freq);
        else if(freq_list_contains(freq_5GHz_5745_5825, freq))
            snprintf(buff, sizeof(buff), "<span color=\"#0000ff\"><b>%d</b></span>", freq);
        else
            snprintf(buff, sizeof(buff), "%d", freq);

        b_freq = gtk_check_button_new();
        l_freq = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(l_freq), buff);
        gtk_container_add(GTK_CONTAINER(b_freq), l_freq);
        g_object_set_data(G_OBJECT(b_freq), "mtscan_scanlist_freq", GUINT_TO_POINTER(freq));
        g_signal_connect(b_freq, "button-press-event", G_CALLBACK(ui_scanlist_click), s);
        gtk_box_pack_start(GTK_BOX(box_freq), b_freq, FALSE, FALSE, 1);
        g_tree_insert(s->freq_tree, GINT_TO_POINTER(freq), b_freq);
    }

    s->box_custom = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(s->content), s->box_custom, FALSE, FALSE, 1);

    s->l_custom = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(s->l_custom), "<b>Custom channels:</b>");
    gtk_box_pack_start(GTK_BOX(s->box_custom), s->l_custom, FALSE, FALSE, 0);

    s->e_custom = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(s->box_custom), s->e_custom, TRUE, TRUE, 0);

    s->l_current = gtk_label_new(NULL);
    gtk_label_set_ellipsize(GTK_LABEL(s->l_current), PANGO_ELLIPSIZE_END);
    gtk_misc_set_alignment(GTK_MISC(s->l_current), 0.0, 0.5);
    gtk_box_pack_start(GTK_BOX(s->content), s->l_current, FALSE, TRUE, 1);

    s->box_button = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(s->box_button), GTK_BUTTONBOX_END);
    gtk_box_set_spacing(GTK_BOX(s->box_button), 5);
    gtk_box_pack_start(GTK_BOX(s->content), s->box_button, FALSE, FALSE, 5);

    s->b_clear = gtk_button_new_from_stock(GTK_STOCK_CLEAR);
    g_signal_connect(s->b_clear, "clicked", G_CALLBACK(ui_scanlist_clear), s);
    gtk_container_add(GTK_CONTAINER(s->box_button), s->b_clear);

    s->b_revert = gtk_button_new_from_stock(GTK_STOCK_REVERT_TO_SAVED);
    g_signal_connect(s->b_revert, "clicked", G_CALLBACK(ui_scanlist_revert), s);
    gtk_container_add(GTK_CONTAINER(s->box_button), s->b_revert);

    s->b_save = gtk_button_new_from_stock(GTK_STOCK_SAVE);
    g_signal_connect(s->b_save, "clicked", G_CALLBACK(ui_scanlist_save), s);
    gtk_container_add(GTK_CONTAINER(s->box_button), s->b_save);

    s->b_preset_a = gtk_button_new_with_label(NULL);
    g_signal_connect(s->b_preset_a, "clicked", G_CALLBACK(ui_scanlist_preset), s);
    gtk_container_add(GTK_CONTAINER(s->box_button), s->b_preset_a);

    s->b_preset_b = gtk_button_new_with_label(NULL);
    g_signal_connect(s->b_preset_b, "clicked", G_CALLBACK(ui_scanlist_preset), s);
    gtk_container_add(GTK_CONTAINER(s->box_button), s->b_preset_b);

    s->b_preset_c = gtk_button_new_with_label(NULL);
    g_signal_connect(s->b_preset_c, "clicked", G_CALLBACK(ui_scanlist_preset), s);
    gtk_container_add(GTK_CONTAINER(s->box_button), s->b_preset_c);

    s->b_apply = gtk_button_new_from_stock(GTK_STOCK_APPLY);
    g_signal_connect(s->b_apply, "clicked", G_CALLBACK(ui_scanlist_apply), s);
    gtk_container_add(GTK_CONTAINER(s->box_button), s->b_apply);

    s->b_close = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
    g_signal_connect_swapped(s->b_close, "clicked", G_CALLBACK(gtk_widget_hide), s->window);
    gtk_container_add(GTK_CONTAINER(s->box_button), s->b_close);

    g_signal_connect(s->notebook, "switch-page", G_CALLBACK(ui_scanlist_switch_page), s);
    g_signal_connect(s->window, "key-press-event", G_CALLBACK(ui_scanlist_key), s);
    g_signal_connect(s->window, "delete-event", G_CALLBACK(gtk_widget_hide), NULL);
    g_signal_connect(s->window, "destroy", G_CALLBACK(ui_scanlist_destroy), s);

    ui_scanlist_update_current(s);

    return s;
}

void
ui_scanlist_free(ui_scanlist_t *s)
{
    gtk_widget_destroy(s->window);
}

void
ui_scanlist_show(ui_scanlist_t *s)
{
    if(gtk_widget_get_visible(s->window))
    {
        gtk_window_present(GTK_WINDOW(s->window));
    }
    else
    {
        gtk_window_set_transient_for(GTK_WINDOW(s->window), GTK_WINDOW(s->parent));
        gtk_window_set_position(GTK_WINDOW(s->window), GTK_WIN_POS_CENTER_ON_PARENT);

        gtk_widget_show_all(s->window);
        gtk_notebook_set_current_page(GTK_NOTEBOOK(s->notebook), s->default_page);

        /* Scan-list window is now centered over parent, don't stay on top anymore */
        gtk_window_set_transient_for(GTK_WINDOW(s->window), NULL);
    }
}

void
ui_scanlist_current(ui_scanlist_t *s,
                    const gchar   *str)
{
    g_free(s->current);
    s->current = g_strdup(str);
    ui_scanlist_update_current(s);
}

void
ui_scanlist_add(ui_scanlist_t *s,
                const gchar   *channel)
{
    const gchar *custom;
    gpointer button;
    guint frequency;
    gchar *endptr;

    frequency = (gint)g_ascii_strtoull(channel, &endptr, 10);

    if(*endptr == '\0' &&
       frequency &&
       (button = g_tree_lookup(s->freq_tree, GINT_TO_POINTER(frequency))))
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
    }
    else
    {
        custom = gtk_entry_get_text(GTK_ENTRY(s->e_custom));
        if(strlen(custom))
            gtk_entry_append_text(GTK_ENTRY(s->e_custom), ",");
        gtk_entry_append_text(GTK_ENTRY(s->e_custom), channel);
    }
}

static void
ui_scanlist_destroy(GtkWidget *widget,
                    gpointer   user_data)
{
    ui_scanlist_t *s = (ui_scanlist_t*)user_data;
    g_free(s->current);
    g_tree_destroy(s->freq_tree);
    g_free(s);
}

static gboolean
ui_scanlist_key(GtkWidget   *widget,
                GdkEventKey *event,
                gpointer     user_data)
{
    ui_scanlist_t *s = (ui_scanlist_t*)user_data;
    guint current = gdk_keyval_to_upper(event->keyval);
    if(current == GDK_KEY_Escape)
    {
        gtk_button_clicked(GTK_BUTTON(s->b_close));
        return TRUE;
    }
    return FALSE;
}

static gboolean
ui_scanlist_click(GtkWidget      *widget,
                  GdkEventButton *event,
                  gpointer        user_data)
{
    gchar *string;
    guint freq;

    freq = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(widget), "mtscan_scanlist_freq"));

    if(freq &&
       ui.conn &&
       event->type == GDK_BUTTON_PRESS &&
       event->button == 3) // right mouse button
    {
        /* Quick frequency set */
        string = g_strdup_printf("%d", freq);
        mt_ssh_cmd(ui.conn, MT_SSH_CMD_SCANLIST, string);
        g_free(string);
        return TRUE;
    }
    return FALSE;
}

static void
ui_scanlist_switch_page(GtkWidget *widget,
                        gpointer   page,
                        guint      page_id,
                        gpointer   user_data)
{
    ui_scanlist_t *s = (ui_scanlist_t*)user_data;

    if(page_id == UI_SCANLIST_PAGE_5_GHZ)
    {
        gtk_button_set_label(GTK_BUTTON(s->b_preset_a), "5180-5320");
        gtk_button_set_label(GTK_BUTTON(s->b_preset_b), "5500-5700");
        gtk_button_set_label(GTK_BUTTON(s->b_preset_c), "5745-5825");
        g_object_set_data(G_OBJECT(s->b_preset_a), "mtscan_scanlist_preset", (gpointer) freq_5GHz_5180_5320);
        g_object_set_data(G_OBJECT(s->b_preset_b), "mtscan_scanlist_preset", (gpointer) freq_5GHz_5500_5700);
        g_object_set_data(G_OBJECT(s->b_preset_c), "mtscan_scanlist_preset", (gpointer) freq_5GHz_5745_5825);
    }
    else if(page_id == UI_SCANLIST_PAGE_2_GHZ)
    {
        gtk_button_set_label(GTK_BUTTON(s->b_preset_a), "2412-2462");
        gtk_button_set_label(GTK_BUTTON(s->b_preset_b), "2412-2472");
        gtk_button_set_label(GTK_BUTTON(s->b_preset_c), "2412-2484");
        g_object_set_data(G_OBJECT(s->b_preset_a), "mtscan_scanlist_preset", (gpointer)freq_2GHz_usa);
        g_object_set_data(G_OBJECT(s->b_preset_b), "mtscan_scanlist_preset", (gpointer)freq_2GHz_eu);
        g_object_set_data(G_OBJECT(s->b_preset_c), "mtscan_scanlist_preset", (gpointer)freq_2GHz_jp);
    }
}

static void
ui_scanlist_update_current(ui_scanlist_t *s)
{
    gchar *compressed = str_scanlist_compress(s->current);
    gchar *str = g_markup_printf_escaped("<b>Current:</b> %s", (compressed ? compressed : "unknown"));

    gtk_label_set_markup(GTK_LABEL(s->l_current), str);
    g_free(compressed);
    g_free(str);
}

static void
ui_scanlist_clear(GtkWidget *widget,
                  gpointer   user_data)
{
    ui_scanlist_t *s = (ui_scanlist_t*)user_data;
    g_tree_foreach(s->freq_tree, ui_scanlist_set_foreach, GINT_TO_POINTER(FALSE));
    gtk_entry_set_text(GTK_ENTRY(s->e_custom), "");
}

static void
ui_scanlist_revert(GtkWidget *widget,
                   gpointer   user_data)
{
    ui_scanlist_t *s = (ui_scanlist_t*)user_data;
    gchar **channels;
    gchar **ch;

    if(!s->current)
    {
        ui_dialog(GTK_WINDOW(s->window),
                  GTK_MESSAGE_WARNING,
                  "Scan-list",
                  "<big>Unable to revert scan-list</big>\n\nCurrent scan-list is unknown.");
        return;
    }

    gtk_button_clicked(GTK_BUTTON(s->b_clear));

    channels = g_strsplit(s->current, ",", -1);
    for(ch = channels; *ch; ch++)
    {
        if(strlen(*ch))
            ui_scanlist_add(s, *ch);
    }
    g_strfreev(channels);
}

static void
ui_scanlist_save(GtkWidget *widget,
                 gpointer   user_data)
{
    ui_scanlist_t *s = (ui_scanlist_t*)user_data;
    const gchar *custom;
    gchar *output;
    conf_scanlist_t *sl;
    ui_dialog_scanlist_t *ret;

    custom = gtk_entry_get_text(GTK_ENTRY(s->e_custom));
    output = ui_scanlist_string(s->freq_tree, custom);
    if(!output)
    {
        ui_dialog(GTK_WINDOW(s->window),
                  GTK_MESSAGE_WARNING,
                  "Scan-list",
                  "<big>Unable to save scan-list</big>\n\nNo frequencies selected.");
        return;
    }

    ret = ui_dialog_scanlist(GTK_WINDOW(s->window), FALSE);
    if(ret)
    {
        sl = conf_scanlist_new(g_strdup(ret->name), g_strdup(output), FALSE, FALSE);
        conf_scanlist_list_add(conf_get_scanlists(), sl);
        conf_scanlist_free(sl);
        ui_dialog_scanlist_free(ret);
    }

    g_free(output);

}

static void
ui_scanlist_preset(GtkWidget *widget,
                   gpointer   user_data)
{
    ui_scanlist_t *s = (ui_scanlist_t*)user_data;
    gint *preset = (gint*)(g_object_get_data(G_OBJECT(widget), "mtscan_scanlist_preset"));
    gpointer ptr;

    if(preset)
    {
        while(*preset)
        {
            if((ptr = g_tree_lookup(s->freq_tree, GINT_TO_POINTER(*preset))))
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ptr), TRUE);
            preset++;
        }
    }
}

static void
ui_scanlist_apply(GtkWidget *widget,
                  gpointer   user_data)
{
    ui_scanlist_t *s = (ui_scanlist_t*)user_data;
    const gchar *custom;
    gchar *output;

    if(!ui.conn)
        return;

    custom = gtk_entry_get_text(GTK_ENTRY(s->e_custom));
    output = ui_scanlist_string(s->freq_tree, custom);
    if(!output)
    {
        ui_dialog(GTK_WINDOW(s->window),
                  GTK_MESSAGE_WARNING,
                  "Scan-list",
                  "<big>Unable to set scan-list</big>\n\nNo frequencies selected.");
        return;
    }

    mt_ssh_cmd(ui.conn, MT_SSH_CMD_SCANLIST, output);
    g_free(output);
}

static gchar*
ui_scanlist_string(GTree       *freq_tree,
                   const gchar *custom)
{
    GString *str;
    gchar *scanlist, *compress;
    gsize len;

    str = g_string_new(NULL);
    g_tree_foreach(freq_tree, ui_scanlist_string_foreach, str);

    if(custom && strlen(custom))
        g_string_append(str, custom);

    scanlist = g_string_free(str, FALSE);
    len = strlen(scanlist);

    if(len)
    {
        /* Remove ending comma */
        if(scanlist[len-1] == ',')
            scanlist[len-1] = '\0';

        /* Compress the frequency ranges */
        compress = str_scanlist_compress(scanlist);

        g_free(scanlist);
        return compress;
    }

    g_free(scanlist);
    return NULL;
}

static gboolean
ui_scanlist_string_foreach(gpointer key,
                           gpointer value,
                           gpointer data)
{
    GString *str = (GString*)data;
    gint freq = GPOINTER_TO_INT(key);
    GtkWidget *widget = (GtkWidget*)value;

    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
        g_string_append_printf(str, "%d,", freq);

    return FALSE;
}

static gboolean
ui_scanlist_set_foreach(gpointer key,
                        gpointer value,
                        gpointer user_data)
{
    GtkWidget *widget = (GtkWidget*)value;
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), GPOINTER_TO_INT(user_data));
    return FALSE;
}

static gboolean
freq_list_contains(const gint *list,
                  gint        freq)
{
    while(*list)
        if(*list++ == freq)
            return TRUE;
    return FALSE;
}
