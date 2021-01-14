/*
 *  MTscan - MikroTik RouterOS wireless scanner
 *  Copyright (c) 2015-2017  Konrad Kosmatka
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
#include <stdlib.h>
#include "ie-mikrotik.h"
#include "ie-mikrotik-utils.h"
#include "utils.h"

#define IE_MIKROTIK_HEADER_LEN 6
#define IE_MIKROTIK_TAG_HEADER_LEN 2

#define IE_MIKROTIK_TAG_DATA 0x01
#define IE_MIKROTIK_TAG_DATA_LEN 30

#define IE_MIKROTIK_TAG_FREQ 0x05
#define IE_MIKROTIK_TAG_FREQ_LEN 2

#define IE_MIKROTIK_DATA_FLAGS1         0
#define IE_MIKROTIK_DATA_FLAGS2         1
#define IE_MIKROTIK_DATA_FLAGS3         2
#define IE_MIKROTIK_DATA_FLAGS4         3
#define IE_MIKROTIK_DATA_VERSION_REV    4
#define IE_MIKROTIK_DATA_VERSION_TYPE   5
#define IE_MIKROTIK_DATA_VERSION_MINOR  6
#define IE_MIKROTIK_DATA_VERSION_MAJOR  7
#define IE_MIKROTIK_DATA_MRU_L          8
#define IE_MIKROTIK_DATA_MRU_H          9
#define IE_MIKROTIK_DATA_RADIONAME      10
#define IE_MIKROTIK_DATA_FRAMER_LIMIT_L 26
#define IE_MIKROTIK_DATA_FRAMER_LIMIT_H 27
#define IE_MIKROTIK_DATA_NSTREME        28

#define IE_MIKROTIK_DATA_RADIONAME_LEN  16

#define IE_MIKROTIK_FLAGS1_NSTREME                (1 << 0)
#define IE_MIKROTIK_FLAGS1_FAST_FRAMES            (1 << 1)
#define IE_MIKROTIK_FLAGS1_DOING_WDS              (1 << 2)
#define IE_MIKROTIK_FLAGS1_WITHOUT_POLLING        (1 << 3)
#define IE_MIKROTIK_FLAGS1_DYNAMIC_PACKAGING_SIZE (1 << 4)

#define IE_MIKROTIK_FLAGS2_WITHOUT_CSMA           (1 << 0)
#define IE_MIKROTIK_FLAGS2_MGMT_PROTECTION        (1 << 2)
#define IE_MIKROTIK_FLAGS2_MGMT_PROTECTION_REQ    (1 << 3)
#define IE_MIKROTIK_FLAGS2_BRIDGE                 (1 << 4)

#define IE_MIKROTIK_FREQUENCY_L 0
#define IE_MIKROTIK_FREQUENCY_H 1

typedef struct ie_mikrotik
{
    uint8_t flags1;
    uint8_t flags2;
    uint16_t mru;
    uint16_t framer_limit;
    uint16_t frequency;
    char *radioname;
    char *version;
} ie_mikrotik_t;

static ie_mikrotik_t* ie_mikrotik_process_data(const uint8_t*, uint8_t);
static void ie_mikrotik_process_tag(ie_mikrotik_t*, uint8_t, uint8_t, const uint8_t*);

ie_mikrotik_t*
ie_mikrotik_parse(const uint8_t *ie,
                  uint8_t        ie_len)
{
    static const uint8_t magic[] = { 0x00, 0x0c, 0x42, 0x00, 0x00, 0x00 };

    if(ie_len < IE_MIKROTIK_HEADER_LEN)
        return NULL;

    /* The IE must start with a magic byte sequence */
    if(memcmp(magic, ie, sizeof(magic)) != 0)
        return NULL;

    return ie_mikrotik_process_data(ie, ie_len);
}

