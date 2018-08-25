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

#include <gtk/gtk.h>
#include <string.h>
#include <inttypes.h>
#include "mtscan.h"

#ifdef G_OS_WIN32
#include "win32.h"
#endif

#define MAC_ADDR_HEX_LEN 12

static gboolean create_liststore_from_tree_foreach(gpointer, gpointer, gpointer);
static gboolean fill_tree_from_liststore_foreach(GtkTreeModel*, GtkTreePath*, GtkTreeIter*, gpointer);

gint
gptrcmp(gconstpointer a,
        gconstpointer b)
{
    return GPOINTER_TO_INT(a) - GPOINTER_TO_INT(b);
}

gint
gint64cmp(const gint64 *a,
          const gint64 *b)
{
    if(*a < *b)
        return -1;
    if(*a > *b)
        return 1;
    return 0;
}

gint64*
gint64dup(const gint64* src)
{
    gint64 *dst;
    dst = g_malloc(sizeof(gint64));
    *dst = *src;
    return dst;
}

void
remove_char(gchar *str,
            gchar  c)
{
    gchar *pr = str;
    gchar *pw = str;
    while(*pr)
    {
        *pw = *pr++;
        pw += (*pw != c);
    }
    *pw = 0;
}

gchar*
str_scanlist_compress(const gchar *input)
{
    GString *output;
    gchar **channels;
    gchar **ch;
    gchar *endptr;
    gint prev, curr, last_written;

    if(!input)
        return NULL;

    output = g_string_new(NULL);

    /* Reset buffers */
    prev = 0;
    last_written = 0;

    channels = g_strsplit(input, ",", -1);
    for(ch = channels; *ch; ch++)
    {
        if(!strlen(*ch))
            continue;

        curr = (gint)g_ascii_strtoull(*ch, &endptr, 10);
        if(curr && *endptr == '\0')
        {
            /* The frequency was successfully parsed as integer */
            if(!prev)
            {
                /* There is no buffered frequency */
                g_string_append_printf(output, "%s%s", (output->len ? "," : ""), *ch);

                /* Save current frequency for further processing */
                prev = curr;
                last_written = curr;
            }
            else
            {
                /* There is a previous frequency */
                if(prev != curr-5 ||
                   (curr % 5 != 0))
                {
                    /* Current frequency is not greater by 5 MHz than previous */
                    /* Also it isn't a multiple of 5 (compress only 5 GHz band ranges) */
                    if(last_written != prev)
                    {
                        /* There's something buffered, flush it */
                        g_string_append_printf(output, "-%d", prev);
                    }

                    g_string_append_printf(output, ",%d", curr);
                    last_written = curr;
                }

                /* Buffer current frequency */
                prev = curr;
            }
        }
        else
        {
            if(last_written != prev)
            {
                /* There's something buffered, flush it */
                g_string_append_printf(output, "-%d", prev);
            }

            g_string_append_printf(output, "%s%s", (output->len ? "," : ""), *ch);

            /* Reset buffers */
            prev = 0;
            last_written = 0;
        }
    }

    if(last_written != prev)
    {
        /* There's something buffered, flush it */
        g_string_append_printf(output, "-%d", prev);
    }

    g_strfreev(channels);
    return g_string_free(output, FALSE);
}

GtkListStore*
create_liststore_from_tree(GTree *tree)
{
    GtkListStore *model = gtk_list_store_new(1, G_TYPE_INT64);
    g_tree_foreach(tree, create_liststore_from_tree_foreach, model);
    return model;
}

static gboolean
create_liststore_from_tree_foreach(gpointer key,
                                   gpointer value,
                                   gpointer data)
{
    gint64 *v = (gint64*)key;
    GtkListStore *model = (GtkListStore*)data;
    gtk_list_store_insert_with_values(model, NULL, -1, 0, *v, -1);
    return FALSE;
}

void
fill_tree_from_liststore(GTree        *tree,
                         GtkListStore *model)
{
    g_tree_ref(tree);
    g_tree_destroy(tree);
    gtk_tree_model_foreach(GTK_TREE_MODEL(model), fill_tree_from_liststore_foreach, tree);
}

static gboolean
fill_tree_from_liststore_foreach(GtkTreeModel *model,
                                 GtkTreePath  *path,
                                 GtkTreeIter  *iter,
                                 gpointer      data)
{
    GTree *tree = (GTree*)data;
    gint64 value;

    gtk_tree_model_get(model, iter, 0, &value, -1);
    g_tree_insert(tree, gint64dup(&value), GINT_TO_POINTER(TRUE));
    return FALSE;
}

gint64
str_addr_to_gint64(const gchar* str,
                   gint         len)
{
    gchar buffer[MAC_ADDR_HEX_LEN+1];
    gchar *ptr;
    gint64 value;
    gint i;

    if(len == MAC_ADDR_HEX_LEN)
    {
        for(i=0; i<len; i++)
        {
            if(!((str[i] >= '0' && str[i] <= '9') ||
                (str[i] >= 'A' && str[i] <= 'F') ||
                (str[i] >= 'a' && str[i] <= 'f')))
                return -1;
        }

        memcpy(buffer, str, MAC_ADDR_HEX_LEN);
        buffer[MAC_ADDR_HEX_LEN] = '\0';
        value = g_ascii_strtoll(buffer, &ptr, 16);
        if(ptr != buffer)
            return value;
    }
    return -1;
}

gboolean
str_addr_to_guint8(const gchar *str,
                   gint         len,
                   guint8      *buff)
{
    gint64 addr_buff;
    guint8 *ptr;
    gint i;

    addr_buff = str_addr_to_gint64(str, len);

    if(addr_buff < 0)
        return FALSE;

    ptr = buff;
    for(i=5; i>=0; i--)
        *ptr++ = (uint8_t) (addr_buff >> (CHAR_BIT * i));

    return TRUE;
}

void
mtscan_sound(const gchar *filename)
{
    gchar *path;
    path = g_build_filename(APP_SOUND_DIR, filename, NULL);

#ifdef G_OS_WIN32
    win32_play(path);
#else
    gchar *command[] = { APP_SOUND_EXEC, path, NULL };
    GError *error = NULL;
    if(!g_spawn_async(NULL, command, NULL, G_SPAWN_SEARCH_PATH, 0, NULL, NULL, &error))
    {
        fprintf(stderr, "Unable to start " APP_SOUND_EXEC ": %s\n", error->message);
        g_error_free(error);
    }
#endif

    g_free(path);
}

gchar*
timestamp_to_filename(const gchar *path,
                      gint64       ts)
{
    static gchar output[50];
    struct tm *tm;
#ifdef G_OS_WIN32
    __time64_t ts_val;

    ts_val = (__time64_t)ts;
    tm = _localtime64(&ts_val);
#else
    time_t ts_val;

    ts_val = (time_t)ts;
    tm = localtime(&ts_val);
#endif

    strftime(output, sizeof(output), "%Y%m%d-%H%M%S" APP_FILE_EXT APP_FILE_COMPRESS, tm);
    return g_build_filename(path, output, NULL);
}
