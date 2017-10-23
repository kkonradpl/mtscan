#ifndef MTSCAN_UI_BUTTONS_H_
#define MTSCAN_UI_BUTTONS_H_
#include <gtk/gtk.h>

GtkWidget* ui_toolbar_create(void);

void ui_toolbar_connect_set_state(gboolean);
void ui_toolbar_scan_set_state(gboolean);

#endif
