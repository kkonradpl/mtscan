/*
 *  MTscan - MikroTik RouterOS wireless scanner
 *  Copyright (c) 2015-2023  Konrad Kosmatka
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
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <yajl/yajl_parse.h>
#include "gpsd.h"
#include "data.h"
#ifdef G_OS_WIN32
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <Mstcpip.h>
#include "../win32.h"
#define MSG_NOSIGNAL 0
typedef SOCKET gpsd_socket_t;
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
typedef int gpsd_socket_t;
#endif

#define DEBUG       0
#define DEBUG_READ  0
#define DEBUG_WRITE 0

/* Buffer length from gpsd gps.h (GPS_JSON_RESPONSE_MAX) */
#define GPSD_DATA_BUFFER_LEN  10240

#define GPSD_DATA_TIMEOUT_SEC 10

#define GPSD_TCP_KEEPCNT    2
#define GPSD_TCP_KEEPINTVL 10
#define GPSD_TCP_KEEPIDLE  30

#define GPSD_INIT_STRING "?WATCH={\"enable\":true,\"json\":true};"

typedef struct gnss_gpsd
{
    /* Configuration */
    gchar *hostname;
    gchar *port;
    guint  reconnect;

    /* Callback pointers */
    void (*cb_gpsd)(gnss_gpsd_t*);
    void (*cb_msg)(const gnss_msg_t*);

    /* Thread cancelation flag */
    volatile gboolean canceled;

    /* Private data */
    gpsd_socket_t fd;
    gint64 ts_connected;
    gboolean ready;
} gnss_gpsd_t;

typedef enum gpsd_json_level
{
    GPSD_JSON_LEVEL_ROOT,
    GPSD_JSON_LEVEL_OBJECT
} gpsd_json_level_t;

typedef enum gpsd_json_key
{
    GPSD_JSON_KEY_UNKNOWN = -1,
    GPSD_JSON_KEY_CLASS = 0,
    GPSD_JSON_KEY_DEVICE,
    GPSD_JSON_KEY_MODE,
    GPSD_JSON_KEY_TIME,
    GPSD_JSON_KEY_EPT,
    GPSD_JSON_KEY_LAT,
    GPSD_JSON_KEY_LON,
    GPSD_JSON_KEY_ALT,
    GPSD_JSON_KEY_EPX,
    GPSD_JSON_KEY_EPY,
    GPSD_JSON_KEY_EPV,
    GPSD_JSON_KEY_TRACK,
    GPSD_JSON_KEY_SPEED,
    GPSD_JSON_KEY_CLIMB,
    GPSD_JSON_KEY_EPS,
    GPSD_JSON_KEY_EPC
} gpsd_json_key_t;

static const gchar *const gpsd_json_keys[] =
{
    "class",
    "device",
    "mode",
    "time",
    "ept",
    "lat",
    "lon",
    "alt",
    "epx",
    "epy",
    "epv",
    "track",
    "speed",
    "climb",
    "eps",
    "epc"
};

typedef enum gpsd_json_class
{
    GPSD_JSON_CLASS_UNKNOWN = -1,
    GPSD_JSON_CLASS_VERSION = 0,
    GPSD_JSON_CLASS_TPV
} gpsd_json_class_t;

static const gchar *const gpsd_json_classes[] =
{
    "VERSION",
    "TPV"
};

typedef struct gpsd_json
{
    gnss_gpsd_t *context;
    gpsd_json_level_t level;
    gpsd_json_key_t key;
    gpsd_json_class_t class;
    gboolean ready;
    gint array;
    gnss_data_t *data;
} gpsd_json_t;

static gpointer gnss_gpsd_thread(gpointer);
static gboolean gnss_gpsd_open(gnss_gpsd_t*);
static gboolean gnss_gpsd_read(gnss_gpsd_t*);
static gboolean gnss_gpsd_write(gnss_gpsd_t*, const gchar*);
static gboolean gnss_gpsd_close(gnss_gpsd_t*);
static gboolean gnss_gpsd_parse(gnss_gpsd_t*, const gchar*);

