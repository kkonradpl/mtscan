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

#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <yajl/yajl_parse.h>
#include "gpsd.h"
#ifdef G_OS_WIN32
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <Mstcpip.h>
#include "win32.h"
#define MSG_NOSIGNAL 0
typedef SOCKET gpsd_socket_t;
#else
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
typedef int gpsd_socket_t;
#endif

#define DEBUG       1
#define DEBUG_READ  0
#define DEBUG_WRITE 0

/* The gpsd_json manual says:
 *
 * >The length limit for responses and reports is 1536 characters, including trailing
 * >newline; longer responses will be truncated, so client code must be prepared for
 * >the possibility of invalid JSON fragments.
 *
 * However, in year 2011, the GPS_JSON_RESPONSE_MAX was changed from 1536 to 4096.
 * https://git.savannah.gnu.org/cgit/gpsd.git/commit/?id=cd86d4410bcff417de15a0fc5eb905ef56c2659
 */

#define GPSD_DATA_BUFFER_LEN  4096
#define GPSD_DATA_TIMEOUT_SEC 10

#define GPSD_TCP_KEEPCNT    2
#define GPSD_TCP_KEEPINTVL 10
#define GPSD_TCP_KEEPIDLE  30

#define GPSD_INIT_STRING "?WATCH={\"enable\":true,\"json\":true};"

typedef struct gpsd_msg
{
    const gpsd_conn_t *src;
    gpsd_msg_type_t    type;
    gpointer           data;
} gpsd_msg_t;

typedef struct gpsd_data
{
    gchar *device;
    gpsd_mode_t mode;
    gint64 time;
    gdouble ept;
    gdouble lat;
    gdouble lon;
    gdouble alt;
    gdouble epx;
    gdouble epy;
    gdouble epv;
    gdouble track;
    gdouble speed;
    gdouble climb;
    gdouble eps;
    gdouble epc;
} gpsd_data_t;

typedef struct gpsd_conn
{
    /* Configuration */
    gchar *hostname;
    gchar *port;
    guint  reconnect;

    /* Callback pointers */
    void (*cb)    (gpsd_conn_t*);
    void (*cb_msg)(const gpsd_conn_t*, gpsd_msg_type_t, gconstpointer);

    /* Thread cancelation flag */
    volatile gboolean canceled;

    /* Private data */
    gpsd_socket_t fd;
    gint64 ts_connected;
    gboolean ready;
} gpsd_conn_t;

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
    gpsd_conn_t *context;
    gpsd_json_level_t level;
    gpsd_json_key_t key;
    gpsd_json_class_t class;
    gboolean ready;
    gint array;
    gpsd_data_t *data;
} gpsd_json_t;

static gpsd_msg_t* gpsd_msg_new(const gpsd_conn_t*, gpsd_msg_type_t, gpointer);
static void gpsd_msg_free(gpsd_msg_t*);

static gpsd_data_t* gpsd_data_new();
static void gpsd_data_free(gpsd_data_t*);

static gboolean gpsd_cb(gpointer);
static gboolean gpsd_cb_msg(gpointer);

static gpointer gpsd_conn_thread(gpointer);
static gboolean gpsd_conn_open(gpsd_conn_t*);
static gboolean gpsd_conn_read(gpsd_conn_t*);
static gboolean gpsd_conn_write(gpsd_conn_t*, const gchar*);
static gboolean gpsd_conn_close(gpsd_conn_t*);
static gboolean gpsd_conn_parse(gpsd_conn_t*, const gchar*);

static gint parse_integer(gpointer, long long int);
static gint parse_double(gpointer, gdouble);
static gint parse_string(gpointer, const guchar*, size_t);
static gint parse_key_start(gpointer);
static gint parse_key(gpointer, const guchar*, size_t);
static gint parse_key_end(gpointer);
static gint parse_array_start(gpointer);
static gint parse_array_end(gpointer);

gpsd_conn_t*
gpsd_conn_new(void        (*cb)(gpsd_conn_t*),
              void        (*cb_msg)(const gpsd_conn_t*, gpsd_msg_type_t, gconstpointer),
              const gchar  *hostname,
              gint          port,
              guint         reconnect)
{
    gpsd_conn_t *context;
    context = g_malloc(sizeof(gpsd_conn_t));

    /* Configuration */
    context->hostname = g_strdup(hostname);
    context->port = g_strdup_printf("%d", port);
    context->reconnect = reconnect;

    /* Callback pointers */
    context->cb = cb;
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

    g_thread_unref(g_thread_new("gpsd_conn_thread", gpsd_conn_thread, (gpointer)context));
    return context;
}

