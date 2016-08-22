#include <gdk/gdkkeysyms.h>
#include "conf.h"
#include "ui.h"
#include "ui-view.h"
#include "ui-dialogs.h"
#include "log.h"
#include "mt-ssh.h"
#include "scanlist.h"
#include "ui-connect.h"
#include "ui-toolbar.h"
#include "export.h"
#include "gps.h"

static gchar range_full[] = "4920-6000";
static gchar range_ext[] = "5330-5490,5720-5785,default,5790-5900,5055-5170,default,4920-5050,5905-6000,default";

static void ui_toolbar_connect(GtkWidget*, gpointer);
static void ui_toolbar_scan(GtkWidget*, gpointer);
static void ui_toolbar_restart(GtkWidget*, gpointer);
static void ui_toolbar_scanlist_default(GtkWidget*, gpointer);
static void ui_toolbar_scanlist_custom(GtkWidget*, gpointer);
static void ui_toolbar_scanlist(GtkWidget*, gpointer);

static void ui_toolbar_new(GtkWidget*, gpointer);
static void ui_toolbar_open(GtkWidget*, gpointer);
static void ui_toolbar_merge(GtkWidget*, gpointer);
static void ui_toolbar_save(GtkWidget*, gpointer);
static void ui_toolbar_save_as(GtkWidget*, gpointer);
static void ui_toolbar_export(GtkWidget*, gpointer);
static void ui_toolbar_screenshot(GtkWidget*, gpointer);

static void ui_toolbar_preferences(GtkWidget*, gpointer);
static void ui_toolbar_sound(GtkWidget*, gpointer);
static void ui_toolbar_mode(GtkWidget*, gpointer);
static void ui_toolbar_gps(GtkWidget*, gpointer);
static void ui_toolbar_signals(GtkWidget*, gpointer);
static void ui_toolbar_about(GtkWidget*, gpointer);

/* ToggleToolButton's 'clicked' callback contain
   some hacks, as the "toggled" signal does not
   work with gtk_widget_add_accelerator. */

