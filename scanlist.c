#include <string.h>
#include <gdk/gdkkeysyms.h>
#include "ui.h"
#include "conn.h"
#include "scanlist.h"

#define FREQ_2GHZ_START 2192
#define FREQ_2GHZ_END   2732
#define FREQ_2GHZ_INC      5

#define FREQ_5GHZ_START 4800
#define FREQ_5GHZ_END   6100
#define FREQ_5GHZ_INC      5

#define FREQS_PER_LINE 20

GtkWidget *scanlist_window = NULL;
GHashTable *freqmap;

/* TODO
static const gint freq_2GHz[] =
{
    2412, 2417, 2422, 2427, 2432, 2437, 2442, 2447, 2452, 2457, 2462, 2467, 2472, 2484, 0
};
*/

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

static void scanlist_destroy(GtkWidget*, gpointer);
static void scanlist_preset(GtkWidget*, gpointer);
static void scanlist_set(GtkWidget*, gpointer);
static void scanlist_foreach_str(gpointer, gpointer, gpointer);
static void scanlist_all(GtkWidget*, gpointer);
static void scanlist_clear(GtkWidget*, gpointer);
static void scanlist_foreach_set(gpointer, gpointer, gpointer);
static gboolean scanlist_key(GtkWidget*, GdkEventKey*, gpointer);
static gboolean freq_in_list(gint, const gint*);

gboolean
scanlist_active()
{
    return GPOINTER_TO_INT(scanlist_window);
}

void
scanlist_dialog()
{
    GtkWidget *content, *notebook;
    GtkWidget *page_5G, *frequencies5;
    GtkWidget *freq_box, *freq_button, *freq_label;
    GtkWidget *box_button, *b_clear, *b_all, *b_preset_lo;
    GtkWidget *b_preset_ext, *b_preset_upp, *b_apply, *b_close;
    gchar buff[100];
    gint freq;

    if(scanlist_active())
    {
        gtk_window_present(GTK_WINDOW(scanlist_window));
        return;
    }

    freqmap = g_hash_table_new(g_direct_hash, g_direct_equal);

    scanlist_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_modal(GTK_WINDOW(scanlist_window), FALSE);
    gtk_window_set_resizable(GTK_WINDOW(scanlist_window), FALSE);
    gtk_window_set_title(GTK_WINDOW(scanlist_window), "Scan-list");
    gtk_window_set_destroy_with_parent(GTK_WINDOW(scanlist_window), TRUE);
    gtk_container_set_border_width(GTK_CONTAINER(scanlist_window), 2);
    gtk_window_set_transient_for(GTK_WINDOW(scanlist_window), GTK_WINDOW(ui.window));
    gtk_window_set_position(GTK_WINDOW(scanlist_window), GTK_WIN_POS_CENTER_ON_PARENT);

    content = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(scanlist_window), content);

    notebook = gtk_notebook_new();
    gtk_notebook_popup_enable(GTK_NOTEBOOK(notebook));
    gtk_container_add(GTK_CONTAINER(content), notebook);

    page_5G = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(page_5G), 4);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page_5G, gtk_label_new("5 GHz"));
    gtk_container_child_set(GTK_CONTAINER(notebook), page_5G, "tab-expand", FALSE, "tab-fill", FALSE, NULL);
    frequencies5 = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(page_5G), frequencies5);

    for(freq=FREQ_5GHZ_START; freq<=FREQ_5GHZ_END; freq+=FREQ_5GHZ_INC)
    {
        if((((freq-FREQ_5GHZ_START)/FREQ_5GHZ_INC) % FREQS_PER_LINE) == 0)
        {
            freq_box = gtk_vbox_new(FALSE, 0);
            gtk_box_pack_start(GTK_BOX(frequencies5), freq_box, FALSE, FALSE, 0);
        }

        if(freq_in_list(freq, freq_5GHz_5180_5320))
            snprintf(buff, sizeof(buff), "<span color=\"#ff0000\"><b>%d</b></span>", freq);
        else if(freq_in_list(freq, freq_5GHz_5500_5700))
            snprintf(buff, sizeof(buff), "<span color=\"#ff00ff\"><b>%d</b></span>", freq);
        else if(freq_in_list(freq, freq_5GHz_5745_5825))
            snprintf(buff, sizeof(buff), "<span color=\"#0000ff\"><b>%d</b></span>", freq);
        else
            snprintf(buff, sizeof(buff), "%d", freq);

        freq_button = gtk_check_button_new();
        freq_label = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(freq_label), buff);
        gtk_container_add(GTK_CONTAINER(freq_button), freq_label);
        gtk_box_pack_start(GTK_BOX(freq_box), freq_button, FALSE, FALSE, 1);
        g_hash_table_insert(freqmap, GINT_TO_POINTER(freq), freq_button);
    }

    box_button = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(box_button), GTK_BUTTONBOX_END);
    gtk_box_set_spacing(GTK_BOX(box_button), 5);
    gtk_box_pack_start(GTK_BOX(content), box_button, FALSE, FALSE, 5);

    b_clear = gtk_button_new_from_stock(GTK_STOCK_CLEAR);
    g_signal_connect(b_clear, "clicked", G_CALLBACK(scanlist_clear), freqmap);
    gtk_container_add(GTK_CONTAINER(box_button), b_clear);

    b_all = gtk_button_new_with_label("Select all");
    g_signal_connect(b_all, "clicked", G_CALLBACK(scanlist_all), freqmap);
    gtk_container_add(GTK_CONTAINER(box_button), b_all);

    b_preset_lo = gtk_button_new_with_label("5180-5320");
    g_signal_connect(b_preset_lo, "clicked", G_CALLBACK(scanlist_preset), (gpointer)freq_5GHz_5180_5320);
    gtk_container_add(GTK_CONTAINER(box_button), b_preset_lo);

    b_preset_ext = gtk_button_new_with_label("5500-5700");
    g_signal_connect(b_preset_ext, "clicked", G_CALLBACK(scanlist_preset), (gpointer)freq_5GHz_5500_5700);
    gtk_container_add(GTK_CONTAINER(box_button), b_preset_ext);

    b_preset_upp = gtk_button_new_with_label("5745-5825");
    g_signal_connect(b_preset_upp, "clicked", G_CALLBACK(scanlist_preset), (gpointer)freq_5GHz_5745_5825);
    gtk_container_add(GTK_CONTAINER(box_button), b_preset_upp);

    b_apply = gtk_button_new_from_stock(GTK_STOCK_APPLY);
    g_signal_connect(b_apply, "clicked", G_CALLBACK(scanlist_set), freqmap);
    gtk_container_add(GTK_CONTAINER(box_button), b_apply);

    b_close = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
    g_signal_connect_swapped(b_close, "clicked", G_CALLBACK(gtk_widget_destroy), scanlist_window);
    gtk_container_add(GTK_CONTAINER(box_button), b_close);

    g_signal_connect(scanlist_window, "key-press-event", G_CALLBACK(scanlist_key), NULL);
    g_signal_connect(scanlist_window, "destroy", G_CALLBACK(scanlist_destroy), NULL);
    gtk_widget_show_all(scanlist_window);

    /* Scan-list is now centered over parent, don't stay on top anymore */
    gtk_window_set_transient_for(GTK_WINDOW(scanlist_window), NULL);
}

