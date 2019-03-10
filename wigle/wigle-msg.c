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
#include <glib.h>
#include "wigle.h"

typedef struct wigle_msg
{
    const wigle_t *src;
    wigle_data_t  *data;
} wigle_msg_t;


wigle_msg_t*
wigle_msg_new(const wigle_t *src,
              wigle_data_t  *data)
{
    wigle_msg_t *msg;
    msg = g_malloc(sizeof(wigle_msg_t));

    msg->src = src;
    msg->data = data;
    return msg;
}

void
wigle_msg_free(wigle_msg_t *msg)
{
    wigle_data_free(msg->data);
    g_free(msg);
}

const wigle_t*
wigle_msg_get_src(const wigle_msg_t *msg)
{
    return msg->src;
}

const wigle_data_t*
wigle_msg_get_data(const wigle_msg_t *msg)
{
    return msg->data;
}
