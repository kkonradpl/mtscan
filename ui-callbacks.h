#ifndef MTSCAN_UI_CALLBACKS_H_
#define MTSCAN_UI_CALLBACKS_H_
#include "ui.h"

void ui_callback_status(const mt_ssh_t*, const gchar*, const gchar*);
void ui_callback_verify(const mt_ssh_t*, const gchar*);
void ui_callback_connected(const mt_ssh_t*, const gchar*);
void ui_callback_disconnected(const mt_ssh_t*);
void ui_callback_scanning_state(const mt_ssh_t*, gboolean);
void ui_callback_scanning_error(const mt_ssh_t*, const gchar*);
void ui_callback_network(const mt_ssh_t *, network_t *);
void ui_callback_heartbeat(const mt_ssh_t*);
void ui_callback_scanlist(const mt_ssh_t*, const gchar*);

#endif

