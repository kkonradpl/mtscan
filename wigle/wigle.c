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
#include <curl/curl.h>
#include "wigle.h"
#include "wigle-msg.h"
#include "wigle-json.h"

#define DEBUG      1
#define DEBUG_READ 0

#define WIGLE_QUEUE_TIMEOUT   500
#define WIGLE_REQUEST_DELAY   0
#define WIGLE_REQUEST_TIMEOUT 15L

#define WIGLE_TOO_MANY_QUERIES "too many queries"


typedef struct wigle
{
    /* Configuration */
    gchar *url;
    gchar *key;

    /* Callback pointers */
    void (*cb)    (wigle_t*);
    void (*cb_msg)(const wigle_t*, const wigle_data_t*);

    /* Thread cancelation flag */
    volatile gboolean canceled;

    /* Private data */
    GMutex conf_mutex;
    GAsyncQueue *queue;
    CURL *curl;
    wigle_json_t *json;
} wigle_t;


static gpointer wigle_thread(gpointer);
static size_t   wigle_process(gpointer, size_t, size_t, gpointer);
static gboolean wigle_cb(gpointer);
static gboolean wigle_cb_msg(gpointer);


wigle_t*
wigle_new(void        (*cb)(wigle_t*),
          void        (*cb_msg)(const wigle_t*, const wigle_data_t*),
          const gchar  *url,
          const gchar  *key)
{
    wigle_t *context;
    context = g_malloc(sizeof(wigle_t));

    /* Configuration */
    context->url = g_strdup(url);
    context->key = g_strdup(key);

    /* Callback pointers */
    context->cb = cb;
    context->cb_msg = cb_msg;

    /* Thread cancelation flag */
    context->canceled = FALSE;

    /* Private data */
    g_mutex_init(&context->conf_mutex);
    context->queue = g_async_queue_new_full(g_free);
    context->curl = curl_easy_init();

    if(!context->curl)
    {
        wigle_free(context);
        return NULL;
    }

    g_thread_unref(g_thread_new("wigle_thread", wigle_thread, (gpointer)context));
    return context;
}

void
wigle_set_config(wigle_t     *context,
                 const gchar *url,
                 const gchar *key)
{

    g_mutex_lock(&context->conf_mutex);

    g_free(context->url);
    context->url = g_strdup(url);

    g_free(context->key);
    context->key = g_strdup(key);

    g_mutex_unlock(&context->conf_mutex);
}

void
wigle_free(wigle_t *context)
{
    if(context)
    {
        g_free(context->url);
        g_free(context->key);

        g_async_queue_unref(context->queue);
        curl_easy_cleanup(context->curl);

        g_free(context);
    }
}

void
wigle_lookup(wigle_t *context,
             gint64   addr)
{
    gint64 *ptr;

    if(context)
    {
        ptr = g_malloc(sizeof(gint64));
        *ptr = addr;
        g_async_queue_push(context->queue, ptr);
    }
}

void
wigle_cancel(wigle_t *context)
{
    if(context)
        context->canceled = TRUE;
}

