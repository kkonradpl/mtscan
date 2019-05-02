/*
 *  MTscan - MikroTik RouterOS wireless scanner
 *  Copyright (c) 2015-2019  Konrad Kosmatka
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
#include <glib.h>
#include <yajl/yajl_parse.h>
#include "wigle-data.h"

typedef enum wigle_json_level
{
    WIGLE_JSON_LEVEL_ROOT,
    WIGLE_JSON_LEVEL_OBJECT,
    WIGLE_JSON_LEVEL_RESULT
} wigle_json_level_t;

typedef enum wigle_json_key
{
    WIGLE_JSON_KEY_UNKNOWN = -1,
    WIGLE_JSON_KEY_SUCCESS = 0,
    WIGLE_JSON_KEY_MESSAGE,
    WIGLE_JSON_KEY_RESULTS,
    WIGLE_JSON_KEY_TRILAT,
    WIGLE_JSON_KEY_TRILONG,
    WIGLE_JSON_KEY_SSID,
} wigle_json_key_t;

static const gchar *const wigle_json_keys[] =
{
    "success",
    "message",
    "results",
    "trilat",
    "trilong",
    "ssid"
};

typedef struct wigle_json
{
    yajl_handle handle;
    wigle_json_level_t level;
    wigle_json_key_t key;
    gint key_results;
    gint key_results_item;
    gint array;
    wigle_data_t *data;
} wigle_json_t;


static gint wigle_json_parse_boolean(gpointer, gint);
static gint wigle_json_parse_double(gpointer, gdouble);
static gint wigle_json_parse_string(gpointer, const guchar*, size_t);
static gint wigle_json_parse_key_start(gpointer);
static gint wigle_json_parse_key(gpointer, const guchar*, size_t);
static gint wigle_json_parse_key_end(gpointer);
static gint wigle_json_parse_array_start(gpointer);
static gint wigle_json_parse_array_end(gpointer);


wigle_json_t*
wigle_json_new(gint64 bssid)
{
    wigle_json_t *json;

    static const yajl_callbacks wigle_json_callbacks =
    {
        NULL,                          /* yajl_null        */
        &wigle_json_parse_boolean,     /* yajl_boolean     */
        NULL,                          /* yajl_integer     */
        &wigle_json_parse_double,      /* yajl_double      */
        NULL,                          /* yajl_number      */
        &wigle_json_parse_string,      /* yajl_string      */
        &wigle_json_parse_key_start,   /* yajl_start_map   */
        &wigle_json_parse_key,         /* yajl_map_key     */
        &wigle_json_parse_key_end,     /* yajl_end_map     */
        &wigle_json_parse_array_start, /* yajl_start_array */
        &wigle_json_parse_array_end    /* yajl_end_array   */
    };

    json = g_malloc(sizeof(wigle_json_t));

    json->handle = yajl_alloc(&wigle_json_callbacks, NULL, json);
    json->level = WIGLE_JSON_LEVEL_ROOT;
    json->key = WIGLE_JSON_KEY_UNKNOWN;
    json->key_results = 0;
    json->key_results_item = 0;
    json->array = 0;

    json->data = wigle_data_new();
    wigle_data_set_bssid(json->data, bssid);

    return json;
}

wigle_data_t*
wigle_json_free(wigle_json_t *json)
{
    wigle_data_t *data;
    data = json->data;
    yajl_free(json->handle);
    g_free(json);
    return data;
}

gboolean
wigle_json_parse_chunk(wigle_json_t *json,
                       const guchar *data,
                       size_t        len)
{
    return yajl_parse(json->handle, data, len) == yajl_status_ok;
}

gboolean
wigle_json_parse(wigle_json_t *json)
{
    return yajl_complete_parse(json->handle) == yajl_status_ok;
}

wigle_data_t*
wigle_json_get_data(wigle_json_t *json)
{
    return json->data;
}

static gint
wigle_json_parse_boolean(gpointer user_data,
                         gint     value)
{
    wigle_json_t *json = (wigle_json_t*)user_data;

    if(json->level == WIGLE_JSON_LEVEL_OBJECT &&
       !json->array)
    {
        if(json->key == WIGLE_JSON_KEY_SUCCESS)
            wigle_data_set_match(json->data, value);
    }
    return 1;
}

static gint
wigle_json_parse_double(gpointer user_data,
                        gdouble  value)
{
    wigle_json_t *json = (wigle_json_t*)user_data;

    if(json->level == WIGLE_JSON_LEVEL_RESULT &&
       json->array &&
       json->array == json->key_results &&
       json->key_results_item == 1)
    {
        if(json->key == WIGLE_JSON_KEY_TRILAT)
            wigle_data_set_lat(json->data, value);
        else if(json->key == WIGLE_JSON_KEY_TRILONG)
            wigle_data_set_lon(json->data, value);
    }
    return 1;
}

static gint
wigle_json_parse_string(gpointer      user_data,
                        const guchar *string,
                        size_t        length)
{
    wigle_json_t *json = (wigle_json_t*)user_data;

    if(json->level == WIGLE_JSON_LEVEL_OBJECT &&
       !json->array)
    {
        if(json->key == WIGLE_JSON_KEY_MESSAGE)
            wigle_data_set_message(json->data, g_strndup((const gchar*)string, length));
    }
    else if(json->level == WIGLE_JSON_LEVEL_RESULT &&
            json->array &&
            json->array == json->key_results &&
            json->key_results_item == 1)
    {
        if(json->key == WIGLE_JSON_KEY_SSID)
            wigle_data_set_ssid(json->data, g_strndup((const gchar*)string, length));
    }

    return 1;
}

static gint
wigle_json_parse_key_start(gpointer user_data)
{
    wigle_json_t *json = (wigle_json_t*)user_data;

    if(json->level == WIGLE_JSON_LEVEL_OBJECT &&
       json->array &&
       json->key_results == json->array)
    {
        json->key_results_item++;
    }

    json->level++;

    return 1;
}

static gint
wigle_json_parse_key(gpointer      user_data,
                     const guchar *string,
                     size_t        length)
{
    wigle_json_t *json = (wigle_json_t*)user_data;
    wigle_json_key_t i;

    json->key = WIGLE_JSON_KEY_UNKNOWN;
    for(i=WIGLE_JSON_KEY_UNKNOWN+1; i<G_N_ELEMENTS(wigle_json_keys); i++)
    {
        if(!strncmp(wigle_json_keys[i], (const gchar*)string, length))
        {
            json->key = i;
            break;
        }
    }

    return 1;
}

static gint
wigle_json_parse_key_end(gpointer user_data)
{
    wigle_json_t *json = (wigle_json_t*)user_data;
    json->level--;
    return 1;
}

static gint
wigle_json_parse_array_start(gpointer user_data)
{
    wigle_json_t *json = (wigle_json_t*)user_data;

    json->array++;

    if(json->level == WIGLE_JSON_LEVEL_OBJECT &&
       json->key == WIGLE_JSON_KEY_RESULTS)
    {
        json->key_results = json->array;
        json->key_results_item = 0;
    }

    return 1;
}

static gint
wigle_json_parse_array_end(gpointer user_data)
{
    wigle_json_t *json = (wigle_json_t*)user_data;

    if(json->level == WIGLE_JSON_LEVEL_OBJECT &&
       json->key_results == json->array)
    {
        json->key_results = 0;
    }

    json->array--;
    return 1;
}