static gint parse_integer(gpointer, long long int);
static gint parse_double(gpointer, gdouble);
static gint parse_string(gpointer, const guchar*, size_t);
static gint parse_key_start(gpointer);
static gint parse_key(gpointer, const guchar*, size_t);
static gint parse_key_end(gpointer);
static gint parse_array_start(gpointer);
static gint parse_array_end(gpointer);

static gboolean cb_gpsd(gpointer);
static gboolean cb_msg(gpointer);


gnss_gpsd_t*
gnss_gpsd_new(void        (*cb_gpsd)(gnss_gpsd_t*),
              void        (*cb_msg)(const gnss_msg_t*),
              const gchar  *hostname,
              gint          port,
              guint         reconnect)
{
    gnss_gpsd_t *context;
    context = g_malloc(sizeof(gnss_gpsd_t));

    /* Configuration */
    context->hostname = g_strdup(hostname);
    context->port = g_strdup_printf("%d", port);
    context->reconnect = reconnect;

    /* Callback pointers */
    context->cb_gpsd = cb_gpsd;
    context->cb_msg = cb_msg;

    /* Thread cancelation flag */
    context->canceled = FALSE;

    /* Private data */
#ifdef G_OS_WIN32
    context->fd = INVALID_SOCKET;
#else
    context->fd = -1;
#endif
    context->ts_connected = 0;
    context->ready = FALSE;

    g_thread_unref(g_thread_new("gnss_gpsd_thread", gnss_gpsd_thread, (gpointer)context));
    return context;
}

void
gnss_gpsd_free(gnss_gpsd_t *context)
{
    if(context)
    {
        /* Configuration */
        g_free(context->hostname);
        g_free(context->port);

        g_free(context);
    }
}

void
gnss_gpsd_cancel(gnss_gpsd_t *context)
{
    context->canceled = TRUE;
}

const gchar*
gnss_gpsd_get_hostname(const gnss_gpsd_t *context)
{
    return context->hostname;
}

const gchar*
gnss_gpsd_get_port(const gnss_gpsd_t *context)
{
    return context->port;
}

static gpointer
gnss_gpsd_thread(gpointer user_data)
{
    gnss_gpsd_t *context = (gnss_gpsd_t*)user_data;

#if DEBUG
    g_print("gnss_gpsd@%p: thread start\n", (gpointer)context);
#endif

    do
    {
        if(gnss_gpsd_open(context))
        {
            if(!gnss_gpsd_read(context))
                g_idle_add(cb_msg, gnss_msg_new(context, GNSS_MSG_INFO, GINT_TO_POINTER(GNSS_INFO_ERR_MISMATCH)));

            g_idle_add(cb_msg, gnss_msg_new(context, GNSS_MSG_INFO, GINT_TO_POINTER(GNSS_INFO_CLOSED)));
            gnss_gpsd_close(context);
        }

        if(!context->canceled && context->reconnect)
        {
            /* Reconnect with delay */
            g_usleep(context->reconnect * G_USEC_PER_SEC);
            context->ts_connected = 0;
            context->ready = FALSE;
        }
    } while(!context->canceled && context->reconnect);

#if DEBUG
    g_print("gnss_gpsd@%p: thread stop\n", (gpointer)context);
#endif
    g_idle_add(cb_gpsd, context);
    return NULL;
}

