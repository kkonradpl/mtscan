/*
 *  MTscan - MikroTik RouterOS wireless scanner
 *  Copyright (c) 2015-2020  Konrad Kosmatka
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

#ifndef MTSCAN_TZSP_CAMBIUM_H
#define MTSCAN_TZSP_CAMBIUM_H

typedef struct cambium_net cambium_net_t;

cambium_net_t* cambium_network(const uint8_t*, uint32_t, const uint8_t**);

uint16_t cambium_net_get_frequency(cambium_net_t*);
const char* cambium_net_get_ssid(cambium_net_t*);

void cambium_net_free(cambium_net_t*);

#endif
