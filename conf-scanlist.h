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

#ifndef MTSCAN_CONF_SCANLIST_H_
#define MTSCAN_CONF_SCANLIST_H_

typedef struct conf_scanlist conf_scanlist_t;

enum
{
    CONF_SCANLIST_COL_NAME,
    CONF_SCANLIST_COL_DATA,
    CONF_SCANLIST_COL_MAIN,
    CONF_SCANLIST_COLS
};

conf_scanlist_t* conf_scanlist_new(gchar*, gchar*, gboolean);
void conf_scanlist_free(conf_scanlist_t*);

const gchar* conf_scanlist_get_name(const conf_scanlist_t*);
const gchar* conf_scanlist_get_data(const conf_scanlist_t*);
gboolean conf_scanlist_get_main(const conf_scanlist_t*);

GtkListStore* conf_scanlist_list_new(void);
GtkTreeIter conf_scanlist_list_add(GtkListStore*, const conf_scanlist_t*);
conf_scanlist_t* conf_scanlist_list_get(GtkListStore*, GtkTreeIter*);

#endif

