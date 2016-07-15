#include <string.h>
#include "ui.h"
#include "ui-dialogs.h"
#include "log.h"
#include "export.h"
#ifdef G_OS_WIN32
#include "win32.h"
#endif

static const gchar* filetype_default = APP_NAME " file";

static void ui_dialog_save_response(GtkWidget*, gint, gpointer);
static gboolean str_has_suffix(const gchar*, const gchar*);
static void ui_dialog_export_response(GtkWidget*, gint, gpointer);

void
ui_dialog(GtkWidget      *window,
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
    dialog = gtk_message_dialog_new(GTK_WINDOW(window),
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

void
ui_dialog_open(gboolean merge)
{
    GtkWidget *dialog;
    GtkFileFilter *filter, *filter_all;
    GSList *filenames = NULL;

    dialog = gtk_file_chooser_dialog_new((!merge ? "Open file" : "Merge file"),
                                         GTK_WINDOW(ui.window),
                                         GTK_FILE_CHOOSER_ACTION_OPEN,
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);

    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, filetype_default);
    gtk_file_filter_add_pattern(filter, "*" APP_FILE_EXT);
    gtk_file_filter_add_pattern(filter, "*" APP_FILE_EXT ".gz");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    filter_all = gtk_file_filter_new();
    gtk_file_filter_set_name(filter_all, "All files");
    gtk_file_filter_add_pattern(filter_all, "*");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter_all);

    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), merge);
    if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        filenames = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));
        log_open(filenames, merge);
        g_slist_free_full(filenames, g_free);
    }
    gtk_widget_destroy(dialog);
}

gboolean
ui_dialog_save(gboolean save_as)
{
    GtkWidget *dialog;
    GtkWidget *check_buttton;
    GtkFileFilter *filter;
    gboolean state = FALSE;

    if(!ui.filename || save_as)
    {
        dialog = gtk_file_chooser_dialog_new("Save log as",
                                             GTK_WINDOW(ui.window),
                                             GTK_FILE_CHOOSER_ACTION_SAVE,
                                             GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                             GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
                                             NULL);

        gtk_file_chooser_set_create_folders(GTK_FILE_CHOOSER(dialog), TRUE);
        gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

        check_buttton = gtk_check_button_new_with_label("Compress (.gz)");
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_buttton), TRUE);
        gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(dialog), check_buttton);

        filter = gtk_file_filter_new();
        gtk_file_filter_set_name(filter, filetype_default);
        gtk_file_filter_add_pattern(filter, "*" APP_FILE_EXT);
        gtk_file_filter_add_pattern(filter, "*" APP_FILE_EXT ".gz");
        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

        g_signal_connect(dialog, "response", G_CALLBACK(ui_dialog_save_response), &state);
        while(gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_NONE);
    }
    else
    {
        state = log_save(ui.filename);
        if(state)
        {
            /* update the window title */
            ui.changed = FALSE;
            ui_set_title(ui.filename);
        }
    }
    return state;
}

static void
ui_dialog_save_response(GtkWidget *dialog,
                        gint       response_id,
                        gpointer   user_data)
{
    gboolean *state = (gboolean*)user_data;
    gchar *filename;
    gboolean compress;
    gboolean add_suffix;

    if(response_id != GTK_RESPONSE_ACCEPT)
    {
        gtk_widget_destroy(dialog);
        return;
    }

    if(!(filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog))))
    {
        ui_dialog(dialog,
                  GTK_MESSAGE_ERROR,
                  "Error",
                  "Unable to save a file.");
        return;
    }

    compress = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_file_chooser_get_extra_widget(GTK_FILE_CHOOSER(dialog))));

    if(compress)
        add_suffix = !str_has_suffix(filename, APP_FILE_EXT ".gz");
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
                filename = (gchar*)g_realloc(filename, strlen(filename) + strlen(".gz") + 1);
                strcat(filename, ".gz");
            }
            else
            {
                filename = (gchar*)g_realloc(filename, strlen(filename) + strlen(APP_FILE_EXT ".gz") + 1);
                strcat(filename, APP_FILE_EXT ".gz");
            }
        }

        gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), filename);
        gtk_dialog_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
        g_free(filename);
        return;
    }

    if(!log_save(filename))
    {
        g_free(filename);
        return;
    }

    /* update the window title */
    ui.changed = FALSE;
    ui_set_title(filename);

    *state = TRUE;
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

