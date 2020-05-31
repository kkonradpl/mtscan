/*
 *  MTscan - MikroTik RouterOS wireless scanner
 *  Copyright (c) 2015-2020  Konrad Kosmatka
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

#include <yajl/yajl_parse.h>
#include <yajl/yajl_gen.h>
#include <glib/gstdio.h>
#include <string.h>
#include <math.h>
#include <zlib.h>
#include <fcntl.h>
#include "model.h"
#include "ui.h"
#include "log.h"
#include "signals.h"
#include "misc.h"

#ifdef G_OS_WIN32
#include "win32.h"
#endif
#ifndef _O_BINARY
#define _O_BINARY 0
#endif

#define READ_BUFFER_LEN 100*1024

enum
{
    LEVEL_ROOT,
    LEVEL_OBJECT,
    LEVEL_NETWORK
};

#define KEY_UNKNOWN -1

static const gchar *const keys[] =
{
    "freq",
    "chan",
    "mode",
    "ss",
    "ssid",
    "name",
    "s",
    "priv",
    "ros",
    "ns",
    "tdma",
    "wds",
    "br",
    "airmax",
    "airmax-ac-ptp",
    "airmax-ac-ptmp",
    "airmax-ac-mixed",
    "first",
    "last",
    "lat",
    "lon",
    "azi",
    "signals"
};

enum
{
    KEY_FREQUENCY = KEY_UNKNOWN+1,
    KEY_CHANNEL,
    KEY_MODE,
    KEY_SPATIAL_STREAMS,
    KEY_SSID,
    KEY_RADIONAME,
    KEY_RSSI,
    KEY_PRIVACY,
    KEY_ROUTEROS,
    KEY_NSTREME,
    KEY_TDMA,
    KEY_WDS,
    KEY_BRIDGE,
    KEY_AIRMAX,
    KEY_AIRMAX_AC_PTP,
    KEY_AIRMAX_AC_PTMP,
    KEY_AIRMAX_AC_MIXED,
    KEY_FIRSTSEEN,
    KEY_LASTSEEN,
    KEY_LATITUDE,
    KEY_LONGITUDE,
    KEY_AZIMUTH,
    KEY_SIGNALS
};

static const gchar *const keys_signals[] =
{
    "t",
    "s",
    "lat",
    "lon",
    "azi"
};

enum
{
    KEY_SIGNALS_TIMESTAMP = KEY_UNKNOWN+1,
    KEY_SIGNALS_RSSI,
    KEY_SIGNALS_LATITUDE,
    KEY_SIGNALS_LONGITUDE,
    KEY_SIGNALS_AZIMUTH
};

typedef struct read_context
{
    void (*net_cb)(network_t*, gpointer);
    gpointer user_data;

    gint level;
    gint key;
    gboolean level_signals;
    gboolean strip_samples;
    network_t network;
    signals_node_t *signal;
    gint count;
} read_ctx_t;

typedef struct save_context
{
    gzFile gzfp;
    FILE *fp;
    yajl_gen gen;
    gboolean strip_signals;
    gboolean strip_gps;
    gboolean strip_azi;
    size_t wrote;
    size_t length;
} save_ctx_t;

static gint parse_integer(gpointer, long long int);
static gint parse_double(gpointer, double);
static gint parse_string(gpointer, const guchar*, size_t);
static gint parse_key(gpointer, const guchar*, size_t);
static gint parse_key_start(gpointer);
static gint parse_key_end(gpointer);
static gint parse_array_start(gpointer);
static gint parse_array_end(gpointer);
static gboolean log_save_foreach(GtkTreeModel*, GtkTreePath*, GtkTreeIter*, gpointer);
static gboolean log_save_write(save_ctx_t*);

static yajl_callbacks json_callbacks =
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


gint
log_read(const gchar  *filename,
         void        (*net_cb)(network_t*, gpointer),
         gpointer     user_data,
         gboolean     strip_samples)
{
    gzFile gzfp;
    gint n, err;
    guchar buffer[READ_BUFFER_LEN];
    read_ctx_t context;

    yajl_handle json;
    yajl_status status;

    context.net_cb = net_cb;
    context.user_data = user_data;
    context.strip_samples = strip_samples;

    gzfp = gzdopen(g_open(filename, O_RDONLY | _O_BINARY, 0), "r");

    if(!gzfp)
        return LOG_READ_ERROR_OPEN;

    status = yajl_status_ok;
    json = yajl_alloc(&json_callbacks, NULL, &context);
    err = 0;

    context.key = KEY_UNKNOWN;
    context.level = LEVEL_ROOT;
    context.level_signals = FALSE;
    context.signal = NULL;
    context.count = 0;
    network_init(&context.network);

    do
    {
        n = gzread(gzfp, buffer, READ_BUFFER_LEN-1);
        if(n <= 0)
        {
            gzerror(gzfp, &err);
            if(err)
            {
                context.count = LOG_READ_ERROR_READ;
                break;
            }
        }
        status = yajl_parse(json, buffer, (size_t)n);
    } while (!gzeof(gzfp) && status == yajl_status_ok);

    if(!err)
    {
        if(status == yajl_status_ok)
            status = yajl_complete_parse(json);
        if(status != yajl_status_ok)
            context.count = LOG_READ_ERROR_PARSE;
    }

    gzclose(gzfp);
    yajl_free(json);
    network_free(&context.network);
    g_free(context.signal);

    return context.count;
}

static gint
parse_integer(gpointer ptr,
              long long int value)
{
    read_ctx_t *ctx = (read_ctx_t*)ptr;
    if(ctx->level == LEVEL_NETWORK+1 &&
       ctx->level_signals &&
       ctx->signal)
    {
        if(ctx->key == KEY_SIGNALS_TIMESTAMP)
            ctx->signal->timestamp = value;
        else if(ctx->key == KEY_SIGNALS_RSSI)
            ctx->signal->rssi = value;
    }
    else if(ctx->level == LEVEL_NETWORK)
    {
        if(ctx->key == KEY_FREQUENCY)
            ctx->network.frequency = value*1000;
        if(ctx->key == KEY_SPATIAL_STREAMS)
            ctx->network.streams = value;
        else if(ctx->key == KEY_RSSI)
            ctx->network.rssi = value;
        else if(ctx->key == KEY_PRIVACY)
            ctx->network.flags.privacy = value;
        else if(ctx->key == KEY_ROUTEROS)
            ctx->network.flags.routeros = value;
        else if(ctx->key == KEY_NSTREME)
            ctx->network.flags.nstreme = value;
        else if(ctx->key == KEY_TDMA)
            ctx->network.flags.tdma = value;
        else if(ctx->key == KEY_WDS)
            ctx->network.flags.wds = value;
        else if(ctx->key == KEY_BRIDGE)
            ctx->network.flags.bridge = value;
        else if(ctx->key == KEY_AIRMAX)
            ctx->network.ubnt_airmax = value;
        else if(ctx->key == KEY_AIRMAX_AC_PTP)
            ctx->network.ubnt_ptp = value;
        else if(ctx->key == KEY_AIRMAX_AC_PTMP)
            ctx->network.ubnt_ptmp = value;
        else if(ctx->key == KEY_AIRMAX_AC_MIXED)
            ctx->network.ubnt_mixed = value;
        else if(ctx->key == KEY_FIRSTSEEN)
            ctx->network.firstseen = value;
        else if(ctx->key == KEY_LASTSEEN)
            ctx->network.lastseen = value;
    }

    return 1;
}

static gint
parse_double(gpointer ptr,
             double value)
{
    read_ctx_t *ctx = (read_ctx_t*)ptr;
    if(ctx->level == LEVEL_NETWORK+1 &&
       ctx->level_signals &&
       ctx->signal)
    {
        if(ctx->key == KEY_SIGNALS_LATITUDE)
            ctx->signal->latitude = value;
        else if(ctx->key == KEY_SIGNALS_LONGITUDE)
            ctx->signal->longitude = value;
        else if(ctx->key == KEY_SIGNALS_AZIMUTH)
            ctx->signal->azimuth = value;
    }
    else if(ctx->level == LEVEL_NETWORK)
    {
        if(ctx->key == KEY_FREQUENCY)
            ctx->network.frequency = lround(value*1000.0);
        else if(ctx->key == KEY_LATITUDE)
            ctx->network.latitude = value;
        else if(ctx->key == KEY_LONGITUDE)
            ctx->network.longitude = value;
        else if(ctx->key == KEY_AZIMUTH)
            ctx->network.azimuth = value;
    }
    return 1;
}

static gint
parse_string(gpointer      ptr,
             const guchar *string,
             size_t        length)
{
    read_ctx_t *ctx = (read_ctx_t*)ptr;
    if(ctx->level == LEVEL_NETWORK)
    {
        if(ctx->key == KEY_CHANNEL)
            ctx->network.channel = g_strndup((gchar*)string, length);
        else if(ctx->key == KEY_MODE)
            ctx->network.mode = g_strndup((gchar*)string, length);
        else if(ctx->key == KEY_SSID)
            ctx->network.ssid = g_strndup((gchar*)string, length);
        else if(ctx->key == KEY_RADIONAME)
            ctx->network.radioname = g_strndup((gchar*)string, length);
        else if(ctx->key == KEY_ROUTEROS)
        {
            ctx->network.flags.routeros = TRUE;
            ctx->network.routeros_ver = g_strndup((gchar*)string, length);
        }
    }
    return 1;
}

static gint
parse_key_start(gpointer ptr)
{
    read_ctx_t *ctx = (read_ctx_t*)ptr;
    ctx->level++;
    if(ctx->level == LEVEL_NETWORK+1 &&
       ctx->level_signals == TRUE &&
       ctx->network.address >= 0 &&
       !ctx->strip_samples)
    {
        ctx->signal = signals_node_new0();
    }
    return 1;
}

static gint
parse_key(gpointer      ptr,
          const guchar *string,
          size_t        length)
{
    read_ctx_t *ctx = (read_ctx_t*)ptr;
    gint i;
    ctx->key = KEY_UNKNOWN;

    if(ctx->level == LEVEL_NETWORK+1)
    {
        for(i=KEY_UNKNOWN+1; i<G_N_ELEMENTS(keys_signals); i++)
        {
            if(length == strlen(keys_signals[i]) &&
               !strncmp(keys_signals[i], (gchar*)string, length))
            {
                ctx->key = i;
                break;
            }
        }
    }
    else if(ctx->level == LEVEL_NETWORK)
    {
        for(i=KEY_UNKNOWN+1; i<G_N_ELEMENTS(keys); i++)
        {
            if(length == strlen(keys[i]) &&
               !strncmp(keys[i], (gchar*)string, length))
            {
                ctx->key = i;
                break;
            }
        }
    }
    else if(ctx->level == LEVEL_OBJECT)
    {
        network_init(&ctx->network);
        if(length == 12)
        {
            ctx->network.address = str_addr_to_gint64((gchar*)string, length);
            if(ctx->network.address >= 0)
                ctx->network.signals = signals_new();
        }
    }

    return 1;
}

static gint
parse_key_end(gpointer ptr)
{
    read_ctx_t *ctx = (read_ctx_t*)ptr;

    if(ctx->level == LEVEL_NETWORK+1 &&
       ctx->level_signals == TRUE)
    {
        /* Assume that all signal samples are sorted by timestamp */
        if(ctx->signal &&
           ctx->signal->timestamp &&
           ctx->network.signals)
        {
            signals_append(ctx->network.signals, ctx->signal);
        }
        else
            g_free(ctx->signal);
        ctx->signal = NULL;
    }
    else if(ctx->level == LEVEL_NETWORK)
    {
        if(ctx->network.address >= 0)
        {
            ctx->net_cb(&ctx->network, ctx->user_data);
            ctx->count++;
        }

        network_free_null(&ctx->network);
    }
    ctx->level--;
    return 1;
}

