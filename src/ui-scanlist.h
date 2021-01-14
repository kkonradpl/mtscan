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

#ifndef MTSCAN_UI_SCANLIST_H_
#define MTSCAN_UI_SCANLIST_H_

typedef struct ui_scanlist ui_scanlist_t;

typedef enum ui_scanlist_page
{
    UI_SCANLIST_PAGE_2_GHZ,
    UI_SCANLIST_PAGE_5_GHZ
} ui_scanlist_page_t;

ui_scanlist_t* ui_scanlist(GtkWidget*, ui_scanlist_page_t);
void ui_scanlist_free(ui_scanlist_t*);
void ui_scanlist_show(ui_scanlist_t*);
void ui_scanlist_current(ui_scanlist_t*, const gchar*);
void ui_scanlist_add(ui_scanlist_t*, const gchar*);

#endif