static void
scanlist_destroy(GtkWidget *widget,
                 gpointer   data)
{
    g_hash_table_destroy(freqmap);
    scanlist_window = NULL;
}

static void
scanlist_preset(GtkWidget *widget,
                gpointer   data)
{
    gint *preset = (gint*)data;
    gpointer ptr;
    while(*preset)
    {
        if((ptr = g_hash_table_lookup(freqmap, GINT_TO_POINTER(*preset))))
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ptr), TRUE);
        preset++;
    }
}

static void
scanlist_set(GtkWidget *widget,
             gpointer   data)
{
    GHashTable *freqmap = (GHashTable*)data;
    GString *str;
    gchar *output;
    gint len;

    if(!conn)
        return;

    str = g_string_new("");
    g_hash_table_foreach(freqmap, scanlist_foreach_str, str);
    output = g_string_free(str, FALSE);
    len = strlen(output);
    if(output[0] == '\0')
    {
        /* No frequencies selected, fallback to default scanlist */
        g_free(output);
        output = g_strdup("default");
    }
    else
    {
        /* Remove ending comma */
        output[len-1] = '\0';
    }
    g_async_queue_push(conn->queue, conn_command_new(COMMAND_SET_SCANLIST, output));
}

static void
scanlist_foreach_str(gpointer key,
                     gpointer value,
                     gpointer data)
{
    GString *str = (GString*)data;
    gint freq = GPOINTER_TO_INT(key);
    GtkWidget *widget = (GtkWidget*)value;
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
        g_string_append_printf(str, "%d,", freq);
}

static void
scanlist_all(GtkWidget *widget,
             gpointer   data)
{
    GHashTable *freqmap = (GHashTable*)data;
    g_hash_table_foreach(freqmap, scanlist_foreach_set, GINT_TO_POINTER(TRUE));
}

static void
scanlist_clear(GtkWidget *widget,
               gpointer   data)
{
    GHashTable *freqmap = (GHashTable*)data;
    g_hash_table_foreach(freqmap, scanlist_foreach_set, GINT_TO_POINTER(FALSE));
}

static void
scanlist_foreach_set(gpointer key,
                     gpointer value,
                     gpointer data)
{
    GtkWidget *widget = (GtkWidget*)value;
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), GPOINTER_TO_INT(data));
}

static gboolean
scanlist_key(GtkWidget   *widget,
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
freq_in_list(gint        freq,
             const gint *list)
{
    while(*list)
        if(*list++ == freq)
            return TRUE;
    return FALSE;
}

void
scanlist_add(gint frequency)
{
    gpointer ptr;
    if(!scanlist_active())
        return;

    if((ptr = g_hash_table_lookup(freqmap, GINT_TO_POINTER(frequency))))
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ptr), TRUE);
}
