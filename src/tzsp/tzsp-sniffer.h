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

#ifndef MTSCAN_TZSP_SNIFFER_H
#define MTSCAN_TZSP_SNIFFER_H

#include <stdint.h>

typedef struct tzsp_sniffer tzsp_sniffer_t;

enum
{
    TZSP_SNIFFER_OK                   =  0,
    TZSP_SNIFFER_ERROR_INVALID_IP     = -1,
    TZSP_SNIFFER_ERROR_MEMORY         = -2,
    TZSP_SNIFFER_ERROR_CAPTURE        = -3,
    TZSP_SNIFFER_ERROR_DATALINK       = -4,
    TZSP_SNIFFER_ERROR_FILTER         = -5,
    TZSP_SNIFFER_ERROR_SET_FILTER     = -6,
    TZSP_SNIFFER_ERROR_DUMP_OPEN_DEAD = -7,
    TZSP_SNIFFER_ERROR_DUMP_OPEN      = -8
};

tzsp_sniffer_t* tzsp_sniffer_new();
int tzsp_sniffer_init(tzsp_sniffer_t*, uint16_t, const char*, const char*, const char*, int);
const char* tzsp_sniffer_get_error(const tzsp_sniffer_t*);
void tzsp_sniffer_set_func(tzsp_sniffer_t*, void (*)(const uint8_t*, uint32_t, const int8_t*, const uint8_t*, const uint8_t*, void*), void*);
void tzsp_sniffer_loop(tzsp_sniffer_t*);
void tzsp_sniffer_enable(tzsp_sniffer_t*);
void tzsp_sniffer_disable(tzsp_sniffer_t*);
void tzsp_sniffer_cancel(tzsp_sniffer_t*);
void tzsp_sniffer_free(tzsp_sniffer_t*);

#endif