GtkWidget*
ui_toolbar_create(GtkWidget *box)
{
    GtkAccelGroup *accel_group;
    GtkWidget *toolbar;

    accel_group = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(ui.window), accel_group);

    toolbar = gtk_toolbar_new();
    gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
    gtk_box_pack_start(GTK_BOX(box), toolbar, FALSE, FALSE, 0);

    ui.b_connect = gtk_toggle_tool_button_new();
    gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(ui.b_connect), gtk_image_new_from_stock(GTK_STOCK_CONNECT, GTK_ICON_SIZE_BUTTON));
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(ui.b_connect), "Connect");
    gtk_widget_set_tooltip_text(GTK_WIDGET(ui.b_connect), "Connect (Ctrl+1)");
    g_signal_connect(ui.b_connect, "clicked", G_CALLBACK(ui_toolbar_connect), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), ui.b_connect, -1);
    gtk_widget_add_accelerator(GTK_WIDGET(ui.b_connect), "clicked", accel_group, GDK_KEY_1, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    ui.b_scan = gtk_toggle_tool_button_new();
    gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(ui.b_scan), gtk_image_new_from_stock(GTK_STOCK_MEDIA_PLAY, GTK_ICON_SIZE_BUTTON));
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(ui.b_scan), "Scan");
    gtk_widget_set_tooltip_text(GTK_WIDGET(ui.b_scan), "Scan (Ctrl+2)");
    g_signal_connect(ui.b_scan, "clicked", G_CALLBACK(ui_toolbar_scan), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), ui.b_scan, -1);
    gtk_widget_add_accelerator(GTK_WIDGET(ui.b_scan), "clicked", accel_group, GDK_KEY_2, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    ui.b_restart = gtk_tool_button_new(gtk_image_new_from_stock(GTK_STOCK_REFRESH, GTK_ICON_SIZE_BUTTON), "Restart");
    gtk_widget_set_tooltip_text(GTK_WIDGET(ui.b_restart), "Restart (Ctrl+3)");
    g_signal_connect(ui.b_restart, "clicked", G_CALLBACK(ui_toolbar_restart), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), ui.b_restart, -1);
    gtk_widget_add_accelerator(GTK_WIDGET(ui.b_restart), "clicked", accel_group, GDK_KEY_3, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    ui.b_scanlist_default = gtk_tool_button_new(gtk_image_new_from_stock(GTK_STOCK_HOME, GTK_ICON_SIZE_BUTTON), "Default scan-list");
    gtk_widget_set_tooltip_text(GTK_WIDGET(ui.b_scanlist_default), "Default scan-list (Ctrl+4)");
    g_signal_connect(ui.b_scanlist_default, "clicked", G_CALLBACK(ui_toolbar_scanlist_default), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), ui.b_scanlist_default, -1);
    gtk_widget_add_accelerator(GTK_WIDGET(ui.b_scanlist_default), "clicked", accel_group, GDK_KEY_4, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    ui.b_scanlist = gtk_tool_button_new(gtk_image_new_from_stock(GTK_STOCK_EDIT, GTK_ICON_SIZE_BUTTON), "Custom scan-list");
    gtk_widget_set_tooltip_text(GTK_WIDGET(ui.b_scanlist), "Custom scan-list (Ctrl+L)");
    g_signal_connect(ui.b_scanlist, "clicked", G_CALLBACK(ui_toolbar_scanlist), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), ui.b_scanlist, -1);
    gtk_widget_add_accelerator(GTK_WIDGET(ui.b_scanlist), "clicked", accel_group, GDK_KEY_l, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    ui.b_scanlist_custom = gtk_menu_tool_button_new(gtk_image_new_from_stock(GTK_STOCK_SELECT_ALL, GTK_ICON_SIZE_BUTTON), "Custom scan-list");
    gtk_widget_set_tooltip_text(GTK_WIDGET(ui.b_scanlist_custom), "Full scan-list (Ctrl+5)");
    g_signal_connect(ui.b_scanlist_custom, "clicked", G_CALLBACK(ui_toolbar_scanlist_custom), range_ext);

    GtkWidget *menu = gtk_menu_new();
    GtkWidget *item_ext = gtk_image_menu_item_new_with_label("Extended scan-list");
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item_ext), gtk_image_new_from_stock(GTK_STOCK_SELECT_ALL, GTK_ICON_SIZE_MENU));
    g_signal_connect(item_ext, "activate", G_CALLBACK(ui_toolbar_scanlist_custom), range_ext);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_ext);

    GtkWidget *item_full = gtk_image_menu_item_new_with_label("Full scan-list (4920-6000)");
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item_full), gtk_image_new_from_stock(GTK_STOCK_SELECT_ALL, GTK_ICON_SIZE_MENU));
    g_signal_connect(item_full, "activate", G_CALLBACK(ui_toolbar_scanlist_custom), range_full);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_full);

    gtk_widget_show_all(menu);
    gtk_menu_tool_button_set_menu(GTK_MENU_TOOL_BUTTON(ui.b_scanlist_custom), menu);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), ui.b_scanlist_custom, -1);
    gtk_widget_add_accelerator(GTK_WIDGET(ui.b_scanlist_custom), "clicked", accel_group, GDK_KEY_5, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);

    ui.b_new = gtk_tool_button_new(gtk_image_new_from_stock(GTK_STOCK_NEW, GTK_ICON_SIZE_BUTTON), "New");
    gtk_widget_set_tooltip_text(GTK_WIDGET(ui.b_new), "New (Ctrl+N)");
    g_signal_connect(ui.b_new, "clicked", G_CALLBACK(ui_toolbar_new), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), ui.b_new, -1);
    gtk_widget_add_accelerator(GTK_WIDGET(ui.b_new), "clicked", accel_group, GDK_KEY_n, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    ui.b_open = gtk_tool_button_new(gtk_image_new_from_stock(GTK_STOCK_OPEN, GTK_ICON_SIZE_BUTTON), "Open");
    gtk_widget_set_tooltip_text(GTK_WIDGET(ui.b_open), "Open (Ctrl+O)");
    g_signal_connect(ui.b_open, "clicked", G_CALLBACK(ui_toolbar_open), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), ui.b_open, -1);
    gtk_widget_add_accelerator(GTK_WIDGET(ui.b_open), "clicked", accel_group, GDK_KEY_o, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    ui.b_merge = gtk_tool_button_new(gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_BUTTON), "Merge");
    gtk_widget_set_tooltip_text(GTK_WIDGET(ui.b_merge), "Merge (Ctrl+M)");
    g_signal_connect(ui.b_merge, "clicked", G_CALLBACK(ui_toolbar_merge), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), ui.b_merge, -1);
    gtk_widget_add_accelerator(GTK_WIDGET(ui.b_merge), "clicked", accel_group, GDK_KEY_m, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    ui.b_save = gtk_tool_button_new(gtk_image_new_from_stock(GTK_STOCK_SAVE, GTK_ICON_SIZE_BUTTON), "Save");
    gtk_widget_set_tooltip_text(GTK_WIDGET(ui.b_save), "Save (Ctrl+S)");
    g_signal_connect(ui.b_save, "clicked", G_CALLBACK(ui_toolbar_save), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), ui.b_save, -1);
    gtk_widget_add_accelerator(GTK_WIDGET(ui.b_save), "clicked", accel_group, GDK_KEY_s, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    ui.b_save_as = gtk_tool_button_new(gtk_image_new_from_stock(GTK_STOCK_SAVE_AS, GTK_ICON_SIZE_BUTTON), "Save as");
    gtk_widget_set_tooltip_text(GTK_WIDGET(ui.b_save_as), "Save as");
    g_signal_connect(ui.b_save_as, "clicked", G_CALLBACK(ui_toolbar_save_as), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), ui.b_save_as, -1);

    ui.b_export = gtk_tool_button_new(gtk_image_new_from_icon_name("mtscan-export", GTK_ICON_SIZE_BUTTON), "Export");
    gtk_widget_set_tooltip_text(GTK_WIDGET(ui.b_export), "Export as HTML (Ctrl+E)");
    gtk_tool_button_set_use_underline(GTK_TOOL_BUTTON(ui.b_export), TRUE);
    g_signal_connect(ui.b_export, "clicked", G_CALLBACK(ui_toolbar_export), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), ui.b_export, -1);
    gtk_widget_add_accelerator(GTK_WIDGET(ui.b_export), "clicked", accel_group, GDK_KEY_e, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    ui.b_screenshot = gtk_tool_button_new(gtk_image_new_from_icon_name("mtscan-screen", GTK_ICON_SIZE_BUTTON), "Screenshot");
    gtk_widget_set_tooltip_text(GTK_WIDGET(ui.b_screenshot), "Take a screenshot (Ctrl+Z)");
    gtk_tool_button_set_use_underline(GTK_TOOL_BUTTON(ui.b_screenshot), TRUE);
    g_signal_connect(ui.b_screenshot, "clicked", G_CALLBACK(ui_toolbar_screenshot), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), ui.b_screenshot, -1);
    gtk_widget_add_accelerator(GTK_WIDGET(ui.b_screenshot), "clicked", accel_group, GDK_KEY_z, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);

    ui.b_preferences = gtk_tool_button_new(gtk_image_new_from_stock(GTK_STOCK_PREFERENCES, GTK_ICON_SIZE_BUTTON), "Preferences");
    gtk_widget_set_tooltip_text(GTK_WIDGET(ui.b_preferences), "Preferences (CTRL+P)");
    g_signal_connect(ui.b_preferences, "clicked", G_CALLBACK(ui_toolbar_preferences), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), ui.b_preferences, -1);
    gtk_widget_add_accelerator(GTK_WIDGET(ui.b_preferences), "clicked", accel_group, GDK_KEY_p, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    ui.b_sound = gtk_toggle_tool_button_new();
    gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(ui.b_sound), gtk_image_new_from_icon_name("audio-volume-high", GTK_ICON_SIZE_BUTTON));
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(ui.b_sound), "Enable sound");
    gtk_widget_set_tooltip_text(GTK_WIDGET(ui.b_sound), "Enable sound");
    g_signal_connect(ui.b_sound, "clicked", G_CALLBACK(ui_toolbar_sound), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), ui.b_sound, -1);

    ui.b_mode = gtk_toggle_tool_button_new();
    gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(ui.b_mode), gtk_image_new_from_icon_name("mtscan-dark", GTK_ICON_SIZE_BUTTON));
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(ui.b_mode), "Dark mode");
    gtk_widget_set_tooltip_text(GTK_WIDGET(ui.b_mode), "Dark mode");
    g_signal_connect(ui.b_mode, "clicked", G_CALLBACK(ui_toolbar_mode), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), ui.b_mode, -1);

    ui.b_gps = gtk_toggle_tool_button_new();
    gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(ui.b_gps), gtk_image_new_from_icon_name("mtscan-gps", GTK_ICON_SIZE_BUTTON));
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(ui.b_gps), "Enable GPS");
    gtk_widget_set_tooltip_text(GTK_WIDGET(ui.b_gps), "Enable GPS");
    g_signal_connect(ui.b_gps, "clicked", G_CALLBACK(ui_toolbar_gps), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), ui.b_gps, -1);

    ui.b_signals = gtk_toggle_tool_button_new();
    gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(ui.b_signals), gtk_image_new_from_stock(GTK_STOCK_FILE, GTK_ICON_SIZE_BUTTON));
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(ui.b_signals), "Record all signal samples");
    gtk_widget_set_tooltip_text(GTK_WIDGET(ui.b_signals), "Record all signal samples");
    g_signal_connect(ui.b_signals, "clicked", G_CALLBACK(ui_toolbar_signals), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), ui.b_signals, -1);

    ui.b_about = gtk_tool_button_new(gtk_image_new_from_stock(GTK_STOCK_ABOUT, GTK_ICON_SIZE_BUTTON), "About");
    gtk_widget_set_tooltip_text(GTK_WIDGET(ui.b_about), "About " APP_NAME);
    g_signal_connect(ui.b_about, "clicked", G_CALLBACK(ui_toolbar_about), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), ui.b_about, -1);

    return toolbar;
}

