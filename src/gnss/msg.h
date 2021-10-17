/*
 *  MTscan - MikroTik RouterOS wireless scanner
 *  Copyright (c) 2015-2021  Konrad Kosmatka
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

#ifndef MTSCAN_GNSS_MSG_H_
#define MTSCAN_GNSS_MSG_H_

typedef struct gnss_msg gnss_msg_t;

typedef enum gnss_msg_type
{
    GNSS_MSG_INFO,
    GNSS_MSG_DATA
} gnss_msg_type_t;

typedef enum gnss_msg_info
{
    GNSS_INFO_OPENING,
    GNSS_INFO_RESOLVING,
    GNSS_INFO_CONNECTING,
    GNSS_INFO_READY,
    GNSS_INFO_CLOSED,
    GNSS_INFO_ERR_RESOLVING,
    GNSS_INFO_ERR_CONNECTING,
    GNSS_INFO_ERR_UNAVAILABLE,
    GNSS_INFO_ERR_ACCESS,
    GNSS_INFO_ERR_TIMEOUT,
    GNSS_INFO_ERR_MISMATCH
} gnss_msg_info_t;

gnss_msg_t* gnss_msg_new(gconstpointer, gnss_msg_type_t, gpointer);
void        gnss_msg_free(gnss_msg_t*);

gconstpointer   gnss_msg_get_src(const gnss_msg_t*);
gnss_msg_type_t gnss_msg_get_type(const gnss_msg_t*);
gconstpointer   gnss_msg_get_data(const gnss_msg_t*);

#endif
