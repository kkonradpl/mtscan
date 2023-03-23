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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef _WIN32
#include <Winsock2.h>
#else
#include <arpa/inet.h>
#endif
#include "mac80211.h"
#include "utils.h"

#define MAC80211_HEADER_LEN      24
#define MAC80211_ADDR_DST         4
#define MAC80211_ADDR_SRC        10
#define MAC80211_ADDR_BSSID      16

#define MAC80211_MGMT_HEADER_LEN 12
#define MAC80211_MGMT_TAG_LEN     2

#define MAC80211_MGMT_HEADER_CAPS_LO 10
#define MAC80211_MGMT_HEADER_CAPS_HI 11

#define MAC80211_MGMT_TAG_SSID      0x00
#define MAC80211_MGMT_TAG_RATES     0x01
#define MAC80211_MGMT_TAG_CHANNEL   0x03
#define MAC80211_MGMT_TAG_HT_CAPS   0x2D
#define MAC80211_MGMT_TAG_RATES_EXT 0x32
#define MAC80211_MGMT_TAG_HT_INFO   0x3D
#define MAC80211_MGMT_TAG_CISCO     0x85
#define MAC80211_MGMT_TAG_VHT_CAPS  0xBF
#define MAC80211_MGMT_TAG_VHT_INFO  0xC0
#define MAC80211_MGMT_TAG_VENDOR_IE 0xDD
#define MAC80211_MGMT_TAG_EXT       0xFF

#define MAC80211_MGMT_TAG_EXT_HE_CAPS 0x23

#define MAC80211_MGMT_TAG_CHANNEL_LEN    1
#define MAC80211_MGMT_TAG_HT_CAPS_LEN   26
#define MAC80211_MGMT_TAG_HT_INFO_LEN   22
#define MAC80211_MGMT_TAG_CISCO_MIN_LEN 26
#define MAC80211_MGMT_TAG_VHT_CAPS_LEN  12
#define MAC80211_MGMT_TAG_VHT_INFO_LEN   5

#define MAC80211_CAPS_ESS     (1 << 0)
#define MAC80211_CAPS_IBSS    (1 << 1)
#define MAC80211_CAPS_PRIVACY (1 << 4)

#define MAC80211_BASIC_RATE   (1 << 7)

#define MAC80211_DSSS_RATE_1M     2
#define MAC80211_DSSS_RATE_2M     4
#define MAC80211_DSSS_RATE_5_5M  11
#define MAC80211_DSSS_RATE_11M   22
#define MAC80211_OFDM_RATE_6M    12
#define MAC80211_OFDM_RATE_9M    18
#define MAC80211_OFDM_RATE_12M   24
#define MAC80211_OFDM_RATE_18M   36
#define MAC80211_OFDM_RATE_24M   48
#define MAC80211_OFDM_RATE_36M   72
#define MAC80211_OFDM_RATE_48M   96
#define MAC80211_OFDM_RATE_54M  108

#define INTERNAL_DSSS_RATE_1M   (1 << 0)
#define INTERNAL_DSSS_RATE_2M   (1 << 1)
#define INTERNAL_DSSS_RATE_5_5M (1 << 2)
#define INTERNAL_DSSS_RATE_11M  (1 << 3)
#define INTERNAL_OFDM_RATE_6M   (1 << 0)
#define INTERNAL_OFDM_RATE_9M   (1 << 1)
#define INTERNAL_OFDM_RATE_12M  (1 << 2)
#define INTERNAL_OFDM_RATE_18M  (1 << 3)
#define INTERNAL_OFDM_RATE_24M  (1 << 4)
#define INTERNAL_OFDM_RATE_36M  (1 << 5)
#define INTERNAL_OFDM_RATE_48M  (1 << 6)
#define INTERNAL_OFDM_RATE_54M  (1 << 7)

#define MAC80211_HT_INFO_CHANNEL  0
#define MAC80211_HT_INFO_SUBSET_1 1

