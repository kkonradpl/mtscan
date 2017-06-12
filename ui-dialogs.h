#ifndef MTSCAN_UI_DIALOGS_H_
#define MTSCAN_UI_DIALOGS_H_
#include <gtk/gtk.h>

enum
{
    UI_DIALOG_YES = GTK_RESPONSE_YES,
    UI_DIALOG_NO = GTK_RESPONSE_NO,
    UI_DIALOG_CANCEL = GTK_RESPONSE_CANCEL,
    UI_DIALOG_OPEN = 0,
    UI_DIALOG_MERGE = 1,
};

typedef struct
{
    gchar *filename;
    gboolean compress;
    gboolean strip_signals;
    gboolean strip_gps;
    gboolean strip_azi;
} ui_dialog_save_t;

void ui_dialog(GtkWindow*, GtkMessageType, const gchar*, const gchar*, ...);

GSList* ui_dialog_open(GtkWindow*, gboolean);
ui_dialog_save_t* ui_dialog_save(GtkWindow*);
gchar* ui_dialog_export(GtkWindow*);

gint ui_dialog_ask_unsaved(GtkWindow*);
gint ui_dialog_ask_open_or_merge(GtkWindow*);
gint ui_dialog_ask_merge(GtkWindow*, gint);
gint ui_dialog_yesno(GtkWindow*, const gchar*);

void ui_dialog_about(GtkWindow*);

#endif