void
gpsd_conn_free(gpsd_conn_t *context)
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
gpsd_conn_cancel(gpsd_conn_t *context)
{
    context->canceled = TRUE;
}

const gchar*
gpsd_conn_get_hostname(const gpsd_conn_t *context)
{
    return context->hostname;
}

const gchar*
gpsd_conn_get_port(const gpsd_conn_t *context)
{
    return context->port;
}

static gpsd_msg_t*
gpsd_msg_new(const gpsd_conn_t *src,
            gpsd_msg_type_t     type,
            gpointer            data)
{
    gpsd_msg_t *msg;
    msg = g_malloc(sizeof(gpsd_msg_t));

    msg->src = src;
    msg->type = type;
    msg->data = data;
    return msg;
}

static void
gpsd_msg_free(gpsd_msg_t *msg)
{
    switch(msg->type)
    {
        case GPSD_MSG_INFO:
            /* Nothing to free */
            break;

        case GPSD_MSG_DATA:
            gpsd_data_free(msg->data);
            break;
    }
    g_free(msg);
}

static gpsd_data_t*
gpsd_data_new()
{
    gpsd_data_t *data = g_malloc(sizeof(gpsd_data_t));

    data->device = NULL;
    data->mode = GPSD_MODE_INVALID;
    data->time = -1;
    data->ept = NAN;
    data->lat = NAN;
    data->lon = NAN;
    data->alt = NAN;
    data->epx = NAN;
    data->epy = NAN;
    data->epv = NAN;
    data->track = NAN;
    data->speed = NAN;
    data->climb = NAN;
    data->eps = NAN;
    data->epc = NAN;
    return data;
}

static void
gpsd_data_free(gpsd_data_t *data)
{
    if(data)
    {
        g_free(data->device);
        g_free(data);
    }
}

const gchar*
gpsd_data_get_device(const gpsd_data_t *data)
{
    static const gchar *unknown = "";
    return (data->device ? data->device : unknown);
}

gpsd_mode_t
gpsd_data_get_mode(const gpsd_data_t *data)
{
    return data->mode;
}

gint64
gpsd_data_get_time(const gpsd_data_t *data)
{
    return data->time;
}

gdouble
gpsd_data_get_ept(const gpsd_data_t *data)
{
    return data->ept;
}

gdouble
gpsd_data_get_lat(const gpsd_data_t *data)
{
    return data->lat;
}

gdouble
gpsd_data_get_lon(const gpsd_data_t *data)
{
    return data->lon;
}

gdouble
gpsd_data_get_alt(const gpsd_data_t *data)
{
    return data->alt;
}

gdouble
gpsd_data_get_epx(const gpsd_data_t *data)
{
    return data->epx;
}

gdouble
gpsd_data_get_epy(const gpsd_data_t *data)
{
    return data->epy;
}

gdouble
gpsd_data_get_epv(const gpsd_data_t *data)
{
    return data->epv;
}

gdouble
gpsd_data_get_track(const gpsd_data_t *data)
{
    return data->track;
}

gdouble
gpsd_data_get_speed(const gpsd_data_t *data)
{
    return data->speed;
}

gdouble
gpsd_data_get_climb(const gpsd_data_t *data)
{
    return data->climb;
}

gdouble
gpsd_data_get_eps(const gpsd_data_t *data)
{
    return data->eps;
}

gdouble
gpsd_data_get_epc(const gpsd_data_t *data)
{
    return data->epc;
}

static gboolean
gpsd_cb(gpointer user_data)
{
    gpsd_conn_t *context = (gpsd_conn_t*)user_data;
    context->cb(context);
    return FALSE;
}

static gboolean
gpsd_cb_msg(gpointer user_data)
{
    gpsd_msg_t *msg = (gpsd_msg_t*)user_data;

    if(!msg->src->canceled)
        msg->src->cb_msg(msg->src, msg->type, msg->data);

    gpsd_msg_free(msg);
    return FALSE;
}

