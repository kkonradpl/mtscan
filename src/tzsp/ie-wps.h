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

#ifndef MTSCAN_TZSP_IE_WPS_H
#define MTSCAN_TZSP_IE_WPS_H
#include <stdint.h>
#include <stdbool.h>

typedef struct ie_wps ie_wps_t;

ie_wps_t* ie_wps_parse(const uint8_t*, uint8_t);

const char* ie_wps_get_manufacturer(const ie_wps_t*);
const char* ie_wps_get_model_name(const ie_wps_t*);
const char* ie_wps_get_model_number(const ie_wps_t*);
const char* ie_wps_get_serial_number(const ie_wps_t*);
const char* ie_wps_get_device_name(const ie_wps_t*);

void ie_wps_free(ie_wps_t*);

#endif
