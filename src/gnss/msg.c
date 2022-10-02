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

#include <glib.h>
#include "msg.h"
#include "data.h"

typedef struct gnss_msg
{
    gconstpointer   src;
    gnss_msg_type_t type;
    gpointer        data;
} gnss_msg_t;


gnss_msg_t*
gnss_msg_new(gconstpointer   src,
             gnss_msg_type_t type,
             gpointer        data)
{
    gnss_msg_t *msg;
    msg = g_malloc(sizeof(gnss_msg_t));

    msg->src = src;
    msg->type = type;
    msg->data = data;
    return msg;
}

void
gnss_msg_free(gnss_msg_t *msg)
{
    if(msg)
    {
        switch(msg->type)
        {
            case GNSS_MSG_INFO:
                /* Nothing to free */
                break;

            case GNSS_MSG_DATA:
                gnss_data_free(msg->data);
                break;
        }
        g_free(msg);
    }
}

gconstpointer
gnss_msg_get_src(const gnss_msg_t *msg)
{
    return msg->src;
}
 
gnss_msg_type_t
gnss_msg_get_type(const gnss_msg_t *msg)
{
    return msg->type;
}

gconstpointer
gnss_msg_get_data(const gnss_msg_t *msg)
{
    return msg->data;
}
