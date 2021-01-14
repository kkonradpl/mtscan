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

#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>

#define MIKROTIK_VERSION_ALPHA     'a'
#define MIKROTIK_VERSION_BETA      'b'
#define MIKROTIK_VERSION_CANDIDATE 'c'
#define MIKROTIK_VERSION_FINAL     'f'

static const char* ie_mikrotik_version_separator(uint8_t);

char*
ie_mikrotik_version(uint8_t major,
                    uint8_t minor,
                    uint8_t type,
                    uint8_t rev)
{
    char *version;

    if(rev)
    {
        if(asprintf(&version, "%d.%d%s%d",
                    major,
                    minor,
                    ie_mikrotik_version_separator(type),
                    rev) < 0)
            version = NULL;
    }
    else
    {
        if(asprintf(&version, "%d.%d",
                    major,
                    minor) < 0)
            version = NULL;
    }

    return version;
}

static const char*
ie_mikrotik_version_separator(uint8_t type)
{
    static const char* version_alpha     = "alpha";
    static const char* version_beta      = "beta";
    static const char* version_candidate = "rc";
    static const char* version_final     = ".";
    static const char* version_unknown   = "";

    switch(type)
    {
        case MIKROTIK_VERSION_ALPHA:
            return version_alpha;
        case MIKROTIK_VERSION_BETA:
            return version_beta;
        case MIKROTIK_VERSION_CANDIDATE:
            return version_candidate;
        case MIKROTIK_VERSION_FINAL:
            return version_final;
        default:
            return version_unknown;
    }
}
