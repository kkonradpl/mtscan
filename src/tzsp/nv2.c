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

/* Based on reverse engineering, may be not fully accurate */
 
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#ifdef _WIN32
#include <Winsock2.h>
#else
#include <arpa/inet.h>
#endif
#include "nv2.h"
#include "ie-mikrotik-utils.h"
#include "utils.h"

#define MAC80211_HEADER_LEN      24
#define MAC80211_ADDR_DST         4
#define MAC80211_ADDR_SRC        10
#define MAC80211_ADDR_BSSID      16

#define NV2_MGMT_HEADER_LEN  8
#define NV2_MGMT_TAG_LEN     4

#define NV2_MGMT_TAG_TDMA   0x0000
#define NV2_MGMT_TAG_BEACON 0x0005

#define NV2_BEACON_TAG_LEN 2

#define NV2_BEACON_TAG_SSID        0x00
#define NV2_BEACON_TAG_RADIONAME   0x01
#define NV2_BEACON_TAG_INFO        0x02
#define NV2_BEACON_TAG_VERSION     0x03
#define NV2_BEACON_TAG__0x05       0x05 /* Len=0x01 Val=0x01 */
#define NV2_BEACON_TAG_HT_EXTENDED 0x07
#define NV2_BEACON_TAG__0x08       0x08 /* Len=0x01 Val=0x01 */
#define NV2_BEACON_TAG_80211AC     0x0A

#define NV2_BEACON_TAG_INFO_LEN    10
#define NV2_BEACON_TAG_VERSION_LEN 4
#define NV2_BEACON_TAG_HT_EXT_LEN  2
#define NV2_BEACON_TAG_80211AC_LEN 3

/* NV2_BEACON_TAG_INFO, LEN=0x0A (10)
 * DATA: CC CC II II 0x08 LL MM NN OO PP
 * C - channel [MHz]
 * I - info flags and fields
 * 0x08 - unknown
 * L - OFDM 80211AG rates
 * M - MCS0-7 rates control (ch0)
 * N - MCS0-7 rates control (ch1)
 * O - MCS8-15 rates extended (ch0)
 * P - MCS8-15 rates extended (ch1) */

#define NV2_BEACON_INFO_FREQUENCY_H       0
#define NV2_BEACON_INFO_FREQUENCY_L       1
#define NV2_BEACON_INFO_FLAGS_1           2
#define NV2_BEACON_INFO_FLAGS_2           3
#define NV2_BEACON_INFO_UNKNOWN           4
#define NV2_BEACON_INFO_RATES_OFDM        5
#define NV2_BEACON_INFO_RATES_HT_CH0_CTRL 6
#define NV2_BEACON_INFO_RATES_HT_CH1_CTRL 7
#define NV2_BEACON_INFO_RATES_HT_CH0_EXT  8
#define NV2_BEACON_INFO_RATES_HT_CH1_EXT  9

#define NV2_BEACON_INFO_FLAGS_1__7            7 /* -----------------always 0? */
#define NV2_BEACON_INFO_FLAGS_1_SGI           6 /* 0=long only, 1=supported   */
#define NV2_BEACON_INFO_FLAGS_1_BRIDGE        5 /* 0=false, 1=true            */
#define NV2_BEACON_INFO_FLAGS_1_PRIVACY       4 /* 0=none, 1=aes128-ccm       */
#define NV2_BEACON_INFO_FLAGS_1_WDS           3 /* 0=false, 1=true            */
#define NV2_BEACON_INFO_FLAGS_1__2            2 /* -----------------always 0? */
#define NV2_BEACON_INFO_FLAGS_1_FRAMEPRIO     1 /* 0=default, 1=framepriority */
#define NV2_BEACON_INFO_FLAGS_1_QUEUECOUNT_H  0

#define NV2_BEACON_INFO_FLAGS_2_QUEUECOUNT_M  7
#define NV2_BEACON_INFO_FLAGS_2_QUEUECOUNT_L  6 /* 0x1 -> 2, … 0x7 -> 8       */
#define NV2_BEACON_INFO_FLAGS_2_CHAINS_H      5
#define NV2_BEACON_INFO_FLAGS_2_CHAINS_L      4 /* 0=one, 1=two, 2=three, …?  */
#define NV2_BEACON_INFO_FLAGS_2_EXT_CHAN      3 /* 0=false, 1=true            */
#define NV2_BEACON_INFO_FLAGS_2_EXT_CHAN_LOW  2 /* 0=false, 1=true            */
#define NV2_BEACON_INFO_FLAGS_2_EXT_CHAN_UPP  1 /* 0=false, 1=true            */
#define NV2_BEACON_INFO_FLAGS_2_80211N        0 /* 0=false, 1=true            */