static gpointer
gpsd_conn_thread(gpointer user_data)
{
    gpsd_conn_t *context = (gpsd_conn_t*)user_data;

#if DEBUG
    g_print("gpsd_conn@%p: thread start\n", (gpointer)context);
#endif

    do
    {
        if(gpsd_conn_open(context))
        {
            if(!gpsd_conn_read(context))
                g_idle_add(gpsd_cb_msg, gpsd_msg_new(context, GPSD_MSG_INFO, GINT_TO_POINTER(GPSD_INFO_ERR_MISMATCH)));

            g_idle_add(gpsd_cb_msg, gpsd_msg_new(context, GPSD_MSG_INFO, GINT_TO_POINTER(GPSD_INFO_DISCONNECTED)));
            gpsd_conn_close(context);
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
    g_print("gpsd_conn@%p: thread stop\n", (gpointer)context);
#endif
    g_idle_add(gpsd_cb, context);
    return NULL;
}

static gboolean
gpsd_conn_open(gpsd_conn_t *context)
{
    struct addrinfo hints;
    struct addrinfo *result;

#if DEBUG
    g_print("gpsd_conn@%p: connecting\n", (gpointer)context);
#endif

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    /* Resolve the hostname */
    g_idle_add(gpsd_cb_msg, gpsd_msg_new(context, GPSD_MSG_INFO, GINT_TO_POINTER(GPSD_INFO_RESOLVING)));
    if(getaddrinfo(context->hostname, context->port, &hints, &result))
    {
#if DEBUG
        g_print("gpsd_conn@%p: failed to resolve the hostname\n", (gpointer)context);
#endif
        g_idle_add(gpsd_cb_msg, gpsd_msg_new(context, GPSD_MSG_INFO, GINT_TO_POINTER(GPSD_INFO_ERR_RESOLV)));
        return FALSE;
    }

    if(context->canceled)
    {
        freeaddrinfo(result);
        return FALSE;
    }

    g_idle_add(gpsd_cb_msg, gpsd_msg_new(context, GPSD_MSG_INFO, GINT_TO_POINTER(GPSD_INFO_CONNECTING)));
    context->fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
#ifdef G_OS_WIN32
    if(context->fd == INVALID_SOCKET)
#else
    if(context->fd < 0)
#endif
    {
#if DEBUG
        g_print("gpsd_conn@%p: failed to create a socket\n", (gpointer)context);
#endif
        freeaddrinfo(result);
        g_idle_add(gpsd_cb_msg, gpsd_msg_new(context, GPSD_MSG_INFO, GINT_TO_POINTER(GPSD_INFO_ERR_CONN)));
        return FALSE;
    }

    if(connect(context->fd, result->ai_addr, result->ai_addrlen) < 0)
    {
#if DEBUG
        g_print("gpsd_conn@%p: failed to connect\n", (gpointer)context);
#endif
        gpsd_conn_close(context);
        freeaddrinfo(result);
        g_idle_add(gpsd_cb_msg, gpsd_msg_new(context, GPSD_MSG_INFO, GINT_TO_POINTER(GPSD_INFO_ERR_CONN)));
        return FALSE;
    }
    freeaddrinfo(result);

    if(context->canceled)
    {
        gpsd_conn_close(context);
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
    g_print("gpsd_conn@%p: connected\n", (gpointer)context);
#endif

    context->ts_connected = g_get_real_time();
    return TRUE;
}

static gboolean
gpsd_conn_read(gpsd_conn_t *context)
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
            g_print("gpsd_conn@%p: data timeout (%d sec)\n", (gpointer)context, GPSD_DATA_TIMEOUT_SEC);
#endif
            g_idle_add(gpsd_cb_msg, gpsd_msg_new(context, GPSD_MSG_INFO, GINT_TO_POINTER(GPSD_INFO_ERR_TIMEOUT)));
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
        len = recv(context->fd, ptr, GPSD_DATA_BUFFER_LEN- offset, 0);
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
                g_print("gpsd_conn@%p: read: %s\n", (gpointer) context, ptr);
#endif
                if(!gpsd_conn_parse(context, ptr))
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
                    g_print("gpsd_conn@%p: full buffer read: %s\n", (gpointer) context, ptr);
#endif
                    if(!gpsd_conn_parse(context, ptr))
                        return FALSE;
                    offset = 0;
                }
                else
                {
#if DEBUG_READ
                    g_print("gpsd_conn@%p: incomplete read (offset %zd)\n", (gpointer) context, offset);
#endif
                }
            }
        }
    }
    return TRUE;
}

static gboolean
gpsd_conn_write(gpsd_conn_t *context,
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
            g_print("gpsd_conn@%p: write failed: %s\n", (gpointer)context, msg);
#endif
            return FALSE;
        }
        sent += n;
    } while(sent < len);

