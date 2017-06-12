#include <math.h>
#include "ui-icons.h"
#include "model.h"

void
network_init(network_t *net)
{
    net->address = NULL;
    net->frequency = 0;
    net->channel = NULL;
    net->mode = NULL;
    net->ssid = NULL;
    net->radioname = NULL;
    net->rssi = MODEL_NO_SIGNAL;
    net->noise = MODEL_NO_SIGNAL;
    net->flags.active = FALSE;
    net->flags.routeros = FALSE;
    net->flags.privacy = FALSE;
    net->flags.nstreme = FALSE;
    net->flags.tdma = FALSE;
    net->flags.wds = FALSE;
    net->flags.bridge = FALSE;
    net->routeros_ver = NULL;
    net->firstseen = 0;
    net->lastseen = 0;
    net->latitude = NAN;
    net->longitude = NAN;
    net->azimuth = NAN;
    net->signals = NULL;
}

void
network_free(network_t *net)
{
    g_free(net->address);
    g_free(net->channel);
    g_free(net->mode);
    g_free(net->ssid);
    g_free(net->radioname);
    g_free(net->routeros_ver);
    if(net->signals)
        signals_free(net->signals);
}

void
network_free_null(network_t *net)
{
    network_free(net);
    net->address = NULL;
    net->channel = NULL;
    net->mode = NULL;
    net->ssid = NULL;
    net->radioname = NULL;
    net->routeros_ver = NULL;
    net->signals = NULL;
}
