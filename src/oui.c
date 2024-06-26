/*
 *  MTscan - MikroTik RouterOS wireless scanner
 *  Copyright (c) 2015-2024  Konrad Kosmatka
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

#include <gtk/gtk.h>
#include <glib/gstdio.h>
#ifdef G_OS_WIN32
#include "win32.h"
#endif
#include <string.h>
#include <ctype.h>

#define OUI_MAX_LINE_LEN 256

typedef struct oui_context
{
    GHashTable *table;
    FILE *fp;
} oui_context_t;

static GHashTable *oui = NULL;

static gpointer oui_thread(gpointer);
static gboolean oui_thread_callback(gpointer);
static void trim_whitespaces(gchar*);


gboolean
oui_init(const gchar *filename)
{
    oui_context_t *context;
    FILE *fp;

    fp = g_fopen(filename, "r");
    if(!fp)
        return FALSE;

    context = g_malloc0(sizeof(oui_context_t));
    context->table = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, g_free);
    context->fp = fp;

    g_thread_unref(g_thread_new("oui_thread", oui_thread, context));
    return TRUE;
}

const gchar*
oui_lookup(gint64 address)
{
    const gchar* lookup = NULL;
    gint query;

    if(oui)
    {
        query = (gint)(address >> 24);
        lookup = g_hash_table_lookup(oui, GINT_TO_POINTER(query));
    }

    return lookup;
}

void
oui_destroy()
{
    if(oui)
    {
        g_hash_table_destroy(oui);
        oui = NULL;
    }
}


static gpointer
oui_thread(gpointer user_data)
{
    oui_context_t *context = (oui_context_t*)user_data;
    guint o1, o2, o3, oui;
    gchar line[OUI_MAX_LINE_LEN];
    gchar *token, *parser;
    gsize len;

    while(fgets(line, sizeof(line), context->fp) != NULL)
    {
        parser = line;

        /* First column, OUI address */
        token = strsep(&parser, "\t");
        if (token)
        {
            trim_whitespaces(token);
            if (strlen(token) != 8)
            {
                continue;
            }

            if (sscanf(token, "%x:%x:%x", &o1, &o2, &o3) != 3)
            {
                if (sscanf(token, "%x-%x-%x", &o1, &o2, &o3) != 3)
                {
                    if (sscanf(token, "%x.%x.%x", &o1, &o2, &o3) != 3)
                    {
                        continue;
                    }
                }
            }

            oui = ((o1 & 0xFF) << 16) | (o2 & 0xFF) << 8 | (o3 & 0xFF);

            /* Second column, truncated vendor name */
            token = strsep(&parser, "\t");
            if(token)
            {
                /* Third column, full vendor name */
                token = strsep(&parser, "\t");
                if(token)
                {
                    len = strlen(token);
                    if(len && token[len-1] == '\n')
                        token[len-1] = '\0';

                    g_hash_table_insert(context->table, GINT_TO_POINTER(oui), g_strdup(token));
                }
            }
        }
    }

    fclose(context->fp);
    g_idle_add(oui_thread_callback, context);
    return NULL;
}

static gboolean
oui_thread_callback(gpointer user_data)
{
    oui_context_t *context = (oui_context_t*)user_data;

    if(oui)
        g_hash_table_destroy(oui);

    oui = context->table;
    g_free(context);

    return G_SOURCE_REMOVE;
}

static void
trim_whitespaces(gchar *string)
{
    if (string)
    {
        size_t len = strlen(string);
        if (len)
        {
            size_t i;
            for (i = len - 1; i != 0; i--)
            {
                if (!isspace(string[i]))
                {
                    break;
                }

                string[i] = '\0';
            }
        }
    }
}
