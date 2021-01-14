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

#ifndef MTSCAN_LOG_H_
#define MTSCAN_LOG_H_
#include <gtk/gtk.h>
#include "network.h"

#define LOG_READ_ERROR_EMPTY  0
#define LOG_READ_ERROR_OPEN  -1
#define LOG_READ_ERROR_READ  -2
#define LOG_READ_ERROR_PARSE -3

typedef struct log_save_error
{
    size_t wrote;
    size_t length;
    gboolean existing_file;
} log_save_error_t;


gint log_read(const gchar*, void (*)(network_t*, gpointer), gpointer, gboolean);
log_save_error_t* log_save(const gchar*, gboolean, gboolean, gboolean, GList*);

#endif
