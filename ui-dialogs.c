#include <string.h>
#include "ui-dialogs.h"
#include "conf.h"
#include "mtscan.h"
#ifdef G_OS_WIN32
#include "win32.h"
#endif

static const gchar* filetype_default = APP_NAME " file";

static void ui_dialog_save_response(GtkWidget*, gint, gpointer);
static void ui_dialog_export_response(GtkWidget*, gint, gpointer);
static gboolean str_has_suffix(const gchar*, const gchar*);

void
ui_dialog(GtkWindow      *window,
          GtkMessageType  icon,
          const gchar    *title,
          const gchar    *format,
          ...)
{
    GtkWidget *dialog;
    va_list args;
    gchar *msg;

    va_start(args, format);
    msg = g_markup_vprintf_escaped(format, args);
    va_end(args);
    dialog = gtk_message_dialog_new(window,
                                    GTK_DIALOG_MODAL,
                                    icon,
                                    GTK_BUTTONS_CLOSE,
                                    NULL);
    gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG(dialog), msg);
    gtk_window_set_title(GTK_WINDOW(dialog), title);
    if(!window)
        gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    g_free(msg);
}

GSList*
ui_dialog_open(GtkWindow *window,
               gboolean   merge)
{
    GtkWidget *dialog;
    GtkFileFilter *filter, *filter_all;
    GSList *filenames = NULL;
    const gchar *dir;
    gchar *new_dir;

    dialog = gtk_file_chooser_dialog_new((merge ? "Merge files" : "Open file"),
                                         window,
                                         GTK_FILE_CHOOSER_ACTION_OPEN,
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);

    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), merge);

    if((dir = conf_get_path_log_open()))
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), dir);

    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, filetype_default);
    gtk_file_filter_add_pattern(filter, "*" APP_FILE_EXT);
    gtk_file_filter_add_pattern(filter, "*" APP_FILE_EXT APP_FILE_COMPRESS);
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    filter_all = gtk_file_filter_new();
    gtk_file_filter_set_name(filter_all, "All files");
    gtk_file_filter_add_pattern(filter_all, "*");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter_all);

    if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        filenames = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));
        new_dir = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dialog));
        conf_set_path_log_open(new_dir);
        g_free(new_dir);
    }

    gtk_widget_destroy(dialog);
    return filenames;
}

ui_dialog_save_t*
ui_dialog_save(GtkWindow *window)
{
    GtkWidget *dialog;
    GtkWidget *box;
    GtkWidget *compression;
    GtkWidget *strip_signals;
    GtkWidget *strip_gps;
    GtkWidget *strip_azi;
    GtkFileFilter *filter;
    const gchar *dir;
    gchar *new_dir;
    ui_dialog_save_t *ret = NULL;

    dialog = gtk_file_chooser_dialog_new("Save log as",
                                         window,
                                         GTK_FILE_CHOOSER_ACTION_SAVE,
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
                                         NULL);

    gtk_file_chooser_set_create_folders(GTK_FILE_CHOOSER(dialog), TRUE);
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

    if((dir = conf_get_path_log_save()))
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), dir);

    box = gtk_hbox_new(FALSE, 12);

    compression = gtk_check_button_new_with_label("Compress (.gz)");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(compression), TRUE);
    gtk_box_pack_start(GTK_BOX(box), compression, FALSE, FALSE, 0);
    g_object_set_data(G_OBJECT(dialog), "mtscan-compression", compression);

    strip_signals = gtk_check_button_new_with_label("Strip signal samples");
    gtk_box_pack_start(GTK_BOX(box), strip_signals, FALSE, FALSE, 0);
    g_object_set_data(G_OBJECT(dialog), "mtscan-strip-signals", strip_signals);

    strip_gps = gtk_check_button_new_with_label("Strip GPS data");
    gtk_box_pack_start(GTK_BOX(box), strip_gps, FALSE, FALSE, 0);
    g_object_set_data(G_OBJECT(dialog), "mtscan-strip-gps", strip_gps);

    strip_azi = gtk_check_button_new_with_label("Strip azimuth data");
    gtk_box_pack_start(GTK_BOX(box), strip_azi, FALSE, FALSE, 0);
    g_object_set_data(G_OBJECT(dialog), "mtscan-strip-azi", strip_azi);

    gtk_widget_show_all(box);
    gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(dialog), box);

    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, filetype_default);
    gtk_file_filter_add_pattern(filter, "*" APP_FILE_EXT);
    gtk_file_filter_add_pattern(filter, "*" APP_FILE_EXT APP_FILE_COMPRESS);
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    g_signal_connect(dialog, "response", G_CALLBACK(ui_dialog_save_response), &ret);
    while(gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_NONE);

    if(ret)
    {
        new_dir = g_path_get_dirname(ret->filename);
        conf_set_path_log_save(new_dir);
        g_free(new_dir);
    }

    return ret;
}

