/*
 *  MTscan - MikroTik RouterOS wireless scanner
 *  Copyright (c) 2015-2018  Konrad Kosmatka
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

#ifndef MTSCAN_UI_CONNECT_H_
#define MTSCAN_UI_CONNECT_H_

#include "mt-ssh.h"

typedef struct ui_connection ui_connection_t;

ui_connection_t* ui_connection_new(gint);
void ui_connection_set_status(ui_connection_t*, const gchar*, const gchar*);
gboolean ui_connection_verify(ui_connection_t*, const gchar*);
void ui_connection_connected(ui_connection_t*);
void ui_connection_disconnected(ui_connection_t*);


#endif


