#ifndef MTSCAN_UI_DIALOGS_H_
#define MTSCAN_UI_DIALOGS_H_
#include <gtk/gtk.h>

#define UI_DIALOG_CANCEL    -1
#define UI_DIALOG_OPEN       0
#define UI_DIALOG_OPEN_MERGE 1
#define UI_DIALOG_SAVE       0
#define UI_DIALOG_SAVE_AS    1

void ui_dialog(GtkWidget*, GtkMessageType, const gchar*, const gchar*, ...);

void ui_dialog_open(gboolean);
gboolean ui_dialog_save(gboolean);
gboolean ui_dialog_export();

gboolean ui_dialog_ask_unsaved();
gint ui_dialog_ask_open_or_merge();
gint ui_dialog_ask_merge(gint);

gboolean ui_dialog_yesno(GtkWindow*, const gchar*);

void ui_dialog_about();

#endif