#define NV2_BEACON_INFO_RATES_OFDM_54M     (1 << 7)
#define NV2_BEACON_INFO_RATES_OFDM_48M     (1 << 6)
#define NV2_BEACON_INFO_RATES_OFDM_36M     (1 << 5)
#define NV2_BEACON_INFO_RATES_OFDM_24M     (1 << 4)
#define NV2_BEACON_INFO_RATES_OFDM_18M     (1 << 3)
#define NV2_BEACON_INFO_RATES_OFDM_12M     (1 << 2)
#define NV2_BEACON_INFO_RATES_OFDM_9M      (1 << 1)
#define NV2_BEACON_INFO_RATES_OFDM_6M      (1 << 0)

#define NV2_BEACON_INFO_HT_RATES_CH0_MCS7  (1 << 7)
#define NV2_BEACON_INFO_HT_RATES_CH0_MCS6  (1 << 6)
#define NV2_BEACON_INFO_HT_RATES_CH0_MCS5  (1 << 5)
#define NV2_BEACON_INFO_HT_RATES_CH0_MCS4  (1 << 4)
#define NV2_BEACON_INFO_HT_RATES_CH0_MCS3  (1 << 3)
#define NV2_BEACON_INFO_HT_RATES_CH0_MCS2  (1 << 2)
#define NV2_BEACON_INFO_HT_RATES_CH0_MCS1  (1 << 1)
#define NV2_BEACON_INFO_HT_RATES_CH0_MCS0  (1 << 0)

#define NV2_BEACON_INFO_HT_RATES_CH1_MCS15 (1 << 7)
#define NV2_BEACON_INFO_HT_RATES_CH1_MCS14 (1 << 6)
#define NV2_BEACON_INFO_HT_RATES_CH1_MCS13 (1 << 5)
#define NV2_BEACON_INFO_HT_RATES_CH1_MCS12 (1 << 4)
#define NV2_BEACON_INFO_HT_RATES_CH1_MCS11 (1 << 3)
#define NV2_BEACON_INFO_HT_RATES_CH1_MCS10 (1 << 2)
#define NV2_BEACON_INFO_HT_RATES_CH1_MCS9  (1 << 1)
#define NV2_BEACON_INFO_HT_RATES_CH1_MCS8  (1 << 0)

#define NV2_BEACON_INFO_HT_RATES_CH2_MCS23 (1 << 7)
#define NV2_BEACON_INFO_HT_RATES_CH2_MCS22 (1 << 6)
#define NV2_BEACON_INFO_HT_RATES_CH2_MCS21 (1 << 5)
#define NV2_BEACON_INFO_HT_RATES_CH2_MCS20 (1 << 4)
#define NV2_BEACON_INFO_HT_RATES_CH2_MCS19 (1 << 3)
#define NV2_BEACON_INFO_HT_RATES_CH2_MCS18 (1 << 2)
#define NV2_BEACON_INFO_HT_RATES_CH2_MCS17 (1 << 1)
#define NV2_BEACON_INFO_HT_RATES_CH2_MCS16 (1 << 0)

#define NV2_BEACON_VERSION_MAJOR   0
#define NV2_BEACON_VERSION_MINOR   1
#define NV2_BEACON_VERSION_TYPE    2
#define NV2_BEACON_VERSION_REV     3

#define NV2_BEACON_AC_EXT_CHAN       0
#define NV2_BEACON_AC_UNKNOWN        1
#define NV2_BEACON_AC_CHAIN_RATE     2

#define NV2_BEACON_AC_EXT_CHAN_NONE 0x00
#define NV2_BEACON_AC_EXT_CHAN_Ce   0x01
#define NV2_BEACON_AC_EXT_CHAN_eC   0x05
#define NV2_BEACON_AC_EXT_CHAN_Ceee 0x22
#define NV2_BEACON_AC_EXT_CHAN_eCee 0x26
#define NV2_BEACON_AC_EXT_CHAN_eeCe 0x2A
#define NV2_BEACON_AC_EXT_CHAN_eeeC 0x2E

typedef struct nv2_net
{
    uint8_t flags1;
    uint8_t flags2;
    uint8_t rates_ofdm;
    bool vht;
    uint8_t vht_chan;
    uint16_t frequency;
    char *ssid;
    char *radioname;
    char *version;
} nv2_net_t;

static nv2_net_t* nv2_process_tag(uint16_t, uint16_t, const uint8_t*);
static void nv2_process_beacon_tag(nv2_net_t*, uint8_t, uint8_t, const uint8_t*);