static gpointer
wigle_thread(gpointer user_data)
{
    wigle_t *context = (wigle_t*)user_data;
    struct curl_slist *chunk;
    CURLcode status;
    gint64 *bssid;
    gchar *bssid_string;
    gchar *auth_string;
    gchar *url_string;
    wigle_data_t *data;
    const gchar *message;

#if DEBUG
    g_print("wigle_thread@%p: start\n", (gpointer)context);
#endif

    curl_easy_setopt(context->curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);
    curl_easy_setopt(context->curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(context->curl, CURLOPT_TIMEOUT, WIGLE_REQUEST_TIMEOUT);
    curl_easy_setopt(context->curl, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(context->curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(context->curl, CURLOPT_WRITEFUNCTION, wigle_process);
    curl_easy_setopt(context->curl, CURLOPT_WRITEDATA, context);

    while(!context->canceled)
    {
        bssid = g_async_queue_timeout_pop(context->queue, WIGLE_QUEUE_TIMEOUT * 1000L);

        if(!bssid)
            continue;

        context->json = wigle_json_new(*bssid);

        bssid_string = g_strdup_printf("%02X:%02X:%02X:%02X:%02X:%02X",
                                       (gint)((*bssid >> 40) & 0xFF),
                                       (gint)((*bssid >> 32) & 0xFF),
                                       (gint)((*bssid >> 24) & 0xFF),
                                       (gint)((*bssid >> 16) & 0xFF),
                                       (gint)((*bssid >>  8) & 0xFF),
                                       (gint)((*bssid >>  0) & 0xFF));

        g_mutex_lock(&context->conf_mutex);
        auth_string = g_strdup_printf("Authorization: basic %s", context->key);
        url_string = g_strdup_printf("%s%s", context->url, bssid_string);
        g_mutex_unlock(&context->conf_mutex);

        chunk = NULL;
        chunk = curl_slist_append(chunk, "Accept: application/json");
        chunk = curl_slist_append(chunk, auth_string);
        curl_easy_setopt(context->curl, CURLOPT_HTTPHEADER, chunk);

        curl_easy_setopt(context->curl, CURLOPT_URL, url_string);
        status = curl_easy_perform(context->curl);

        if(status == CURLE_OK)
        {
            if(wigle_json_parse(context->json))
            {
                message = wigle_data_get_message(wigle_json_get_data(context->json));
                if(g_ascii_strcasecmp(message, WIGLE_TOO_MANY_QUERIES) == 0)
                {
#if DEBUG
                    g_print("wigle@%p: too many queries\n", (gpointer)context);
#endif
                    wigle_data_set_error(wigle_json_get_data(context->json), WIGLE_ERROR_LIMIT_EXCEEDED);
                }
                else
                {
#if DEBUG
                    g_print("wigle@%p: match: %s (%d)\n",
                            (gpointer)context,
                            bssid_string,
                            wigle_data_get_match(wigle_json_get_data(context->json)));
#endif
                }
            }
            else
            {
#if DEBUG
                g_print("wigle@%p: protocol mismatch (incomplete response)\n", (gpointer)context);
#endif
                wigle_data_set_error(wigle_json_get_data(context->json), WIGLE_ERROR_INCOMPLETE);
            }
        }
        else if(status == CURLE_WRITE_ERROR)
        {
#if DEBUG
            g_print("wigle@%p: protocol mismatch (invalid response)\n", (gpointer)context);
#endif
            wigle_data_set_error(wigle_json_get_data(context->json), WIGLE_ERROR_MISMATCH);
        }
        else if(status == CURLE_OPERATION_TIMEDOUT)
        {
#if DEBUG
            g_print("wigle@%p: connection timed out\n", (gpointer)context);
#endif
            wigle_data_set_error(wigle_json_get_data(context->json), WIGLE_ERROR_TIMEOUT);
        }
        else if(status == CURLE_COULDNT_CONNECT)
        {
#if DEBUG
            g_print("wigle@%p: unable to connect\n", (gpointer)context);
#endif
            wigle_data_set_error(wigle_json_get_data(context->json), WIGLE_ERROR_CONNECT);
        }
        else
        {
#if DEBUG
            g_print("wigle@%p: unknown error\n", (gpointer)context);
#endif
            wigle_data_set_error(wigle_json_get_data(context->json), WIGLE_ERROR_UNKNOWN);
        }

        data = wigle_json_free(context->json);
        g_idle_add(wigle_cb_msg, wigle_msg_new(context, data));

        curl_slist_free_all(chunk);
        g_free(auth_string);
        g_free(url_string);
        g_free(bssid_string);
        g_free(bssid);

        g_usleep(WIGLE_REQUEST_DELAY * 1000UL);
    }

#if DEBUG
    g_print("wigle_thread@%p: stop\n", (gpointer)context);
#endif

    g_idle_add(wigle_cb, context);
    return NULL;
}

static size_t
wigle_process(gpointer data,
              size_t   size,
              size_t   nmemb,
              gpointer user_data)
{
    wigle_t *context = (wigle_t*)user_data;

#if DEBUG_READ
    fwrite(data, size, nmemb, stdout);
#endif

    if(!wigle_json_parse_chunk(context->json, (const guchar*)data, nmemb*size))
    {
        /* Invalid response */
        return 0;
    }

    return nmemb*size;
}

static gboolean
wigle_cb(gpointer user_data)
{
    wigle_t *context = (wigle_t*)user_data;
    context->cb(context);
    return FALSE;
}

static gboolean
wigle_cb_msg(gpointer user_data)
{
    wigle_msg_t *msg = (wigle_msg_t*)user_data;
    const wigle_t *src = wigle_msg_get_src(msg);
    const wigle_data_t *data = wigle_msg_get_data(msg);

    if(!src->canceled)
        src->cb_msg(src, data);

    wigle_msg_free(msg);
    return FALSE;
}