static gboolean
gnss_gpsd_open(gnss_gpsd_t *context)
{
    struct addrinfo hints;
    struct addrinfo *result;

#if DEBUG
    g_print("gnss_gpsd@%p: opening\n", (gpointer)context);
#endif
    g_idle_add(cb_msg, gnss_msg_new(context, GNSS_MSG_INFO, GINT_TO_POINTER(GNSS_INFO_OPENING)));

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    /* Resolve the hostname */
    g_idle_add(cb_msg, gnss_msg_new(context, GNSS_MSG_INFO, GINT_TO_POINTER(GNSS_INFO_RESOLVING)));
    if(getaddrinfo(context->hostname, context->port, &hints, &result))
    {
#if DEBUG
        g_print("gnss_gpsd@%p: failed to resolve the hostname\n", (gpointer)context);
#endif
        g_idle_add(cb_msg, gnss_msg_new(context, GNSS_MSG_INFO, GINT_TO_POINTER(GNSS_INFO_ERR_RESOLVING)));
        return FALSE;
    }

    if(context->canceled)
    {
        freeaddrinfo(result);
        return FALSE;
    }

    g_idle_add(cb_msg, gnss_msg_new(context, GNSS_MSG_INFO, GINT_TO_POINTER(GNSS_INFO_CONNECTING)));
    context->fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
#ifdef G_OS_WIN32
    if(context->fd == INVALID_SOCKET)
#else
    if(context->fd < 0)
#endif
    {
#if DEBUG
        g_print("gnss_gpsd@%p: failed to create a socket\n", (gpointer)context);
#endif
        freeaddrinfo(result);
        g_idle_add(cb_msg, gnss_msg_new(context, GNSS_MSG_INFO, GINT_TO_POINTER(GNSS_INFO_ERR_CONNECTING)));
        return FALSE;
    }

    if(connect(context->fd, result->ai_addr, result->ai_addrlen) < 0)
    {
#if DEBUG
        g_print("gnss_gpsd@%p: failed to connect\n", (gpointer)context);
#endif
        gnss_gpsd_close(context);
        freeaddrinfo(result);
        g_idle_add(cb_msg, gnss_msg_new(context, GNSS_MSG_INFO, GINT_TO_POINTER(GNSS_INFO_ERR_CONNECTING)));
        return FALSE;
    }
    freeaddrinfo(result);

    if(context->canceled)
    {
        gnss_gpsd_close(context);
        return FALSE;
    }

#ifdef G_OS_WIN32
    struct tcp_keepalive ka =
    {
        .onoff = 1,
        .keepaliveinterval = GPSD_TCP_KEEPINTVL * GPSD_TCP_KEEPCNT * 1000 / 10,
        .keepalivetime = GPSD_TCP_KEEPIDLE * 1000
    };
    DWORD ret;
    WSAIoctl(context->fd, SIO_KEEPALIVE_VALS, &ka, sizeof(ka), NULL, 0, &ret, NULL, NULL);
#else
    gint opt = 1;
    if(setsockopt(context->fd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt)) >= 0)
    {
        opt = GPSD_TCP_KEEPCNT;
        setsockopt(context->fd, IPPROTO_TCP, TCP_KEEPCNT, &opt, sizeof(opt));

        opt = GPSD_TCP_KEEPINTVL;
        setsockopt(context->fd, IPPROTO_TCP, TCP_KEEPINTVL, &opt, sizeof(opt));

        opt = GPSD_TCP_KEEPIDLE;
        setsockopt(context->fd, IPPROTO_TCP, TCP_KEEPIDLE, &opt, sizeof(opt));
    }
#endif

#if DEBUG
    g_print("gnss_gpsd@%p: connected\n", (gpointer)context);
#endif

    context->ts_connected = g_get_real_time();
    return TRUE;
}

static gboolean
gnss_gpsd_read(gnss_gpsd_t *context)
{
    struct timeval timeout;
    fd_set input;
    gchar buffer[GPSD_DATA_BUFFER_LEN+1];
    gchar *parser, *ptr;
    gint ret;
    size_t offset;
    ssize_t len;
    gint i;

    offset = 0;
    while(!context->canceled)
    {
        if (!context->ready &&
            g_get_real_time() - context->ts_connected > GPSD_DATA_TIMEOUT_SEC * G_USEC_PER_SEC)
        {
#if DEBUG
            g_print("gnss_gpsd@%p: data timeout (%d sec)\n", (gpointer)context, GPSD_DATA_TIMEOUT_SEC);
#endif
            g_idle_add(cb_msg, gnss_msg_new(context, GNSS_MSG_INFO, GINT_TO_POINTER(GNSS_INFO_ERR_TIMEOUT)));
            break;
        }

        FD_ZERO(&input);
        FD_SET(context->fd, &input);
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        ret = select(context->fd+1, &input, NULL, NULL, &timeout);
        if (!ret)
            continue;
        if (ret < 0)
            break;
        ptr = buffer + offset;
        len = recv(context->fd, ptr, GPSD_DATA_BUFFER_LEN-offset, 0);
        if (len <= 0)
            break;

        buffer[offset + len] = '\0';
        offset = 0;

        parser = buffer;
        while((ptr = strsep(&parser, "\n")))
        {
            if(parser)
            {
                /* Complete message */
#if DEBUG_READ
                g_print("gnss_gpsd@%p: read: %s\n", (gpointer) context, ptr);
#endif
                if(!gnss_gpsd_parse(context, ptr))
                    return FALSE;
            }
            else if(*ptr)
            {
                /* Incomplete message */
                offset = strlen(ptr);
                if (ptr != buffer)
                    for (i = 0; i < offset; i++)
                        buffer[i] = ptr[i];

                if (offset >= GPSD_DATA_BUFFER_LEN)
                {
#if DEBUG_READ
                    g_print("gnss_gpsd@%p: full buffer read: %s\n", (gpointer) context, ptr);
#endif
                    if(!gnss_gpsd_parse(context, ptr))
                        return FALSE;
                    offset = 0;
                }
                else
                {
#if DEBUG_READ
                    g_print("gnss_gpsd@%p: incomplete read (offset %zd)\n", (gpointer) context, offset);
#endif
                }
            }
        }
    }
    return TRUE;
}