#define VHT_CHANNEL_MODE_HT   0
#define VHT_CHANNEL_MODE_80   1
#define VHT_CHANNEL_MODE_160  2
#define VHT_CHANNEL_MODE_2x80 3

static int mac80211_frame(const uint8_t*, uint32_t);
static void mac80211_process(mac80211_net_t*, const uint8_t*, uint32_t);
static void mac80211_process_tag(mac80211_net_t*, uint8_t, uint8_t, const uint8_t*, const uint8_t*);

mac80211_net_t*
mac80211_network(const uint8_t  *data,
                 uint32_t        len,
                 const uint8_t **src)
{
    mac80211_net_t *net;
    int frame;

    frame = mac80211_frame(data, len);
    if(frame == MAC80211_FRAME_INVALID)
        return NULL;

    *src = data+MAC80211_ADDR_SRC;

    if(frame == MAC80211_FRAME_UNKNOWN)
        return NULL;

    net = calloc(sizeof(mac80211_net_t), 1);
    net->source = frame;
    net->channel = -1;

    mac80211_process(net, data, len);
    return net;
}

static int
mac80211_frame(const uint8_t *data,
               uint32_t       len)
{
    static const uint8_t broadcast[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    int frame = MAC80211_FRAME_UNKNOWN;

    if(len < MAC80211_HEADER_LEN)
        return MAC80211_FRAME_INVALID;

    if(data[0] == 0x80)
    {
        /* 802.11 beacon frame */
        if(memcmp(data + MAC80211_ADDR_DST, broadcast, sizeof(broadcast)) != 0)
            return MAC80211_FRAME_UNKNOWN;
        frame = MAC80211_FRAME_BEACON;
    }
    else if(data[0] == 0x50)
    {
        /* 802.11 probe response frame */
        frame = MAC80211_FRAME_PROBE_RESPONSE;
    }
    return frame;
}

static void
mac80211_process(mac80211_net_t *context,
                 const uint8_t  *data,
                 uint32_t        len)
{
    const uint8_t *src;
    uint8_t data_type;
    uint8_t data_len;
    int i;

    src = data + MAC80211_ADDR_SRC;
    data = data + MAC80211_HEADER_LEN;
    len = len - MAC80211_HEADER_LEN;

    context->caps = (data[MAC80211_MGMT_HEADER_CAPS_HI] << 8) |
                     data[MAC80211_MGMT_HEADER_CAPS_LO];

    for(i=MAC80211_MGMT_HEADER_LEN;
        i+MAC80211_MGMT_TAG_LEN <= len;
        i+=MAC80211_MGMT_TAG_LEN + data_len)
    {
        data_type = data[i];
        data_len = data[i+1];
        if(data_len)
        {
            if ((i + MAC80211_MGMT_TAG_LEN + data_len) > len)
                break;
            mac80211_process_tag(context,
                                 data_type,
                                 data_len,
                                 data+i+MAC80211_MGMT_TAG_LEN,
                                 src);
        }
    }
}

static void
mac80211_process_tag(mac80211_net_t *context,
                     uint8_t         type,
                     uint8_t         len,
                     const uint8_t  *data,
                     const uint8_t  *bssid)
{
    static const uint8_t oui_epigram[] = { 0x00, 0x90, 0x4c };

    if(type == MAC80211_MGMT_TAG_SSID &&
       !context->ssid &&
       len &&
       data[0])
    {

        context->ssid = tzsp_utils_string(data, len);
    }
    else if((type == MAC80211_MGMT_TAG_RATES ||
             type == MAC80211_MGMT_TAG_RATES_EXT) &&
             len)
    {
        int i;
        for(i=0;i<len;i++)
        {
            int rate = data[i] & ~MAC80211_BASIC_RATE;
            if(rate == MAC80211_DSSS_RATE_1M)
                context->dsss_rates |= INTERNAL_DSSS_RATE_1M;
            else if(rate == MAC80211_DSSS_RATE_2M)
                context->dsss_rates |= INTERNAL_DSSS_RATE_2M;
            else if(rate == MAC80211_DSSS_RATE_5_5M)
                context->dsss_rates |= INTERNAL_DSSS_RATE_5_5M;
            else if(rate == MAC80211_DSSS_RATE_11M)
                context->dsss_rates |= INTERNAL_DSSS_RATE_11M;
            else if(rate == MAC80211_OFDM_RATE_6M)
                context->ofdm_rates |= INTERNAL_OFDM_RATE_6M;
            else if(rate == MAC80211_OFDM_RATE_9M)
                context->ofdm_rates |= INTERNAL_OFDM_RATE_9M;
            else if(rate == MAC80211_OFDM_RATE_12M)
                context->ofdm_rates |= INTERNAL_OFDM_RATE_12M;
            else if(rate == MAC80211_OFDM_RATE_18M)
                context->ofdm_rates |= INTERNAL_OFDM_RATE_18M;
            else if(rate == MAC80211_OFDM_RATE_24M)
                context->ofdm_rates |= INTERNAL_OFDM_RATE_24M;
            else if(rate == MAC80211_OFDM_RATE_36M)
                context->ofdm_rates |= INTERNAL_OFDM_RATE_36M;
            else if(rate == MAC80211_OFDM_RATE_48M)
                context->ofdm_rates |= INTERNAL_OFDM_RATE_48M;
            else if(rate == MAC80211_OFDM_RATE_54M)
                context->ofdm_rates |= INTERNAL_OFDM_RATE_54M;
        }
    }
    else if(type == MAC80211_MGMT_TAG_CHANNEL &&
            len == MAC80211_MGMT_TAG_CHANNEL_LEN)
    {
        context->channel = data[0];
    }
    else if(type == MAC80211_MGMT_TAG_HT_CAPS &&
            len == MAC80211_MGMT_TAG_HT_CAPS_LEN)
    {
        if(data[6])
            context->ht_chains = 4;
        else if(data[5])
            context->ht_chains = 3;
        else if(data[4])
            context->ht_chains = 2;
        else if(data[3])
            context->ht_chains = 1;
    }
    else if(type == MAC80211_MGMT_TAG_HT_INFO &&
            len == MAC80211_MGMT_TAG_HT_INFO_LEN)
    {
        context->ht = true;
        context->ht_chan = data[MAC80211_HT_INFO_CHANNEL];
        context->ht_mode = data[MAC80211_HT_INFO_SUBSET_1];
    }
    else if(type == MAC80211_MGMT_TAG_CISCO &&
            len >= MAC80211_MGMT_TAG_CISCO_MIN_LEN)
    {
        context->radioname = tzsp_utils_string(data+10, 16);
    }
    else if(type == MAC80211_MGMT_TAG_VHT_CAPS &&
            len == MAC80211_MGMT_TAG_VHT_CAPS_LEN)
    {
        uint16_t tx_mcs_map = (data[9] << 8) | data[8];
        if((tx_mcs_map      & 0xC000) >> 14 != 0x03)
            context->vht_chains = 8;
        else if((tx_mcs_map & 0x3000) >> 12 != 0x03)
            context->vht_chains = 7;
        else if((tx_mcs_map & 0x0C00) >> 10 != 0x03)
            context->vht_chains = 6;
        else if((tx_mcs_map & 0x0300) >>  8 != 0x03)
            context->vht_chains = 5;
        else if((tx_mcs_map & 0x00C0) >>  6 != 0x03)
            context->vht_chains = 4;
        else if((tx_mcs_map & 0x0030) >>  4 != 0x03)
            context->vht_chains = 3;
        else if((tx_mcs_map & 0x000C) >>  2 != 0x03)
            context->vht_chains = 2;
        else if((tx_mcs_map & 0x0003) >>  0 != 0x03)
            context->vht_chains = 1;
    }
    else if(type == MAC80211_MGMT_TAG_VHT_INFO &&
            len == MAC80211_MGMT_TAG_VHT_INFO_LEN)
    {
        context->vht = true;
        context->vht_mode = data[0];
        context->vht_chan0 = data[1];
        context->vht_chan1 = data[2];
    }
    else if(type == MAC80211_MGMT_TAG_VENDOR_IE &&
            len)
    {
        if(len == 26 &&
           memcmp(oui_epigram, data, sizeof(oui_epigram)) == 0 &&
           data[3] == 0x34)
        {
            /* HT Additional Capabilities (802.11n draft) */
            if(!context->ht)
            {
                context->ht = true;
                context->ht_chan = data[4+MAC80211_HT_INFO_CHANNEL];
                context->ht_mode = data[4+MAC80211_HT_INFO_SUBSET_1];
            }
        }

        if(!context->ie_mikrotik)
            context->ie_mikrotik = ie_mikrotik_parse(data, len);

        if(!context->ie_airmax)
            context->ie_airmax = ie_airmax_parse(data, len);

        if(!context->ie_airmax_ac)
            context->ie_airmax_ac = ie_airmax_ac_parse(data, len, bssid);

        if(!context->ie_wps)
            context->ie_wps = ie_wps_parse(data, len);
    }
    else if(type == MAC80211_MGMT_TAG_EXT &&
            len)
    {
        if (data[0] == MAC80211_MGMT_TAG_EXT_HE_CAPS)
            context->he = true;
    }
}

bool
mac80211_net_is_privacy(mac80211_net_t *context)
{
    return (context->caps & MAC80211_CAPS_PRIVACY) != 0;
}

bool
mac80211_net_is_dsss(mac80211_net_t *context)
{
    return context->dsss_rates != 0;
}

bool
mac80211_net_is_ofdm(mac80211_net_t *context)
{
    return context->ofdm_rates != 0;
}

bool
mac80211_net_is_ht(mac80211_net_t *context)
{
    return context->ht;
}

bool
mac80211_net_is_vht(mac80211_net_t *context)
{
    return context->vht;
}

bool
mac80211_net_is_he(mac80211_net_t *context)
{
    return context->he;
}

uint8_t
mac80211_net_get_chains(mac80211_net_t *context)
{
    return (context->vht_chains > context->ht_chains ?
            context->vht_chains : context->ht_chains);
}

const char*
mac80211_net_get_ext_channel(mac80211_net_t *context)
{
    static const char channel_Ce[]      = "Ce";
    static const char channel_eC[]      = "eC";
    static const char channel_Ceee[]    = "Ceee";
    static const char channel_eCee[]    = "eCee";
    static const char channel_eeCe[]    = "eeCe";
    static const char channel_eeeC[]    = "eeeC";
    static const char channel_80_80[]   = "2x80";
    static const char channel_160[]     = "160";
    static const char channel_unknown[] = "?";
    int diff;

    if(context->vht &&
       context->ht &&
       context->vht_mode != VHT_CHANNEL_MODE_HT)
    {
        if(context->vht_mode == VHT_CHANNEL_MODE_80)
        {
            diff = (int)context->vht_chan0 - (int)context->ht_chan;
            if(diff == 6)
                return channel_Ceee;
            else if(diff == 2)
                return channel_eCee;
            else if(diff == -2)
                return channel_eeCe;
            else if(diff == -6)
                return channel_eeeC;
            return channel_unknown;
        }
        else if(context->vht_mode == VHT_CHANNEL_MODE_160)
        {
            return channel_160;
        }
        else if(context->vht_mode == VHT_CHANNEL_MODE_2x80)
        {
            return channel_80_80;
        }
        return channel_unknown;
    }
    else if(context->ht &&
            context->ht_mode & (1 << 2))
    {
        if (context->ht_mode & (1 << 0) &&
            context->ht_mode & (1 << 1))
            return channel_eC;
        if (context->ht_mode & (1 << 0))
            return channel_Ce;
        return channel_unknown;
    }

    return NULL;
}

void
mac80211_net_free(mac80211_net_t *context)
{
    free(context->ssid);
    free(context->radioname);
    ie_mikrotik_free(context->ie_mikrotik);
    ie_airmax_free(context->ie_airmax);
    ie_airmax_ac_free(context->ie_airmax_ac);
    ie_wps_free(context->ie_wps);
    free(context);
}