static void
ui_toolbar_connect(GtkWidget *widget,
                   gpointer   data)
{
    if(conn)
    {
        /* Close connection */
        ui_toolbar_connect_set_state(TRUE);
        gtk_widget_set_sensitive(widget, FALSE);
        g_async_queue_push(conn->queue, conn_command_new(COMMAND_DISCONNECT, NULL));
        return;
    }

    /* Display connection dialog */
    ui_toolbar_connect_set_state(FALSE);
    connection_dialog();
}

void
ui_toolbar_connect_set_state(gboolean state)
{
    g_signal_handlers_block_by_func(G_OBJECT(ui.b_connect), GINT_TO_POINTER(ui_toolbar_connect), NULL);
    gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(ui.b_connect), state);
    g_signal_handlers_unblock_by_func(G_OBJECT(ui.b_connect), GINT_TO_POINTER(ui_toolbar_connect), NULL);
}

static void
ui_toolbar_scan(GtkWidget *widget,
                gpointer   data)
{
    ui_toolbar_scan_set_state(ui.scanning);
    if(conn)
    {
        if(ui.scanning)
            g_async_queue_push(conn->queue, conn_command_new(COMMAND_SCAN_STOP, NULL));
        else
            g_async_queue_push(conn->queue, conn_command_new(COMMAND_SCAN_START, NULL));
        gtk_widget_set_sensitive(widget, FALSE);
    }
}

