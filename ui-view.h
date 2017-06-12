#ifndef MTSCAN_UI_VIEW_H_
#define MTSCAN_UI_VIEW_H_
#include <gtk/gtk.h>
#include "model.h"

GtkWidget* ui_view_new(mtscan_model_t*, gint);
void ui_view_configure(GtkWidget*);
void ui_view_set_icon_size(GtkWidget*, gint);
void ui_view_lock(GtkWidget*);
void ui_view_unlock(GtkWidget*);

void ui_view_remove_iter(GtkWidget*, GtkTreeIter*, gboolean);
void ui_view_remove_selection(GtkWidget*);
void ui_view_check_position(GtkWidget*);
void ui_view_dark_mode(GtkWidget*, gboolean);
void ui_view_latlon_column(GtkWidget*, gboolean);
void ui_view_azimuth_column(GtkWidget*, gboolean);

#endif