static gboolean
gnss_gpsd_write(gnss_gpsd_t *context,
                const gchar *msg)
{
    size_t len = strlen(msg);
    size_t sent = 0;
    ssize_t n;

    do
    {
        n = send(context->fd, msg+sent, len-sent, MSG_NOSIGNAL);
        if(n < 0)
        {
#if (DEBUG && DEBUG_WRITE)
            g_print("gnss_gpsd@%p: write failed: %s\n", (gpointer)context, msg);
#endif
            return FALSE;
        }
        sent += n;
    } while(sent < len);

#if (DEBUG && DEBUG_WRITE)
    g_print("gnss_gpsd@%p: write: %s\n", (gpointer)context, msg);
#endif
    return TRUE;
}

static gboolean
gnss_gpsd_close(gnss_gpsd_t *context)
{
#ifdef G_OS_WIN32
    if(context->fd == INVALID_SOCKET)
        return FALSE;
    shutdown(context->fd, SD_BOTH);
    closesocket(context->fd);
    context->fd = INVALID_SOCKET;
#else
    if(context->fd < 0)
        return FALSE;
    shutdown(context->fd, SHUT_RDWR);
    close(context->fd);
    context->fd = -1;
#endif
#if DEBUG
    g_print("gnss_gpsd@%p: disconnected\n", (gpointer)context);
#endif
    return TRUE;
}

