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

#ifndef MTSCAN_TZSP_NV2_H
#define MTSCAN_TZSP_NV2_H

typedef struct nv2_net nv2_net_t;

nv2_net_t* nv2_network(const uint8_t*, uint32_t, const uint8_t**);

bool nv2_net_is_ofdm(nv2_net_t*);
bool nv2_net_is_ht(nv2_net_t*);
bool nv2_net_is_vht(nv2_net_t*);
bool nv2_net_is_wds(nv2_net_t*);
bool nv2_net_is_bridge(nv2_net_t*);
bool nv2_net_is_sgi(nv2_net_t*);
bool nv2_net_is_privacy(nv2_net_t*);
bool nv2_net_is_frameprio(nv2_net_t*);

uint8_t nv2_net_get_chains(nv2_net_t*);
uint8_t nv2_net_get_queue_count(nv2_net_t*);
uint16_t nv2_net_get_frequency(nv2_net_t*);

const char* nv2_net_get_ssid(nv2_net_t*);
const char* nv2_net_get_radioname(nv2_net_t*);
const char* nv2_net_get_version(nv2_net_t*);
const char* nv2_net_get_mode(nv2_net_t*);
const char* nv2_net_get_ext_channel(nv2_net_t*);

void nv2_net_free(nv2_net_t*);

#endif
