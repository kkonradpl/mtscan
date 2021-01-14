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

#ifndef MTSCAN_WIGLE_MSG_H_
#define MTSCAN_WIGLE_MSG_H_

typedef struct wigle_msg wigle_msg_t;

wigle_msg_t*        wigle_msg_new(const wigle_t*, wigle_data_t*);
void                wigle_msg_free(wigle_msg_t*);
const wigle_t*      wigle_msg_get_src(const wigle_msg_t*);
const wigle_data_t* wigle_msg_get_data(const wigle_msg_t*);

#endif
