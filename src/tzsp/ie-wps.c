/*
 *  MTscan - MikroTik RouterOS wireless scanner
 *  Copyright (c) 2015-2022  Konrad Kosmatka
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
#include <stdio.h>
#include "ie-mikrotik.h"
#include "ie-mikrotik-utils.h"
#include "utils.h"

#define IE_WPS_HEADER_LEN 4

#define IE_WPS_TAG_HEADER_LEN 4

#define IE_WPS_TAG_MANUFACTURER 0x1021
#define IE_WPS_TAG_MODEL_NAME 0x1023
#define IE_WPS_TAG_MODEL_NUMBER 0x1024
#define IE_WPS_TAG_SERIAL_NUMBER 0x1042
#define IE_WPS_TAG_DEVICE_NAME 0x1011

typedef struct ie_wps
{
    char *manufacturer;
    char *model_name;
    char *model_number;
    char *serial_number;
    char *device_name;
} ie_wps_t;

static ie_wps_t* ie_wps_process_data(const uint8_t*, uint8_t);
static void ie_wps_process_tag(ie_wps_t*, uint16_t, uint16_t, const uint8_t*);

ie_wps_t*
ie_wps_parse(const uint8_t *ie,
             uint8_t        ie_len)
{
    static const uint8_t magic[] = { 0x00, 0x50, 0xf2, 0x04 };

    if(ie_len < IE_WPS_HEADER_LEN)
        return NULL;

    /* The IE must start with a magic byte sequence */
    if(memcmp(magic, ie, sizeof(magic)) != 0)
        return NULL;

    return ie_wps_process_data(ie, ie_len);
}

static ie_wps_t*
ie_wps_process_data(const uint8_t *ie,
                    uint8_t        ie_len)
{
    ie_wps_t *context;
    uint16_t tag;
    uint16_t tag_len;
    int i;

    context = calloc(sizeof(ie_wps_t), 1);
    if(context == NULL)
        return NULL;

    for(i=IE_WPS_HEADER_LEN;
        i+IE_WPS_TAG_HEADER_LEN <= ie_len;
        i+=IE_WPS_TAG_HEADER_LEN + tag_len)
    {
        tag = (ie[i] << 8) | ie[i+1];
        tag_len = (ie[i+2] << 8) | ie[i+3];

        if((i + IE_WPS_TAG_HEADER_LEN + tag_len) > ie_len)
            break;

        ie_wps_process_tag(context, tag, tag_len, ie+i+IE_WPS_TAG_HEADER_LEN);
    }

    return context;
}

static void
ie_wps_process_tag(ie_wps_t      *context,
                   uint16_t       type,
                   uint16_t       len,
                   const uint8_t *value)
{
    if(type == IE_WPS_TAG_MANUFACTURER && len)
    {
        if (!context->manufacturer)
            context->manufacturer = tzsp_utils_string(value, len);
    }
    else if(type == IE_WPS_TAG_MODEL_NAME && len)
    {
        if (!context->model_name)
            context->model_name = tzsp_utils_string(value, len);
    }
    else if(type == IE_WPS_TAG_MODEL_NUMBER && len)
    {
        if (!context->model_number)
            context->model_number = tzsp_utils_string(value, len);
    }
    else if(type == IE_WPS_TAG_SERIAL_NUMBER && len)
    {
        if (!context->serial_number)
            context->serial_number = tzsp_utils_string(value, len);
    }
    else if(type == IE_WPS_TAG_DEVICE_NAME && len)
    {
        if (!context->device_name)
            context->device_name = tzsp_utils_string(value, len);
    }
}

const char*
ie_wps_get_manufacturer(const ie_wps_t *context)
{
    return context->manufacturer;
}

const char*
ie_wps_get_model_name(const ie_wps_t *context)
{
    return context->model_name;
}

const char*
ie_wps_get_model_number(const ie_wps_t *context)
{
    return context->model_number;
}

const char*
ie_wps_get_serial_number(const ie_wps_t *context)
{
    return context->serial_number;
}

const char*
ie_wps_get_device_name(const ie_wps_t *context)
{
    return context->device_name;
}

void
ie_wps_free(ie_wps_t *context)
{
    if(context)
    {
        free(context->manufacturer);
        free(context->model_name);
        free(context->model_number);
        free(context->serial_number);
        free(context->device_name);
        free(context);
    }
}