static void
ui_dialog_save_response(GtkWidget *dialog,
                        gint       response_id,
                        gpointer   user_data)
{
    ui_dialog_save_t **ret = (ui_dialog_save_t**)user_data;
    gchar *filename;
    gboolean compress;
    gboolean strip_signals;
    gboolean strip_gps;
    gboolean strip_azi;
    gboolean add_suffix;

    if(response_id != GTK_RESPONSE_ACCEPT)
    {
        gtk_widget_destroy(dialog);
        return;
    }

    if(!(filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog))))
    {
        ui_dialog(GTK_WINDOW(dialog),
                  GTK_MESSAGE_ERROR,
                  "Error",
                  "No file selected.");
        return;
    }

    compress = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_object_get_data(G_OBJECT(dialog), "mtscan-compression")));
    strip_signals = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_object_get_data(G_OBJECT(dialog), "mtscan-strip-signals")));
    strip_gps = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_object_get_data(G_OBJECT(dialog), "mtscan-strip-gps")));
    strip_azi = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_object_get_data(G_OBJECT(dialog), "mtscan-strip-azi")));

    if(compress)
        add_suffix = !str_has_suffix(filename, APP_FILE_EXT APP_FILE_COMPRESS);
    else
        add_suffix = !str_has_suffix(filename, APP_FILE_EXT);

    if(add_suffix)
    {
        if(!compress)
        {
            filename = (gchar*)g_realloc(filename, strlen(filename) + strlen(APP_FILE_EXT) + 1);
            strcat(filename, APP_FILE_EXT);
        }
        else
        {
            if(str_has_suffix(filename, APP_FILE_EXT))
            {
                filename = (gchar*)g_realloc(filename, strlen(filename) + strlen(APP_FILE_COMPRESS) + 1);
                strcat(filename, APP_FILE_COMPRESS);
            }
            else
            {
                filename = (gchar*)g_realloc(filename, strlen(filename) + strlen(APP_FILE_EXT APP_FILE_COMPRESS) + 1);
                strcat(filename, APP_FILE_EXT APP_FILE_COMPRESS);
            }
        }

        /* After adding the suffix, the GTK should check whether we can overwrite something. */
        gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), filename);
        gtk_dialog_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
        g_free(filename);
        return;
    }

    *ret = g_malloc(sizeof(ui_dialog_save_t));
    (*ret)->filename = filename;
    (*ret)->compress = compress;
    (*ret)->strip_signals = strip_signals;
    (*ret)->strip_gps = strip_gps;
    (*ret)->strip_azi = strip_azi;

    gtk_widget_destroy(dialog);
}

gchar*
ui_dialog_export(GtkWindow *window)
{
    GtkWidget *dialog;
    GtkFileFilter *filter;
    const gchar *dir;
    gchar *new_dir;
    gchar *filename = NULL;

    dialog = gtk_file_chooser_dialog_new("Export log as HTML",
                                         window,
                                         GTK_FILE_CHOOSER_ACTION_SAVE,
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
                                         NULL);

    gtk_file_chooser_set_create_folders(GTK_FILE_CHOOSER(dialog), TRUE);
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

    if((dir = conf_get_path_log_export()))
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), dir);

    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "HTML file");
    gtk_file_filter_add_pattern(filter, "*.html");
    gtk_file_filter_add_pattern(filter, "*.htm");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    g_signal_connect(dialog, "response", G_CALLBACK(ui_dialog_export_response), &filename);
    while(gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_NONE);

    if(filename)
    {
        new_dir = g_path_get_dirname(filename);
        conf_set_path_log_export(new_dir);
        g_free(new_dir);
    }

    return filename;
}

static void
ui_dialog_export_response(GtkWidget *dialog,
                          gint       response_id,
                          gpointer   user_data)
{
    gchar **ret = (gchar**)user_data;
    gchar *filename;
    gchar *ext;

    if(response_id != GTK_RESPONSE_ACCEPT)
    {
        gtk_widget_destroy(dialog);
        return;
    }

    if(!(filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog))))
    {
        ui_dialog(GTK_WINDOW(dialog),
                  GTK_MESSAGE_ERROR,
                  "Error",
                  "No file selected.");
        return;
    }

    ext = strrchr(filename, '.');
    if(!ext || (g_ascii_strcasecmp(ext, ".html") && g_ascii_strcasecmp(ext, ".htm")))
    {
        filename = (gchar*)g_realloc(filename, strlen(filename) + strlen(".html") + 1);
        strcat(filename, ".html");

        /* After adding the suffix, the GTK should check whether we can overwrite something. */
        gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), filename);
        gtk_dialog_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
        g_free(filename);
        return;
    }

    *ret = filename;
    gtk_widget_destroy(dialog);
}

