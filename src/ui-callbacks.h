/*
 *  MTscan - MikroTik RouterOS wireless scanner
 *  Copyright (c) 2015-2019  Konrad Kosmatka
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */

#ifndef MTSCAN_UI_CALLBACKS_H_
#define MTSCAN_UI_CALLBACKS_H_
#include "ui.h"

void ui_callback_status(const mt_ssh_t*, const gchar*, const gchar*);
void ui_callback_verify(const mt_ssh_t*, const gchar*);
void ui_callback_connected(const mt_ssh_t*);
void ui_callback_disconnected(const mt_ssh_t*, gboolean);
void ui_callback_state(const mt_ssh_t*, gint);
void ui_callback_failure(const mt_ssh_t*, const gchar*);
void ui_callback_network(const mt_ssh_t*, network_t*);
void ui_callback_heartbeat(const mt_ssh_t*);
void ui_callback_scanlist(const mt_ssh_t*, const gchar*);

void ui_callback_tzsp(tzsp_receiver_t*);
void ui_callback_tzsp_network(const tzsp_receiver_t*, network_t*);

void ui_callback_geoloc(gint64);

#endif
