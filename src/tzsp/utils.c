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

#ifndef MTSCAN_UTILS_H
#define MTSCAN_UTILS_H

#include <string.h>
#include <stdlib.h>
#include "utils.h"

char* tzsp_utils_string(const uint8_t *input,
                        size_t         maxlen)
{
    size_t len;
    char *output;

    len = strnlen((const char*)input, maxlen);
    output = malloc(sizeof(char)*(len+1));
    if(output)
    {
        memcpy(output, (const char *)input, len);
        output[len] = '\0';
    }

    return output;
}

#endif