static gboolean
gnss_gpsd_parse(gnss_gpsd_t *context,
                const gchar *buffer)
{
    const yajl_callbacks json_callbacks =
    {
        NULL,               /* yajl_null        */
        NULL,               /* yajl_boolean     */
        &parse_integer,     /* yajl_integer     */
        &parse_double,      /* yajl_double      */
        NULL,               /* yajl_number      */
        &parse_string,      /* yajl_string      */
        &parse_key_start,   /* yajl_start_map   */
        &parse_key,         /* yajl_map_key     */
        &parse_key_end,     /* yajl_end_map     */
        &parse_array_start, /* yajl_start_array */
        &parse_array_end    /* yajl_end_array   */
    };

    yajl_handle handle;
    yajl_status status;
    gpsd_json_t json;
    size_t len;

    json.context = context;
    json.level = GPSD_JSON_LEVEL_ROOT;
    json.key = GPSD_JSON_KEY_UNKNOWN;
    json.class = GPSD_JSON_CLASS_UNKNOWN;
    json.array = 0;
    json.ready = FALSE;
    json.data = NULL;

    len = strlen(buffer);
    handle = yajl_alloc(&json_callbacks, NULL, &json);
    status = yajl_parse(handle, (const guchar*)buffer, len);

    if(status != yajl_status_ok)
    {
        /* Invalid message */
#if DEBUG
        g_print("gnss_gpsd@%p: protocol mismatch: invalid message\n", (gpointer)context);
#endif
        yajl_free(handle);
        return FALSE;
    }

    status = yajl_complete_parse(handle);
    yajl_free(handle);

#if DEBUG
    if(status != yajl_status_ok)
    {
        /* Longer responses will be truncated */
        if(len == GPSD_DATA_BUFFER_LEN)
            g_print("gnss_gpsd@%p: truncated message\n", (gpointer)context);
    }
#endif

    if(json.ready &&
       !context->ready)
    {
        context->ready = TRUE;
#if DEBUG
        g_print("gnss_gpsd@%p: ready\n", (gpointer)context);
#endif
        g_idle_add(cb_msg, gnss_msg_new(context, GNSS_MSG_INFO, GINT_TO_POINTER(GNSS_INFO_READY)));

        /* Send the init string */
        if(!gnss_gpsd_write(context, GPSD_INIT_STRING))
        {
#if DEBUG
            g_print("gnss_gpsd@%p: failed to write GPSD_INIT_STRING\n", (gpointer)context);
#endif
            gnss_data_free(json.data);
            return FALSE;
        }
    }

    if(!context->ready)
    {
#if DEBUG
        g_print("gnss_gpsd@%p: protocol mismatch: expected VERSION class message as first\n", (gpointer)context);
#endif
        gnss_data_free(json.data);
        return FALSE;
    }

    if(json.data)
    {
#if DEBUG
        g_print("gnss_gpsd@%p: device=%s, mode=%s: time=%" G_GINT64_FORMAT " "
                "ept=%f lat=%f lon=%f alt=%f epx=%f epy=%f epv=%f "
                "track=%f speed=%f climb=%f eps=%f epc=%f\n",
                (gpointer)context,
                gnss_data_get_device(json.data),
                (gnss_data_get_mode(json.data) == GNSS_MODE_NONE ? "none" :
                 (gnss_data_get_mode(json.data) == GNSS_MODE_2D ? "2D" :
                  (gnss_data_get_mode(json.data) == GNSS_MODE_3D ? "3D" : "unknown"))),
                gnss_data_get_time(json.data),
                gnss_data_get_ept(json.data),
                gnss_data_get_lat(json.data),
                gnss_data_get_lon(json.data),
                gnss_data_get_alt(json.data),
                gnss_data_get_epx(json.data),
                gnss_data_get_epy(json.data),
                gnss_data_get_epv(json.data),
                gnss_data_get_track(json.data),
                gnss_data_get_speed(json.data),
                gnss_data_get_climb(json.data),
                gnss_data_get_eps(json.data),
                gnss_data_get_epc(json.data));
#endif
        g_idle_add(cb_msg, gnss_msg_new(context, GNSS_MSG_DATA, json.data));
    }

    return TRUE;
}

static gint
parse_integer(gpointer      user_data,
              long long int value)
{
    gpsd_json_t *json = (gpsd_json_t*)user_data;

    if(json->level == GPSD_JSON_LEVEL_OBJECT &&
       json->class == GPSD_JSON_CLASS_TPV &&
       !json->array)
    {
        g_assert(json->data);

        if(json->key == GPSD_JSON_KEY_MODE)
            gnss_data_set_mode(json->data, (gnss_mode_t)value);
    }
    return 1;
}

static gint
parse_double(gpointer user_data,
             gdouble  value)
{
    gpsd_json_t *json = (gpsd_json_t*)user_data;

    if(json->level == GPSD_JSON_LEVEL_OBJECT &&
       json->class == GPSD_JSON_CLASS_TPV &&
       !json->array)
    {
        g_assert(json->data);

        if(json->key == GPSD_JSON_KEY_EPT)
            gnss_data_set_ept(json->data, value);
        else if(json->key == GPSD_JSON_KEY_LAT)
            gnss_data_set_lat(json->data, value);
        else if(json->key == GPSD_JSON_KEY_LON)
            gnss_data_set_lon(json->data, value);
        else if(json->key == GPSD_JSON_KEY_ALT)
            gnss_data_set_alt(json->data, value);
        else if(json->key == GPSD_JSON_KEY_EPX)
            gnss_data_set_epx(json->data, value);
        else if(json->key == GPSD_JSON_KEY_EPY)
            gnss_data_set_epy(json->data, value);
        else if(json->key == GPSD_JSON_KEY_EPV)
            gnss_data_set_epv(json->data, value);
        else if(json->key == GPSD_JSON_KEY_TRACK)
            gnss_data_set_track(json->data, value);
        else if(json->key == GPSD_JSON_KEY_SPEED)
            gnss_data_set_speed(json->data, value);
        else if(json->key == GPSD_JSON_KEY_CLIMB)
            gnss_data_set_climb(json->data, value);
        else if(json->key == GPSD_JSON_KEY_EPS)
            gnss_data_set_eps(json->data, value);
        else if(json->key == GPSD_JSON_KEY_EPC)
            gnss_data_set_epc(json->data, value);
    }
    return 1;
}

