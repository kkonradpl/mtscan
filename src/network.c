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

#include <math.h>
#include "ui-icons.h"
#include "model.h"

static void convert_to_utf8(gchar**, const gchar *);

void
network_init(network_t *net)
{
    net->address = -1;
    net->frequency = 0;
    net->channel = NULL;
    net->mode = NULL;
    net->streams = 0;
    net->ssid = NULL;
    net->radioname = NULL;
    net->rssi = MODEL_NO_SIGNAL;
    net->noise = MODEL_NO_SIGNAL;
    net->flags.routeros = FALSE;
    net->flags.privacy = FALSE;
    net->flags.nstreme = FALSE;
    net->flags.tdma = FALSE;
    net->flags.wds = FALSE;
    net->flags.bridge = FALSE;
    net->routeros_ver = NULL;
    net->ubnt_airmax = FALSE;
    net->ubnt_ptp = FALSE;
    net->ubnt_ptmp = FALSE;
    net->ubnt_mixed = FALSE;
    net->firstseen = 0;
    net->lastseen = 0;
    net->latitude = NAN;
    net->longitude = NAN;
    net->azimuth = NAN;
    net->signals = NULL;
}

void
network_to_utf8(network_t   *net,
                const gchar *charset)
{
    if(net->channel && !g_utf8_validate(net->channel, -1, NULL))
        convert_to_utf8(&net->channel, charset);

    if(net->mode && !g_utf8_validate(net->mode, -1, NULL))
        convert_to_utf8(&net->mode, charset);

    if(net->ssid && !g_utf8_validate(net->ssid, -1, NULL))
        convert_to_utf8(&net->ssid, charset);

    if(net->radioname && !g_utf8_validate(net->radioname, -1, NULL))
        convert_to_utf8(&net->radioname, charset);

    if(net->routeros_ver && !g_utf8_validate(net->routeros_ver, -1, NULL))
        convert_to_utf8(&net->routeros_ver, charset);
}

static void
convert_to_utf8(gchar       **ptr,
                const gchar  *charset)
{
    gchar *input = *ptr;
    gchar *output;
    gsize bytes_written;

    output = g_convert(input,
                       -1,
                       "UTF-8",
                       charset,
                       NULL,
                       &bytes_written,
                       NULL);

    g_free(input);
    *ptr = output;
}

void
network_free(network_t *net)
{
    if(net)
    {
        g_free(net->channel);
        g_free(net->mode);
        g_free(net->ssid);
        g_free(net->radioname);
        g_free(net->routeros_ver);
        if (net->signals)
            signals_free(net->signals);
    }
}

void
network_free_null(network_t *net)
{
    network_free(net);
    net->channel = NULL;
    net->mode = NULL;
    net->ssid = NULL;
    net->radioname = NULL;
    net->routeros_ver = NULL;
    net->signals = NULL;
}