static gint
parse_array_start(gpointer ptr)
{
    read_ctx_t *ctx = (read_ctx_t*)ptr;
    if(ctx->level == LEVEL_NETWORK && ctx->key == KEY_SIGNALS)
        ctx->level_signals = TRUE;
    return 1;
}

static gint
parse_array_end(gpointer ptr)
{
    read_ctx_t *ctx = (read_ctx_t*)ptr;
    if(ctx->level == LEVEL_NETWORK && ctx->level_signals)
        ctx->level_signals = FALSE;
    return 1;
}

log_save_error_t*
log_save(gchar       *filename,
         gboolean     strip_signals,
         gboolean     strip_gps,
         gboolean     strip_azi,
         GList       *iterlist)
{
    save_ctx_t ctx;
    gchar *ext;
    GList *i;
    log_save_error_t *ret;
    gchar *tmp_name = NULL;

    /* If the file exists, rename it */
    if(g_file_test(filename, G_FILE_TEST_EXISTS))
    {
        tmp_name = g_strdup_printf("%s.old", filename);
        if(g_rename(filename, tmp_name) < 0)
        {
            g_free(tmp_name);
            tmp_name = NULL;
        }
    }

    ext = strrchr(filename, '.');
    ctx.fp = g_fopen(filename, "wb");

    if(!ctx.fp)
    {
        ret = g_malloc0(sizeof(log_save_error_t));
        ret->existing_file = GPOINTER_TO_INT(tmp_name);
        g_free(tmp_name);
        return ret;
    }

    ctx.gzfp = NULL;
    if((ext && !g_ascii_strcasecmp(ext, ".gz")))
    {
        ctx.gzfp = gzdopen(dup(fileno(ctx.fp)), "wb");
        if(!ctx.gzfp)
        {
            ret = g_malloc0(sizeof(log_save_error_t));
            ret->existing_file = GPOINTER_TO_INT(tmp_name);
            g_free(tmp_name);
            fclose(ctx.fp);
            return ret;
        }
    }

    ctx.wrote = 0;
    ctx.length = 0;

    ctx.gen = yajl_gen_alloc(NULL);
    ctx.strip_signals = strip_signals;
    ctx.strip_gps = strip_gps;
    ctx.strip_azi = strip_azi;
    //yajl_gen_config(ctx.gen, yajl_gen_beautify, 1);
    yajl_gen_map_open(ctx.gen);

    if(iterlist)
    {
        for(i=iterlist; i; i=i->next)
            log_save_foreach(GTK_TREE_MODEL(ui.model->store), NULL, (GtkTreeIter*)(i->data), &ctx);
    }
    else
    {
        gtk_tree_model_foreach(GTK_TREE_MODEL(ui.model->store), log_save_foreach, &ctx);
    }

    yajl_gen_map_close(ctx.gen);
    log_save_write(&ctx);
    yajl_gen_free(ctx.gen);

    if(ctx.gzfp)
        gzclose(ctx.gzfp);
#ifdef G_OS_WIN32
    win32_fsync(fileno(ctx.fp));
#else
    fsync(fileno(ctx.fp));
#endif
    fclose(ctx.fp);

    if(ctx.length != ctx.wrote)
    {
        ret = g_malloc(sizeof(log_save_error_t));
        ret->wrote = ctx.wrote;
        ret->length = ctx.length;
        ret->existing_file = GPOINTER_TO_INT(tmp_name);
        g_free(tmp_name);
        return ret;
    }

    return NULL;
}

