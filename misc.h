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

#ifndef MTSCAN_MISC_H_
#define MTSCAN_MISC_H_

gint gptrcmp(gconstpointer, gconstpointer);
gint gint64cmp(const gint64*, const gint64*);
gint64* gint64dup(const gint64*);

void remove_char(gchar*, gchar);
gchar* str_scanlist_compress(const gchar*);

GtkListStore* create_liststore_from_tree(GTree*);
void fill_tree_from_liststore(GTree*, GtkListStore*);

gint64 str_addr_to_gint64(const gchar*, gint);

#endif