static gint
parse_string(gpointer      user_data,
             const guchar *string,
             size_t        length)
{
    gpsd_json_t *json = (gpsd_json_t*)user_data;
    gpsd_json_class_t i;
    GTimeVal tv;
    gchar *tmp;

    if(json->level == GPSD_JSON_LEVEL_OBJECT &&
       !json->array)
    {
        if(json->key == GPSD_JSON_KEY_CLASS &&
           json->class == GPSD_JSON_CLASS_UNKNOWN)
        {
            for(i=GPSD_JSON_CLASS_UNKNOWN+1; i<G_N_ELEMENTS(gpsd_json_classes); i++)
            {
                if(!strncmp(gpsd_json_classes[i], (const gchar*)string, length))
                {
                    json->class = i;
                    break;
                }
            }

            if(json->class == GPSD_JSON_CLASS_VERSION)
                json->ready = TRUE;
            else if(json->class == GPSD_JSON_CLASS_TPV)
                json->data = gnss_data_new();
        }
        else if(json->class == GPSD_JSON_CLASS_TPV)
        {
            g_assert(json->data);

            if(json->key == GPSD_JSON_KEY_DEVICE)
            {
                gnss_data_set_device(json->data, (const gchar*)string, length);
            }
            else if(json->key == GPSD_JSON_KEY_TIME)
            {
                tmp = g_strndup((const gchar*)string, length);
                if(g_time_val_from_iso8601(tmp, &tv))
                    gnss_data_set_time(json->data, tv.tv_sec);
                g_free(tmp);
            }
        }
    }
    return 1;
}

static gint
parse_key_start(gpointer user_data)
{
    gpsd_json_t *json = (gpsd_json_t*)user_data;
    json->level++;
    return 1;
}

static gint
parse_key(gpointer      user_data,
          const guchar *string,
          size_t        length)
{
    gpsd_json_t *json = (gpsd_json_t*)user_data;
    gpsd_json_key_t i;

    json->key = GPSD_JSON_KEY_UNKNOWN;

    if(json->level == GPSD_JSON_LEVEL_OBJECT &&
       !json->array)
    {
        for(i=GPSD_JSON_KEY_UNKNOWN+1; i<G_N_ELEMENTS(gpsd_json_keys); i++)
        {
            if(!strncmp(gpsd_json_keys[i], (const gchar*)string, length))
            {
                json->key = i;
                break;
            }
        }
    }
    return 1;
}

static gint
parse_key_end(gpointer user_data)
{
    gpsd_json_t *json = (gpsd_json_t*)user_data;
    json->level--;
    return 1;
}

static gint
parse_array_start(gpointer user_data)
{
    gpsd_json_t *json = (gpsd_json_t*)user_data;
    json->array++;
    return 1;
}

static gint
parse_array_end(gpointer user_data)
{
    gpsd_json_t *json = (gpsd_json_t*)user_data;
    json->array--;
    return 1;
}

static gboolean
cb_gpsd(gpointer user_data)
{
    gnss_gpsd_t *context = (gnss_gpsd_t*)user_data;
    context->cb_gpsd(context);
    return FALSE;
}

static gboolean
cb_msg(gpointer user_data)
{
    gnss_msg_t *msg = (gnss_msg_t*)user_data;
    const gnss_gpsd_t *src = gnss_msg_get_src(msg);

    if(!src->canceled)
        src->cb_msg(msg);

    gnss_msg_free(msg);
    return FALSE;
}
