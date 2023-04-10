/*
 *  MTscan - MikroTik RouterOS wireless scanner
 *  Copyright (c)  2023  Konrad Kosmatka
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

#ifndef MTSCAN_EXPORT_CSV_H_
#define MTSCAN_EXPORT_CSV_H_
#include "model.h"

gboolean export_csv(const gchar*, mtscan_model_t*);

#endif

