#ifndef MTSCAN_UI_CONNECT_H_
#define MTSCAN_UI_CONNECT_H_
#include "ui.h"

void connection_dialog(void);
void connection_dialog_connected(gint);
gboolean connection_callback(gpointer);
gboolean connection_callback_info(gpointer);

#endif


