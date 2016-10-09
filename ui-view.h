#ifndef MTSCAN_UI_VIEW_H_
#define MTSCAN_UI_VIEW_H_
#include <gtk/gtk.h>

GtkWidget* ui_view_new(mtscan_model_t*);
void ui_view_lock(GtkWidget*);
void ui_view_unlock(GtkWidget*);

void ui_view_remove_iter(GtkTreeView*, GtkTreeModel*, GtkTreeIter*);
void ui_view_check_position(GtkWidget*);
void ui_view_dark_mode(GtkWidget*, gboolean);
void ui_view_latlon_column(GtkWidget*, gboolean);

#endif

