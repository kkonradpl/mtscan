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

#ifndef MTSCAN_DECAP_H
#define MTSCAN_DECAP_H
#include <stdint.h>

const uint8_t* decap_ethernet(const uint8_t*, uint32_t*);
const uint8_t* decap_ip(const uint8_t*, uint32_t*);
const uint8_t* decap_udp(const uint8_t*, uint32_t*);
const uint8_t* decap_tzsp(const uint8_t*, uint32_t*, const int8_t**, const uint8_t**, const uint8_t**);

#endif
