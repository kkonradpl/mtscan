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

#ifndef MTSCAN_TZSP_SOCKET_H
#define MTSCAN_TZSP_SOCKET_H

#include <stdint.h>

typedef struct tzsp_socket tzsp_socket_t;

enum
{
    TZSP_SOCKET_OK                   =  0,
    TZSP_SOCKET_ERROR_INVALID_IP     = -1,
    TZSP_SOCKET_ERROR_SOCKET         = -2,
    TZSP_SOCKET_ERROR_BIND           = -3,
    TZSP_SOCKET_ERROR_DUMP_OPEN_DEAD = -4,
    TZSP_SOCKET_ERROR_DUMP_OPEN      = -5
};

tzsp_socket_t* tzsp_socket_new();
int tzsp_socket_init(tzsp_socket_t*, uint16_t, const char*, const char*);
void tzsp_socket_set_func(tzsp_socket_t*, void (*)(const uint8_t*, uint32_t, const int8_t*, const uint8_t*, const uint8_t*, void*), void*);
void tzsp_socket_loop(tzsp_socket_t*);
void tzsp_socket_enable(tzsp_socket_t*);
void tzsp_socket_disable(tzsp_socket_t*);
void tzsp_socket_cancel(tzsp_socket_t*);
void tzsp_socket_free(tzsp_socket_t*);

#endif
