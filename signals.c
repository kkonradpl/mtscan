#include <math.h>
#include "signals.h"

signals_t*
signals_new(void)
{
    return g_malloc0(sizeof(signals_t));
}

signals_node_t*
signals_node_new0(void)
{
    signals_node_t *sample = g_malloc(sizeof(signals_node_t));
    sample->timestamp = 0;
    sample->rssi = 0;
    sample->latitude = NAN;
    sample->longitude = NAN;
    sample->azimuth = NAN;
    return sample;
}

signals_node_t*
signals_node_new(gint64  timestamp,
                 gint8   rssi,
                 gdouble latitude,
                 gdouble longitude,
                 gfloat  azimuth)
{
    signals_node_t *sample = g_malloc(sizeof(signals_node_t));
    sample->timestamp = timestamp;
    sample->rssi = rssi;
    sample->latitude = latitude;
    sample->longitude = longitude;
    sample->azimuth = azimuth;
    return sample;
}

void
signals_append(signals_t      *list,
               signals_node_t *sample)
{
    sample->next = NULL;
    if(!list->head)
    {
        list->head = sample;
        list->tail = sample;
    }
    else
    {
        list->tail->next = sample;
        list->tail = sample;
    }
}

void
signals_merge(signals_t *list,
              signals_t *merge)
{
    signals_node_t *current_list;
    signals_node_t *current_merge;
    signals_node_t *tmp;

    if(merge->head == NULL)
        return;

    if(list->head == NULL)
    {
        /* Move */
        list->head = merge->head;
        list->tail = merge->tail;
    }
    else if(merge->tail->timestamp <= list->head->timestamp)
    {
        /* Prepend */
        merge->tail->next = list->head;
        list->head = merge->head;
    }
    else if(merge->head->timestamp >= list->tail->timestamp)
    {
        /* Append */
        list->tail->next = merge->head;
        list->tail = merge->tail;
    }
    else
    {
        /* Insert */
        current_list = list->head;
        current_merge = merge->head;
        while(current_merge)
        {
            tmp = current_merge->next;

            while (current_list->next &&
                   current_list->next->timestamp < current_merge->timestamp)
            {
                current_list = current_list->next;
            }
            if(!current_list->next)
                list->tail = current_merge;
            current_merge->next = current_list->next;
            current_list->next = current_merge;

            current_list = current_merge;
            current_merge = tmp;
        }
    }

    merge->head = NULL;
    merge->tail = NULL;
}

void
signals_free(signals_t *list)
{
    signals_node_t *current = list->head;
    signals_node_t *next;
    while(current)
    {
        next = current->next;
        g_free(current);
        current = next;
    }
    g_free(list);
}
