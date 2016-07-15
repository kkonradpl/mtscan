#include <math.h>
#include "signals.h"

signals_t*
signals_new()
{
    return g_malloc0(sizeof(signals_t));
}

signals_node_t*
signals_node_new0()
{
    signals_node_t *sample = g_malloc(sizeof(signals_node_t));
    sample->timestamp = 0;
    sample->rssi = 0;
    sample->latitude = NAN;
    sample->longitude = NAN;
    return sample;
}

signals_node_t*
signals_node_new(gint64  timestamp,
                 gint8   rssi,
                 gdouble latitude,
                 gdouble longitude)
{
    signals_node_t *sample = g_malloc(sizeof(signals_node_t));
    sample->timestamp = timestamp;
    sample->rssi = rssi;
    sample->latitude = latitude;
    sample->longitude = longitude;
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

gboolean
signals_merge_and_free(signals_t **list,
                       signals_t  *merge)
{
    signals_node_t *current_list;
    signals_node_t *current_merge;
    signals_node_t *tmp;

    if((*list)->head == NULL)
    {
        g_free(*list);
        *list = merge;
        /* Return TRUE when pointer to the list changes */
        return TRUE;
    }

    if(merge->head == NULL)
    {
        g_free(merge);
        return FALSE;
    }

    current_merge = merge->head;
    if(merge->tail->timestamp <= (*list)->head->timestamp)
    {
        /* Prepend */
        while(current_merge)
        {
            tmp = current_merge->next;
            current_merge->next = (*list)->head;
            (*list)->head = current_merge;
            current_merge = tmp;
        }
    }
    else if(merge->head->timestamp >= (*list)->tail->timestamp)
    {
        /* Append */
        while(current_merge)
        {
            tmp = current_merge->next;
            current_merge->next = NULL;
            (*list)->tail->next = current_merge;
            (*list)->tail = current_merge;
            current_merge = tmp;
        }
    }
    else
    {
        /* Insert */
        current_list = (*list)->head;
        while(current_merge)
        {
            tmp = current_merge->next;

            while (current_list->next &&
                   current_list->next->timestamp < current_merge->timestamp)
            {
                current_list = current_list->next;
            }
            if(!current_list->next)
                (*list)->tail = current_merge;
            current_merge->next = current_list->next;
            current_list->next = current_merge;

            current_list = current_merge;
            current_merge = tmp;
        }
    }

    g_free(merge);
    return FALSE;
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