void
ui_toolbar_scan_set_state(gboolean state)
{
    g_signal_handlers_block_by_func(G_OBJECT(ui.b_scan), GINT_TO_POINTER(ui_toolbar_scan), NULL);
    gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(ui.b_scan), state);
    g_signal_handlers_unblock_by_func(G_OBJECT(ui.b_scan), GINT_TO_POINTER(ui_toolbar_scan), NULL);
}

static void
ui_toolbar_restart(GtkWidget *widget,
                   gpointer   data)
{
    if(conn)
    {
        g_async_queue_push(conn->queue, conn_command_new(COMMAND_SCAN_RESTART, NULL));
        gtk_widget_set_sensitive(widget, FALSE);
    }
}

static void
ui_toolbar_scanlist_default(GtkWidget *widget,
                            gpointer   data)
{
    if(conn)
        g_async_queue_push(conn->queue, conn_command_new(COMMAND_SET_SCANLIST, g_strdup("default")));
}

static void
ui_toolbar_scanlist_custom(GtkWidget *widget,
                           gpointer   data)
{
    static gchar msg[] = "Are you sure to set the scan-list to full range: <b>%s</b>?\n\n"
                         "<b>Warning:</b> Using an antenna with poor SWR may lead to PA failure during active scanning. "
                         "If unsure, set tx-power to low value before continuing.";
    static gboolean set_scanlist = FALSE;
    GtkWidget *dialog;
    gchar* range = (gchar*)data;

    if(!set_scanlist)
    {
        dialog = gtk_message_dialog_new_with_markup(GTK_WINDOW(ui.window),
                                                    GTK_DIALOG_MODAL,
                                                    GTK_MESSAGE_WARNING,
                                                    GTK_BUTTONS_YES_NO,
                                                    msg,
                                                    range);

        gtk_window_set_title(GTK_WINDOW(dialog), APP_NAME);
        set_scanlist = (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_YES);
        gtk_widget_destroy(dialog);
    }

    if(set_scanlist && conn)
        g_async_queue_push(conn->queue, conn_command_new(COMMAND_SET_SCANLIST, g_strdup(range)));

}