gboolean
ui_dialog_export()
{
    GtkWidget *dialog;
    GtkFileFilter *filter;
    gboolean state = FALSE;

    dialog = gtk_file_chooser_dialog_new("Export log as HTML",
                                         GTK_WINDOW(ui.window),
                                         GTK_FILE_CHOOSER_ACTION_SAVE,
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
                                         NULL);

    gtk_file_chooser_set_create_folders(GTK_FILE_CHOOSER(dialog), TRUE);
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "HTML file");
    gtk_file_filter_add_pattern(filter, "*.html");
    gtk_file_filter_add_pattern(filter, "*.htm");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    g_signal_connect(dialog, "response", G_CALLBACK(ui_dialog_export_response), &state);
    while(gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_NONE);
    return state;
}

static void
ui_dialog_export_response(GtkWidget *dialog,
                          gint       response_id,
                          gpointer   user_data)
{
    gboolean *state = (gboolean*)user_data;;
    gchar *filename;
    gchar *ext;
    FILE *fp;

    if(response_id != GTK_RESPONSE_ACCEPT)
    {
        gtk_widget_destroy(dialog);
        return;
    }

    if(!(filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog))))
    {
        ui_dialog(dialog, GTK_MESSAGE_ERROR, "Error", "Unable to save a file.");
        return;
    }

    ext = strrchr(filename, '.');
    if(!ext || (g_ascii_strcasecmp(ext, ".html") && g_ascii_strcasecmp(ext, ".htm")))
    {
        filename = (gchar*)g_realloc(filename, strlen(filename) + strlen(".html") + 1);
        strcat(filename, ".html");

        gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), filename);
        gtk_dialog_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
        g_free(filename);
        return;
    }

    if(!(fp = fopen(filename, "w")))
    {
        ui_dialog(dialog, GTK_MESSAGE_ERROR, "Error", "Unable to save a file:\n%s", filename);
        g_free(filename);
        return;
    }
    g_free(filename);

    export_html(ui.model, fp, ui.name);
    fclose(fp);

    *state = TRUE;
    gtk_widget_destroy(dialog);
}

gboolean
ui_dialog_ask_unsaved()
{
    GtkWidget *dialog;
    gint response;

    dialog = gtk_message_dialog_new(GTK_WINDOW(ui.window),
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
    if(response == GTK_RESPONSE_YES)
        return ui_dialog_save(UI_DIALOG_SAVE);
    return (response == GTK_RESPONSE_NO);
}

gint
ui_dialog_ask_open_or_merge()
{
    GtkWidget *dialog;
    gint response;
    dialog = gtk_message_dialog_new(GTK_WINDOW(ui.window),
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
        return UI_DIALOG_OPEN_MERGE;
    default:
        return UI_DIALOG_CANCEL;
    }
}

gint
ui_dialog_ask_merge(gint count)
{
    GtkWidget *dialog;
    gint response;
    dialog = gtk_message_dialog_new(GTK_WINDOW(ui.window),
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
        return UI_DIALOG_OPEN_MERGE;
    default:
        return UI_DIALOG_CANCEL;
    }
}

gboolean
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

    response = (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_YES);
    gtk_widget_destroy(dialog);
    return response;
}

void
ui_dialog_about()
{
    GtkWidget *dialog = gtk_about_dialog_new();
    gtk_window_set_icon_name(GTK_WINDOW(dialog), GTK_STOCK_ABOUT);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(ui.window));
    gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(dialog), APP_NAME);
    gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dialog), APP_VERSION);
    gtk_about_dialog_set_logo_icon_name(GTK_ABOUT_DIALOG(dialog), APP_ICON);
    gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(dialog), "Copyright © 2015-2016  Konrad Kosmatka");
    gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(dialog), "Mikrotik RouterOS wireless scanner");
    gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(dialog), "http://fmdx.pl/mtscan");
    gtk_about_dialog_set_license(GTK_ABOUT_DIALOG(dialog), APP_LICENCE);
#ifdef G_OS_WIN32
    g_signal_connect(dialog, "activate-link", G_CALLBACK(win32_uri_signal), NULL);
#endif
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}
