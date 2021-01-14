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
#include "ie-airmax.h"

#define IE_AIRMAX_LEN 38

typedef struct ie_airmax
{
    uint8_t placeholder;
} ie_airmax_t;

ie_airmax_t*
ie_airmax_parse(const uint8_t *ie,
                uint8_t        ie_len)
{
    static const uint8_t magic[] = { 0x00, 0x15, 0x6d, 0xff, 0xff, 0xff };

    if(ie_len != IE_AIRMAX_LEN)
        return NULL;

    /* The IE must start with a magic byte sequence */
    if(memcmp(magic, ie, sizeof(magic)) != 0)
        return NULL;

    /* TODO: reverse engineering */

    return malloc(sizeof(ie_airmax_t));
}

void
ie_airmax_free(ie_airmax_t *context)
{
    if(context)
    {
        /* ... */
        free(context);
    }
}