static void
ui_toolbar_scanlist(GtkWidget *widget,
                    gpointer   data)
{
    scanlist_dialog();
}

static void
ui_toolbar_new(GtkWidget *widget,
               gpointer   data)
{
    if(ui.changed && !ui_dialog_ask_unsaved())
        return;
    ui_clear();
    ui_set_title(NULL);
}

static void
ui_toolbar_open(GtkWidget *widget,
                gpointer   data)
{
    ui_dialog_open(UI_DIALOG_OPEN);
}

static void
ui_toolbar_merge(GtkWidget *widget,
                 gpointer   data)
{
    ui_dialog_open(UI_DIALOG_OPEN_MERGE);
}

static void
ui_toolbar_save(GtkWidget *widget,
                gpointer   data)
{
    ui_dialog_save(UI_DIALOG_SAVE);
}

static void
ui_toolbar_save_as(GtkWidget *widget,
                   gpointer   data)
{
    ui_dialog_save(UI_DIALOG_SAVE_AS);
}

static void
ui_toolbar_export(GtkWidget *widget,
                  gpointer   data)
{
    ui_dialog_export();
}

static void
ui_toolbar_screenshot(GtkWidget *widget,
                      gpointer   data)
{
    ui_screenshot();
}

static void
ui_toolbar_preferences(GtkWidget *widget,
                       gpointer   data)
{
    /* TODO */
}

static void
ui_toolbar_sound(GtkWidget *widget,
                 gpointer   data)
{
    static gboolean pressed = FALSE;
    pressed = !pressed;
    conf_set_interface_sound(pressed);

    g_signal_handlers_block_by_func(G_OBJECT(widget), GINT_TO_POINTER(ui_toolbar_sound), NULL);
    gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(widget), pressed);
    g_signal_handlers_unblock_by_func(G_OBJECT(widget), GINT_TO_POINTER(ui_toolbar_sound), NULL);
}

static void
ui_toolbar_mode(GtkWidget *widget,
                gpointer   data)
{
    static gboolean pressed = FALSE;
    pressed = !pressed;
    ui_view_dark_mode(ui.treeview, pressed);
    conf_set_interface_dark_mode(pressed);

    g_signal_handlers_block_by_func(G_OBJECT(widget), GINT_TO_POINTER(ui_toolbar_mode), NULL);
    gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(widget), pressed);
    g_signal_handlers_unblock_by_func(G_OBJECT(widget), GINT_TO_POINTER(ui_toolbar_mode), NULL);
}

static void
ui_toolbar_gps(GtkWidget *widget,
               gpointer   data)
{
    static gboolean pressed = FALSE;
    pressed = !pressed;
    conf_set_interface_gps(pressed);

    if(pressed)
        gps_start("localhost", 2947);
    else
        gps_stop();

    g_signal_handlers_block_by_func(G_OBJECT(widget), GINT_TO_POINTER(ui_toolbar_gps), NULL);
    gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(widget), pressed);
    g_signal_handlers_unblock_by_func(G_OBJECT(widget), GINT_TO_POINTER(ui_toolbar_gps), NULL);
}

static void
ui_toolbar_signals(GtkWidget *widget,
                   gpointer   data)
{
    static gboolean pressed = FALSE;
    pressed = !pressed;
    conf_set_interface_signals(pressed);

    g_signal_handlers_block_by_func(G_OBJECT(widget), GINT_TO_POINTER(ui_toolbar_signals), NULL);
    gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(widget), pressed);
    g_signal_handlers_unblock_by_func(G_OBJECT(widget), GINT_TO_POINTER(ui_toolbar_signals), NULL);
}

static void
ui_toolbar_about(GtkWidget *widget,
                 gpointer   data)
{
    ui_dialog_about();
}