gint
ui_dialog_ask_unsaved(GtkWindow *window)
{
    GtkWidget *dialog;
    gint response;

    dialog = gtk_message_dialog_new(window,
                                    GTK_DIALOG_MODAL,
                                    GTK_MESSAGE_QUESTION,
                                    GTK_BUTTONS_NONE,
                                    "There are some unsaved changes.\nDo you want to save them?");
    gtk_dialog_add_buttons(GTK_DIALOG(dialog),
                           GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                           GTK_STOCK_SAVE, GTK_RESPONSE_YES,
                           GTK_STOCK_DISCARD, GTK_RESPONSE_NO,
                           NULL);
    gtk_window_set_title(GTK_WINDOW(dialog), APP_NAME);
    response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    return response;
}

gint
ui_dialog_ask_open_or_merge(GtkWindow *window)
{
    GtkWidget *dialog;
    gint response;
    dialog = gtk_message_dialog_new(window,
                                    GTK_DIALOG_MODAL,
                                    GTK_MESSAGE_QUESTION,
                                    GTK_BUTTONS_NONE,
                                    "Do you want to open this log\nor merge with currently opened?");
    gtk_dialog_add_buttons(GTK_DIALOG(dialog),
                           GTK_STOCK_OPEN, GTK_RESPONSE_NO,
                           GTK_STOCK_ADD, GTK_RESPONSE_YES,
                           GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                           NULL);
    gtk_window_set_title(GTK_WINDOW(dialog), APP_NAME);
    response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    switch(response)
    {
    case GTK_RESPONSE_NO:
        return UI_DIALOG_OPEN;
    case GTK_RESPONSE_YES:
        return UI_DIALOG_MERGE;
    default:
        return UI_DIALOG_CANCEL;
    }
}

gint
ui_dialog_ask_merge(GtkWindow *window,
                    gint       count)
{
    GtkWidget *dialog;
    gint response;
    dialog = gtk_message_dialog_new(window,
                                    GTK_DIALOG_MODAL,
                                    GTK_MESSAGE_QUESTION,
                                    GTK_BUTTONS_NONE,
                                    "Do you want to merge all logs?\n\nSelected files: %d", count);
    gtk_dialog_add_buttons(GTK_DIALOG(dialog),
                           GTK_STOCK_ADD, GTK_RESPONSE_YES,
                           GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                           NULL);
    gtk_window_set_title(GTK_WINDOW(dialog), APP_NAME);
    response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    switch(response)
    {
    case GTK_RESPONSE_YES:
        return UI_DIALOG_MERGE;
    default:
        return UI_DIALOG_CANCEL;
    }
}

gint
ui_dialog_yesno(GtkWindow   *window,
                const gchar *message)
{
    GtkWidget *dialog;
    gboolean response;

    dialog = gtk_message_dialog_new(window,
                                    GTK_DIALOG_MODAL,
                                    GTK_MESSAGE_QUESTION,
                                    GTK_BUTTONS_NONE,
                                    message);
    gtk_dialog_add_buttons(GTK_DIALOG(dialog),
                           GTK_STOCK_NO, GTK_RESPONSE_NO,
                           GTK_STOCK_YES, GTK_RESPONSE_YES,
                           NULL);
    gtk_window_set_title(GTK_WINDOW(dialog), APP_NAME);

    response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    return (response == GTK_RESPONSE_YES ? GTK_RESPONSE_YES : GTK_RESPONSE_NO);
}

void
ui_dialog_about(GtkWindow *window)
{
    GtkWidget *dialog = gtk_about_dialog_new();
    gtk_window_set_icon_name(GTK_WINDOW(dialog), GTK_STOCK_ABOUT);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), window);
    gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(dialog), APP_NAME);
    gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dialog), APP_VERSION);
    gtk_about_dialog_set_logo_icon_name(GTK_ABOUT_DIALOG(dialog), APP_ICON);
    gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(dialog), "Copyright © 2015-2017  Konrad Kosmatka");
    gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(dialog), "Mikrotik RouterOS wireless scanner");
    gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(dialog), "http://fmdx.pl/mtscan");
    gtk_about_dialog_set_license(GTK_ABOUT_DIALOG(dialog), APP_LICENCE);
#ifdef G_OS_WIN32
    g_signal_connect(dialog, "activate-link", G_CALLBACK(win32_uri_signal), NULL);
#endif
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static gboolean
str_has_suffix(const gchar *string,
               const gchar *suffix)
{
    size_t string_len = strlen(string);
    size_t suffix_len = strlen(suffix);

    if(string_len < suffix_len)
        return FALSE;

    return g_ascii_strncasecmp(string + string_len - suffix_len, suffix, suffix_len) == 0;
}
