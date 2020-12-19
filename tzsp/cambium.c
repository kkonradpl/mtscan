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

/* Based on reverse engineering, early alpha version */
 
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include "cambium.h"
#include "utils.h"

#define MAC80211_HEADER_LEN      24
#define MAC80211_ADDR_DST         4
#define MAC80211_ADDR_SRC        10

#define CAMBIUM_BEACON_HEADER_LEN  8
#define CAMBIUM_BEACON_TAG_LEN     2

#define CAMBIUM_BEACON_TAG_SSID        0x01
#define CAMBIUM_BEACON_TAG_CHANNEL     0x05

#define CAMBIUM_BEACON_TAG_CHANNEL_LEN 0x0D

#define CAMBIUM_BEACON_CHANNEL_FREQUENCY_H 11
#define CAMBIUM_BEACON_CHANNEL_FREQUENCY_L 12

typedef struct cambium_net
{
    uint16_t frequency;
    char *ssid;
} cambium_net_t;

static cambium_net_t* cambium_process_beacon(const uint8_t*, uint32_t);
static void cambium_process_beacon_tag(cambium_net_t*, uint8_t, uint8_t, const uint8_t*);

cambium_net_t*
cambium_network(const uint8_t *data,
                uint32_t        len,
                const uint8_t **src)
{
    static uint8_t broadcast[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    if(len <= MAC80211_HEADER_LEN + CAMBIUM_BEACON_HEADER_LEN + CAMBIUM_BEACON_TAG_LEN)
        return NULL;

    /* 802.11 Action No Ack */
    if(data[0] != 0xe0)
        return NULL;

    /* Flags */
    if(data[1] != 0x00)
        return NULL;

    /* Destination address */
    if(memcmp(data + MAC80211_ADDR_DST, broadcast, sizeof(broadcast)) != 0)
        return NULL;

    *src = data+MAC80211_ADDR_SRC;

    data += MAC80211_HEADER_LEN;
    len -= MAC80211_HEADER_LEN;
    
    return cambium_process_beacon(data, len);
}

static cambium_net_t*
cambium_process_beacon(const uint8_t *data,
                       uint32_t       len)
{
    cambium_net_t *context;
    uint8_t data_type;
    uint8_t data_len;
    int i;

    /* First 8 bytes (header?)   */
    /* 7f 08 00 07 a1 20 04 06   */
    /* Let's check the first one */
    if(data[0] != 0x7f)
        return NULL;

    context = calloc(sizeof(cambium_net_t), 1);

    for(i=CAMBIUM_BEACON_HEADER_LEN;
        i+CAMBIUM_BEACON_TAG_LEN <= len;
        i+=CAMBIUM_BEACON_TAG_LEN + data_len)
    {
        data_type = data[i];
        data_len = data[i+1];

        if(data_len)
        {
            if((i + CAMBIUM_BEACON_TAG_LEN + data_len) > len)
                break;
            cambium_process_beacon_tag(context, data_type, data_len, data+i+CAMBIUM_BEACON_TAG_LEN);
        }
    }

    return context;
}

static void
cambium_process_beacon_tag(cambium_net_t *context,
                           uint8_t        type,
                           uint8_t        len,
                           const uint8_t *value)
{
    if(type == CAMBIUM_BEACON_TAG_SSID &&
       !context->ssid &&
       len)
    {
        context->ssid = tzsp_utils_string(value, len);
    }
    else if(type == CAMBIUM_BEACON_TAG_CHANNEL &&
            len == CAMBIUM_BEACON_TAG_CHANNEL_LEN)
    {
        context->frequency = (value[CAMBIUM_BEACON_CHANNEL_FREQUENCY_H] << 8) |
                              value[CAMBIUM_BEACON_CHANNEL_FREQUENCY_L];
    }
}

uint16_t
cambium_net_get_frequency(cambium_net_t *context)
{
    return context->frequency;
}

const char*
cambium_net_get_ssid(cambium_net_t *context)
{
    return context->ssid;
}

void
cambium_net_free(cambium_net_t *context)
{
    if(context)
    {
        free(context->ssid);
        free(context);
    }
}
