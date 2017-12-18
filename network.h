#ifndef MTSCAN_NETWORK_H_
#define MTSCAN_NETWORK_H_
#include <gtk/gtk.h>
#include "signals.h"

typedef struct network_flags
{
    gboolean active;
    gboolean privacy;
    gboolean routeros;
    gboolean nstreme;
    gboolean tdma;
    gboolean wds;
    gboolean bridge;
} network_flags_t;

typedef struct network
{
    gint64 address;
    gint frequency;
    gchar *channel;
    gchar *mode;
    gchar *ssid;
    gchar *radioname;
    gint rssi;
    gint noise;
    network_flags_t flags;
    gchar *routeros_ver;
    gint64 firstseen;
    gint64 lastseen;
    gdouble latitude;
    gdouble longitude;
    gfloat azimuth;
    signals_t *signals;
} network_t;

void network_init(network_t*);
void network_free(network_t*);
void network_free_null(network_t*);

#endif

