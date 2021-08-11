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

#ifndef MTSCAN_TZSP_MAC80211_H
#define MTSCAN_TZSP_MAC80211_H
#include "ie-airmax-ac.h"
#include "ie-mikrotik.h"
#include "nv2.h"
#include "ie-airmax.h"

enum
{
    MAC80211_FRAME_INVALID = -1,
    MAC80211_FRAME_UNKNOWN = 0,
    MAC80211_FRAME_BEACON = 1,
    MAC80211_FRAME_PROBE_RESPONSE = 2,
};

typedef struct mac80211_net
{
    int source;
    char *ssid;
    char *radioname;
    int channel;
    uint16_t caps;
    uint8_t dsss_rates;
    uint8_t ofdm_rates;
    bool ht;
    uint8_t ht_chan;
    uint8_t ht_mode;
    uint8_t ht_chains;
    bool vht;
    uint8_t vht_mode;
    uint8_t vht_chan0;
    uint8_t vht_chan1;
    uint8_t vht_chains;
    ie_mikrotik_t *ie_mikrotik;
    ie_airmax_t *ie_airmax;
    ie_airmax_ac_t *ie_airmax_ac;
} mac80211_net_t;

mac80211_net_t* mac80211_network(const uint8_t *, uint32_t, const uint8_t **);

bool mac80211_net_is_privacy(mac80211_net_t *);
bool mac80211_net_is_dsss(mac80211_net_t *);
bool mac80211_net_is_ofdm(mac80211_net_t *);
bool mac80211_net_is_ht(mac80211_net_t *);
bool mac80211_net_is_vht(mac80211_net_t *);

uint8_t mac80211_net_get_chains(mac80211_net_t *);
const char* mac80211_net_get_ext_channel(mac80211_net_t *);

void mac80211_net_free(mac80211_net_t *);

#endif
