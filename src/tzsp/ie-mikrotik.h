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

#ifndef MTSCAN_TZSP_IE_MIKROTIK_H
#define MTSCAN_TZSP_IE_MIKROTIK_H
#include <stdint.h>
#include <stdbool.h>

typedef struct ie_mikrotik ie_mikrotik_t;

ie_mikrotik_t* ie_mikrotik_parse(const uint8_t*, uint8_t);

bool ie_mikrotik_is_nstreme(ie_mikrotik_t*);
bool ie_mikrotik_is_wds(ie_mikrotik_t*);
bool ie_mikrotik_is_bridge(ie_mikrotik_t*);

uint16_t ie_mikrotik_get_mru(ie_mikrotik_t*);
uint16_t ie_mikrotik_get_framer_limit(ie_mikrotik_t*);
uint16_t ie_mikrotik_get_frequency(ie_mikrotik_t*);
const char* ie_mikrotik_get_radioname(ie_mikrotik_t*);
const char* ie_mikrotik_get_version(ie_mikrotik_t*);

void ie_mikrotik_free(ie_mikrotik_t*);

#endif