#if (DEBUG && DEBUG_WRITE)
    g_print("gpsd_conn@%p: write: %s\n", (gpointer)context, msg);
#endif
    return TRUE;
}

static gboolean
gpsd_conn_close(gpsd_conn_t *context)
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
    g_print("gpsd_conn@%p: disconnected\n", (gpointer)context);
#endif
    return TRUE;
}

static gboolean
gpsd_conn_parse(gpsd_conn_t *context,
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
        g_print("gpsd_conn@%p: protocol mismatch: invalid message\n", (gpointer)context);
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
            g_print("gpsd_conn@%p: truncated message\n", (gpointer)context);
    }
#endif

    if(json.ready &&
       !context->ready)
    {
        context->ready = TRUE;
#if DEBUG
        g_print("gpsd_conn@%p: ready\n", (gpointer)context);
#endif
        g_idle_add(gpsd_cb_msg, gpsd_msg_new(context, GPSD_MSG_INFO, GINT_TO_POINTER(GPSD_INFO_CONNECTED)));

        /* Send the init string */
        if(!gpsd_conn_write(context, GPSD_INIT_STRING))
        {
#if DEBUG
            g_print("gpsd_conn@%p: failed to write GPSD_INIT_STRING\n", (gpointer)context);
#endif
            gpsd_data_free(json.data);
            return FALSE;
        }
    }

    if(!context->ready)
    {
#if DEBUG
        g_print("gpsd_conn@%p: protocol mismatch: expected VERSION class message as first\n", (gpointer)context);
#endif
        gpsd_data_free(json.data);
        return FALSE;
    }

    if(json.data)
    {
#if DEBUG
        g_print("gpsd_conn@%p: device=%s, mode=%s: time=%" G_GINT64_FORMAT " "
                "ept=%f lat=%f lon=%f alt=%f epx=%f epy=%f epv=%f "
                "track=%f speed=%f climb=%f eps=%f epc=%f\n",
                (gpointer)context,
                json.data->device,
                (json.data->mode == GPSD_MODE_NONE ? "none" :
                 (json.data->mode == GPSD_MODE_2D ? "2D" :
                  (json.data->mode == GPSD_MODE_3D ? "3D" : "unknown"))),
                json.data->time,
                json.data->ept,
                json.data->lat,
                json.data->lon,
                json.data->alt,
                json.data->epx,
                json.data->epy,
                json.data->epv,
                json.data->track,
                json.data->speed,
                json.data->climb,
                json.data->eps,
                json.data->epc);
#endif
        g_idle_add(gpsd_cb_msg, gpsd_msg_new(context, GPSD_MSG_DATA, json.data));
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
            json->data->mode = (gpsd_mode_t)value;
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
            json->data->ept = value;
        else if(json->key == GPSD_JSON_KEY_LAT)
            json->data->lat = value;
        else if(json->key == GPSD_JSON_KEY_LON)
            json->data->lon = value;
        else if(json->key == GPSD_JSON_KEY_ALT)
            json->data->alt = value;
        else if(json->key == GPSD_JSON_KEY_EPX)
            json->data->epx = value;
        else if(json->key == GPSD_JSON_KEY_EPY)
            json->data->epy = value;
        else if(json->key == GPSD_JSON_KEY_EPV)
            json->data->epv = value;
        else if(json->key == GPSD_JSON_KEY_TRACK)
            json->data->track = value;
        else if(json->key == GPSD_JSON_KEY_SPEED)
            json->data->speed = value;
        else if(json->key == GPSD_JSON_KEY_CLIMB)
            json->data->climb = value;
        else if(json->key == GPSD_JSON_KEY_EPS)
            json->data->eps = value;
        else if(json->key == GPSD_JSON_KEY_EPC)
            json->data->epc = value;
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
                json->data = gpsd_data_new();
        }
        else if(json->class == GPSD_JSON_CLASS_TPV)
        {
            g_assert(json->data);

            if(json->key == GPSD_JSON_KEY_DEVICE &&
               !json->data->device)
            {
                json->data->device = g_strndup((const gchar*)string, length);
            }
            else if(json->key == GPSD_JSON_KEY_TIME)
            {
                tmp = g_strndup((const gchar*)string, length);
                if(g_time_val_from_iso8601(tmp, &tv))
                    json->data->time = tv.tv_sec;
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