static gboolean
log_save_foreach(GtkTreeModel *store,
                 GtkTreePath  *path,
                 GtkTreeIter  *iter,
                 gpointer      data)
{
    save_ctx_t *ctx = (save_ctx_t*)data;
    network_t net;
    signals_node_t *sample;
    const gchar *buffer;
    const gchar *address;

    gtk_tree_model_get(GTK_TREE_MODEL(ui.model->store), iter,
                       COL_ADDRESS, &net.address,
                       COL_FREQUENCY, &net.frequency,
                       COL_CHANNEL, &net.channel,
                       COL_MODE, &net.mode,
                       COL_STREAMS, &net.streams,
                       COL_SSID, &net.ssid,
                       COL_RADIONAME, &net.radioname,
                       COL_MAXRSSI, &net.rssi,
                       COL_PRIVACY, &net.flags.privacy,
                       COL_ROUTEROS, &net.flags.routeros,
                       COL_NSTREME, &net.flags.nstreme,
                       COL_TDMA, &net.flags.tdma,
                       COL_WDS, &net.flags.wds,
                       COL_BRIDGE, &net.flags.bridge,
                       COL_ROUTEROS_VER, &net.routeros_ver,
                       COL_AIRMAX, &net.ubnt_airmax,
                       COL_AIRMAX_AC_PTP, &net.ubnt_ptp,
                       COL_AIRMAX_AC_PTMP, &net.ubnt_ptmp,
                       COL_AIRMAX_AC_MIXED, &net.ubnt_mixed,
                       COL_FIRSTLOG, &net.firstseen,
                       COL_LASTLOG, &net.lastseen,
                       COL_LATITUDE, &net.latitude,
                       COL_LONGITUDE, &net.longitude,
                       COL_AZIMUTH, &net.azimuth,
                       COL_SIGNALS, &net.signals,
                       -1);

    address = model_format_address(net.address, FALSE);
    yajl_gen_string(ctx->gen, (guchar*)address, strlen(address));
    yajl_gen_map_open(ctx->gen);

    buffer = model_format_frequency(net.frequency);
    yajl_gen_string(ctx->gen, (guchar*)keys[KEY_FREQUENCY], strlen(keys[KEY_FREQUENCY]));
    yajl_gen_number(ctx->gen, buffer, strlen(buffer));

    yajl_gen_string(ctx->gen, (guchar*)keys[KEY_CHANNEL], strlen(keys[KEY_CHANNEL]));
    yajl_gen_string(ctx->gen, (guchar*)net.channel, strlen(net.channel));

    yajl_gen_string(ctx->gen, (guchar*)keys[KEY_MODE], strlen(keys[KEY_MODE]));
    yajl_gen_string(ctx->gen, (guchar*)net.mode, strlen(net.mode));

    if(net.streams)
    {
        yajl_gen_string(ctx->gen, (guchar*)keys[KEY_SPATIAL_STREAMS], strlen(keys[KEY_SPATIAL_STREAMS]));
        yajl_gen_integer(ctx->gen, net.streams);
    }

    yajl_gen_string(ctx->gen, (guchar*)keys[KEY_SSID], strlen(keys[KEY_SSID]));
    yajl_gen_string(ctx->gen, (guchar*)net.ssid, strlen(net.ssid));

    yajl_gen_string(ctx->gen, (guchar*)keys[KEY_RADIONAME], strlen(keys[KEY_RADIONAME]));
    yajl_gen_string(ctx->gen, (guchar*)net.radioname, strlen(net.radioname));

    yajl_gen_string(ctx->gen, (guchar*)keys[KEY_RSSI], strlen(keys[KEY_RSSI]));
    yajl_gen_integer(ctx->gen, net.rssi);

    yajl_gen_string(ctx->gen, (guchar*)keys[KEY_PRIVACY], strlen(keys[KEY_PRIVACY]));
    yajl_gen_integer(ctx->gen, net.flags.privacy);

    yajl_gen_string(ctx->gen, (guchar*)keys[KEY_ROUTEROS], strlen(keys[KEY_ROUTEROS]));
    if(net.routeros_ver && strlen(net.routeros_ver))
        yajl_gen_string(ctx->gen, (guchar*)net.routeros_ver, strlen(net.routeros_ver));
    else
        yajl_gen_integer(ctx->gen, net.flags.routeros);

    if(net.flags.routeros)
    {
        yajl_gen_string(ctx->gen, (guchar*)keys[KEY_NSTREME], strlen(keys[KEY_NSTREME]));
        yajl_gen_integer(ctx->gen, net.flags.nstreme);

        yajl_gen_string(ctx->gen, (guchar*)keys[KEY_TDMA], strlen(keys[KEY_TDMA]));
        yajl_gen_integer(ctx->gen, net.flags.tdma);

        yajl_gen_string(ctx->gen, (guchar*)keys[KEY_WDS], strlen(keys[KEY_WDS]));
        yajl_gen_integer(ctx->gen, net.flags.wds);

        yajl_gen_string(ctx->gen, (guchar*)keys[KEY_BRIDGE], strlen(keys[KEY_BRIDGE]));
        yajl_gen_integer(ctx->gen, net.flags.bridge);
    }

    yajl_gen_string(ctx->gen, (guchar*)keys[KEY_AIRMAX], strlen(keys[KEY_AIRMAX]));
    yajl_gen_integer(ctx->gen, net.ubnt_airmax);

    if(net.ubnt_airmax)
    {
        yajl_gen_string(ctx->gen, (guchar*)keys[KEY_AIRMAX_AC_PTP], strlen(keys[KEY_AIRMAX_AC_PTP]));
        yajl_gen_integer(ctx->gen, net.ubnt_ptp);

        yajl_gen_string(ctx->gen, (guchar*)keys[KEY_AIRMAX_AC_PTMP], strlen(keys[KEY_AIRMAX_AC_PTMP]));
        yajl_gen_integer(ctx->gen, net.ubnt_ptmp);

        yajl_gen_string(ctx->gen, (guchar*)keys[KEY_AIRMAX_AC_MIXED], strlen(keys[KEY_AIRMAX_AC_MIXED]));
        yajl_gen_integer(ctx->gen, net.ubnt_mixed);
    }

    yajl_gen_string(ctx->gen, (guchar*)keys[KEY_FIRSTSEEN], strlen(keys[KEY_FIRSTSEEN]));
    yajl_gen_integer(ctx->gen, net.firstseen);

    yajl_gen_string(ctx->gen, (guchar*)keys[KEY_LASTSEEN], strlen(keys[KEY_LASTSEEN]));
    yajl_gen_integer(ctx->gen, net.lastseen);

    if(!isnan(net.latitude) && !isnan(net.longitude) && !ctx->strip_gps)
    {
        buffer = model_format_gps(net.latitude, TRUE);
        yajl_gen_string(ctx->gen, (guchar*)keys[KEY_LATITUDE], strlen(keys[KEY_LATITUDE]));
        yajl_gen_number(ctx->gen, buffer, strlen(buffer));

        buffer = model_format_gps(net.longitude, TRUE);
        yajl_gen_string(ctx->gen, (guchar*)keys[KEY_LONGITUDE], strlen(keys[KEY_LONGITUDE]));
        yajl_gen_number(ctx->gen, buffer, strlen(buffer));
    }

    if(!isnan(net.azimuth) && !ctx->strip_azi)
    {
        buffer = model_format_azimuth(net.azimuth, TRUE);
        yajl_gen_string(ctx->gen, (guchar*)keys[KEY_AZIMUTH], strlen(keys[KEY_AZIMUTH]));
        yajl_gen_number(ctx->gen, buffer, strlen(buffer));
    }

    if(net.signals->head && !ctx->strip_signals)
    {
        yajl_gen_string(ctx->gen, (guchar*)keys[KEY_SIGNALS], strlen(keys[KEY_SIGNALS]));
        yajl_gen_array_open(ctx->gen);

        sample = net.signals->head;
        while(sample)
        {
            yajl_gen_map_open(ctx->gen);

            yajl_gen_string(ctx->gen, (guchar*)keys_signals[KEY_SIGNALS_TIMESTAMP], strlen(keys_signals[KEY_SIGNALS_TIMESTAMP]));
            yajl_gen_integer(ctx->gen, sample->timestamp);

            yajl_gen_string(ctx->gen, (guchar*)keys_signals[KEY_SIGNALS_RSSI], strlen(keys_signals[KEY_SIGNALS_RSSI]));
            yajl_gen_integer(ctx->gen, sample->rssi);

            if(!isnan(sample->latitude) && !isnan(sample->longitude) && !ctx->strip_gps)
            {
                buffer = model_format_gps(sample->latitude, TRUE);
                yajl_gen_string(ctx->gen, (guchar*)keys_signals[KEY_SIGNALS_LATITUDE], strlen(keys_signals[KEY_SIGNALS_LATITUDE]));
                yajl_gen_number(ctx->gen, buffer, strlen(buffer));

                buffer = model_format_gps(sample->longitude, TRUE);
                yajl_gen_string(ctx->gen, (guchar*)keys_signals[KEY_SIGNALS_LONGITUDE], strlen(keys_signals[KEY_SIGNALS_LONGITUDE]));
                yajl_gen_number(ctx->gen, buffer, strlen(buffer));
            }

            if(!isnan(sample->azimuth) && !ctx->strip_azi)
            {
                buffer = model_format_azimuth(sample->azimuth, TRUE);
                yajl_gen_string(ctx->gen, (guchar*)keys_signals[KEY_SIGNALS_AZIMUTH], strlen(keys_signals[KEY_SIGNALS_AZIMUTH]));
                yajl_gen_number(ctx->gen, buffer, strlen(buffer));
            }

            yajl_gen_map_close(ctx->gen);
            sample = sample->next;
        }

        yajl_gen_array_close(ctx->gen);
    }
    yajl_gen_map_close(ctx->gen);


    /* Signals are stored in GtkListStore just as pointer,
       so set it to NULL before freeing the struct */
    net.signals = NULL;
    network_free(&net);

    return !log_save_write(ctx);
}

static gboolean
log_save_write(save_ctx_t *ctx)
{
    const guchar *json_string;
    size_t json_length;
    size_t wrote;

    yajl_gen_get_buf(ctx->gen, &json_string, &json_length);

    if(ctx->gzfp)
        wrote = (size_t)gzwrite(ctx->gzfp, json_string, (gint)json_length);
    else
        wrote = fwrite(json_string, sizeof(gchar), json_length, ctx->fp);

    yajl_gen_clear(ctx->gen);

    ctx->wrote += wrote;
    ctx->length += json_length;
    return (json_length == wrote);
}
