#ifndef MTSCAN_UI_CONNECT_H_
#define MTSCAN_UI_CONNECT_H_

#include "mt-ssh.h"

typedef struct ui_connection ui_connection_t;

ui_connection_t* ui_connection_new();
void ui_connection_set_status(ui_connection_t*, const gchar*, const gchar*);
gboolean ui_connection_verify(ui_connection_t*, const gchar*);
void ui_connection_connected(ui_connection_t*);
void ui_connection_disconnected(ui_connection_t*);


#endif