static ie_mikrotik_t*
ie_mikrotik_process_data(const uint8_t *ie,
                         uint8_t        ie_len)
{
    ie_mikrotik_t *context;
    uint8_t tag_len;
    int i;

    context = calloc(sizeof(ie_mikrotik_t), 1);
    if(context == NULL)
        return NULL;

    for(i=IE_MIKROTIK_HEADER_LEN;
        i+IE_MIKROTIK_TAG_HEADER_LEN <= ie_len;
        i+=IE_MIKROTIK_TAG_HEADER_LEN + tag_len)
    {
        tag_len = ie[i+1];
        if((i + IE_MIKROTIK_TAG_HEADER_LEN + tag_len) > ie_len)
            break;

        ie_mikrotik_process_tag(context, ie[i], tag_len, ie+i+IE_MIKROTIK_TAG_HEADER_LEN);
    }

    return context;
}

static void
ie_mikrotik_process_tag(ie_mikrotik_t *context,
                        uint8_t        type,
                        uint8_t        len,
                        const uint8_t *value)
{
    if(type == IE_MIKROTIK_TAG_DATA &&
       len == IE_MIKROTIK_TAG_DATA_LEN)
    {
        context->flags1 = value[IE_MIKROTIK_DATA_FLAGS1];
        context->flags2 = value[IE_MIKROTIK_DATA_FLAGS2];

        if(!context->version)
        {
            context->version = ie_mikrotik_version(value[IE_MIKROTIK_DATA_VERSION_MAJOR],
                                                   value[IE_MIKROTIK_DATA_VERSION_MINOR],
                                                   value[IE_MIKROTIK_DATA_VERSION_TYPE],
                                                   value[IE_MIKROTIK_DATA_VERSION_REV]);
        }

        context->mru = (value[IE_MIKROTIK_DATA_MRU_H] << 8) |
                        value[IE_MIKROTIK_DATA_MRU_L];

        if(!context->radioname &&
           *(value+IE_MIKROTIK_DATA_RADIONAME))
            context->radioname = tzsp_utils_string(value+IE_MIKROTIK_DATA_RADIONAME,
                                                   IE_MIKROTIK_DATA_RADIONAME_LEN);

        context->framer_limit = (value[IE_MIKROTIK_DATA_FRAMER_LIMIT_H] << 8) |
                                 value[IE_MIKROTIK_DATA_FRAMER_LIMIT_L];
    }
    else if(type == IE_MIKROTIK_TAG_FREQ &&
            len == IE_MIKROTIK_TAG_FREQ_LEN)
    {
        context->frequency = (value[IE_MIKROTIK_FREQUENCY_H] << 8) |
                              value[IE_MIKROTIK_FREQUENCY_L];
    }
}

bool
ie_mikrotik_is_nstreme(ie_mikrotik_t *context)
{
    return (context->flags1 & IE_MIKROTIK_FLAGS1_NSTREME) != 0;
}

bool
ie_mikrotik_is_wds(ie_mikrotik_t *context)
{
    return (context->flags1 & IE_MIKROTIK_FLAGS1_DOING_WDS) != 0;
}

bool ie_mikrotik_is_bridge(ie_mikrotik_t *context)
{
    return (context->flags2 & IE_MIKROTIK_FLAGS2_BRIDGE) != 0;
}

uint16_t
ie_mikrotik_get_mru(ie_mikrotik_t *context)
{
    return context->mru;
}

uint16_t
ie_mikrotik_get_framer_limit(ie_mikrotik_t *context)
{
    return context->framer_limit;
}

uint16_t
ie_mikrotik_get_frequency(ie_mikrotik_t *context)
{
    return context->frequency;
}

const char*
ie_mikrotik_get_radioname(ie_mikrotik_t *context)
{
    return context->radioname;
}

const char*
ie_mikrotik_get_version(ie_mikrotik_t *context)
{
    return context->version;
}

void
ie_mikrotik_free(ie_mikrotik_t *context)
{
    if(context)
    {
        free(context->radioname);
        free(context->version);
        free(context);
    }
}
