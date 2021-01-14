/*
 *  MTscan - MikroTik RouterOS wireless scanner
 *  Copyright (c) 2015-2017  Konrad Kosmatka
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

#ifndef MTSCAN_CALLBACKS_H_
#define MTSCAN_CALLBACKS_H_

void callback_mt_ssh(mt_ssh_t *, mt_ssh_ret_t, const gchar *);
void callback_mt_ssh_msg(const mt_ssh_t *, mt_ssh_msg_type_t, gconstpointer);

#endif



