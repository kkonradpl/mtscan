#ifndef MTSCAN_SIGNALS_H_
#define MTSCAN_SIGNALS_H_
#include <glib.h>

typedef struct signals_node
{
    struct signals_node *next;
    gint64 timestamp;
    gdouble latitude;
    gdouble longitude;
    gint8 rssi;
    gfloat azimuth;
} signals_node_t;

typedef struct signals
{
    signals_node_t *head;
    signals_node_t *tail;
} signals_t;

signals_t* signals_new(void);
signals_node_t* signals_node_new0(void);
signals_node_t* signals_node_new(gint64, gint8, gdouble, gdouble, gfloat);
void signals_append(signals_t*, signals_node_t*);
void signals_merge(signals_t*, signals_t*);
void signals_free(signals_t*);

#endif