nv2_net_t*
nv2_network(const uint8_t *data,
            uint32_t       len,
            const uint8_t **src)
{
    static uint8_t broadcast[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    nv2_net_t *context;
    uint16_t data_type;
    uint16_t data_len;
    int i;
    
    if(len <= MAC80211_HEADER_LEN + NV2_MGMT_HEADER_LEN + NV2_MGMT_TAG_LEN)
        return NULL;

    /* 802.11 Data frame */
    if(data[0] != 0x08)
        return NULL;
    
    /* Flags */
    if(data[1] != 0x90)
        return NULL;

    /* Destination address */
    if(memcmp(data + MAC80211_ADDR_DST, broadcast, sizeof(broadcast)) != 0)
        return NULL;

    *src = data+MAC80211_ADDR_SRC;

    data += MAC80211_HEADER_LEN;
    len -= MAC80211_HEADER_LEN;

    for(i=NV2_MGMT_HEADER_LEN;
        i+NV2_MGMT_TAG_LEN <= len;
        i+=NV2_MGMT_TAG_LEN + data_len)
    {
        data_type = (data[i] << 8) | data[i+1];
        data_len = (data[i+2] << 8) | data[i+3];
        if(data_len)
        {
            if ((i + NV2_MGMT_TAG_LEN + data_len) > len)
                break;
            context = nv2_process_tag(data_type, data_len, data + i + NV2_MGMT_TAG_LEN);
            if(context)
                return context;
        }
    }
    return NULL;
}

static nv2_net_t*
nv2_process_tag(uint16_t       type,
                uint16_t       len,
                const uint8_t *data)
{
    nv2_net_t *context;
    uint8_t data_type;
    uint8_t data_len;
    int i;

    if(type != NV2_MGMT_TAG_BEACON)
        return NULL;

    context = calloc(sizeof(nv2_net_t), 1);

    for(i=0;
        i+NV2_BEACON_TAG_LEN <= len;
        i+=NV2_BEACON_TAG_LEN + data_len)
    {
        data_type = data[i];
        data_len = data[i+1];

        if(data_len)
        {
            if((i + NV2_BEACON_TAG_LEN + data_len) > len)
                break;
            nv2_process_beacon_tag(context, data_type, data_len, data+i+NV2_BEACON_TAG_LEN);
        }
    }

    return context;
}

static void
nv2_process_beacon_tag(nv2_net_t     *context,
                       uint8_t        type,
                       uint8_t        len,
                       const uint8_t *value)
{
    if(type == NV2_BEACON_TAG_SSID &&
       !context->ssid &&
       len)
    {
        context->ssid = tzsp_utils_string(value, len);
    }
    else if(type == NV2_BEACON_TAG_RADIONAME &&
            !context->radioname &&
            len)
    {
        context->radioname = tzsp_utils_string(value, len);
    }
    else if(type == NV2_BEACON_TAG_INFO &&
            len == NV2_BEACON_TAG_INFO_LEN)
    {
        /* Info (channel, rates, flags, etc) */
        context->frequency = (value[NV2_BEACON_INFO_FREQUENCY_H] << 8) |
                              value[NV2_BEACON_INFO_FREQUENCY_L];

        context->flags1 = value[NV2_BEACON_INFO_FLAGS_1];
        context->flags2 = value[NV2_BEACON_INFO_FLAGS_2];

        context->rates_ofdm = value[NV2_BEACON_INFO_RATES_OFDM];
    }
    else if(type == NV2_BEACON_TAG_VERSION &&
            len == NV2_BEACON_TAG_VERSION_LEN &&
            !context->version)
    {
        /* RouterOS version */
        context->version = ie_mikrotik_version(value[NV2_BEACON_VERSION_MAJOR],
                                               value[NV2_BEACON_VERSION_MINOR],
                                               value[NV2_BEACON_VERSION_TYPE],
                                               value[NV2_BEACON_VERSION_REV]);
    }
    else if(type == NV2_BEACON_TAG_HT_EXTENDED &&
            len == NV2_BEACON_TAG_HT_EXT_LEN)
    {
        /* >=MCS16 rates */

    }
    else if(type == NV2_BEACON_TAG_80211AC &&
            len == NV2_BEACON_TAG_80211AC_LEN)
    {
        /* 802.11ac */
        context->vht = true;
        context->vht_chan = value[NV2_BEACON_AC_EXT_CHAN];
    }
}

bool
nv2_net_is_ofdm(nv2_net_t *context)
{
    return (context->rates_ofdm != 0);
}

bool
nv2_net_is_ht(nv2_net_t *context)
{
    return (context->flags2 & (1 << NV2_BEACON_INFO_FLAGS_2_80211N)) != 0;
}

bool
nv2_net_is_vht(nv2_net_t *context)
{
    return context->vht;
}

bool
nv2_net_is_wds(nv2_net_t *context)
{
    return (context->flags1 & (1 << NV2_BEACON_INFO_FLAGS_1_WDS)) != 0;
}

bool
nv2_net_is_bridge(nv2_net_t *context)
{
    return (context->flags1 & (1 << NV2_BEACON_INFO_FLAGS_1_BRIDGE)) != 0;
}

bool
nv2_net_is_sgi(nv2_net_t *context)
{
    return (context->flags1 & (1 << NV2_BEACON_INFO_FLAGS_1_SGI)) != 0;
}

bool
nv2_net_is_privacy(nv2_net_t *context)
{
    return (context->flags1 & (1 << NV2_BEACON_INFO_FLAGS_1_PRIVACY)) != 0;
}

bool
nv2_net_is_frameprio(nv2_net_t *context)
{
    return (context->flags1 & (1 << NV2_BEACON_INFO_FLAGS_1_FRAMEPRIO)) != 0;
}

uint8_t
nv2_net_get_chains(nv2_net_t *context)
{
    int chains = 0;

    /* TODO 802.11 AC */
    if(context->flags2 & (1 << NV2_BEACON_INFO_FLAGS_2_80211N))
    {
        chains = ((context->flags2 & (1 << NV2_BEACON_INFO_FLAGS_2_CHAINS_H)) >> NV2_BEACON_INFO_FLAGS_2_CHAINS_H) << 1;
        chains |= (context->flags2 & (1 << NV2_BEACON_INFO_FLAGS_2_CHAINS_L)) >> NV2_BEACON_INFO_FLAGS_2_CHAINS_L;
        chains++;
    }

    return (uint8_t)chains;
}

uint8_t
nv2_net_get_queue_count(nv2_net_t *context)
{
    int queue_count;

    queue_count  = ((context->flags1 & (1 << NV2_BEACON_INFO_FLAGS_1_QUEUECOUNT_H)) >> NV2_BEACON_INFO_FLAGS_1_QUEUECOUNT_H) << 2;
    queue_count |= ((context->flags2 & (1 << NV2_BEACON_INFO_FLAGS_2_QUEUECOUNT_M)) >> NV2_BEACON_INFO_FLAGS_2_QUEUECOUNT_M) << 1;
    queue_count |= (context->flags2 & (1 << NV2_BEACON_INFO_FLAGS_2_QUEUECOUNT_L)) >> NV2_BEACON_INFO_FLAGS_2_QUEUECOUNT_L;

    return (uint8_t)++queue_count;
}

uint16_t
nv2_net_get_frequency(nv2_net_t *context)
{
    return context->frequency;
}

const char*
nv2_net_get_ssid(nv2_net_t *context)
{
    return context->ssid;
}

const char*
nv2_net_get_radioname(nv2_net_t *context)
{
    return context->radioname;
}

const char*
nv2_net_get_version(nv2_net_t *context)
{
    return context->version;
}

const char*
nv2_net_get_ext_channel(nv2_net_t *context)
{
    static const char channel_Ce[]      = "Ce";
    static const char channel_eC[]      = "eC";
    static const char channel_Ceee[]    = "Ceee";
    static const char channel_eCee[]    = "eCee";
    static const char channel_eeCe[]    = "eeCe";
    static const char channel_eeeC[]    = "eeeC";
    static const char channel_unknown[] = "?";

    if(context->vht)
    {
        switch(context->vht_chan)
        {
            case NV2_BEACON_AC_EXT_CHAN_NONE:
                return NULL;
            case NV2_BEACON_AC_EXT_CHAN_Ce:
                return channel_Ce;
            case NV2_BEACON_AC_EXT_CHAN_eC:
                return channel_eC;
            case NV2_BEACON_AC_EXT_CHAN_Ceee:
                return channel_Ceee;
            case NV2_BEACON_AC_EXT_CHAN_eCee:
                return channel_eCee;
            case NV2_BEACON_AC_EXT_CHAN_eeCe:
                return channel_eeCe;
            case NV2_BEACON_AC_EXT_CHAN_eeeC:
                return channel_eeeC;
            default:
                return channel_unknown;
        }
    }
    else
    {
        if (context->flags2 & (1 << NV2_BEACON_INFO_FLAGS_2_EXT_CHAN))
        {
            if(context->flags2 & (1 << NV2_BEACON_INFO_FLAGS_2_EXT_CHAN_LOW))
                return channel_eC;
            if(context->flags2 & (1 << NV2_BEACON_INFO_FLAGS_2_EXT_CHAN_UPP))
                return channel_Ce;
        }
    }
    return NULL;
}

void
nv2_net_free(nv2_net_t *context)
{
    if(context)
    {
        free(context->version);
        free(context->radioname);
        free(context->ssid);
        free(context);
    }
}
