#include <yajl/yajl_parse.h>
#include <yajl/yajl_gen.h>
#include <string.h>
#include <math.h>
#include <zlib.h>
#include "model.h"
#include "ui.h"
#include "ui-dialogs.h"
#include "ui-view.h"
#include "ui-icons.h"
#include "log.h"
#include "signals.h"

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
    "ssid",
    "name",
    "s",
    "priv",
    "ros",
    "ns",
    "tdma",
    "wds",
    "br",
    "first",
    "last",
    "lat",
    "lon",
    "signals"
};

enum
{
    KEY_FREQUENCY = KEY_UNKNOWN+1,
    KEY_CHANNEL,
    KEY_MODE,
    KEY_SSID,
    KEY_RADIONAME,
    KEY_RSSI,
    KEY_PRIVACY,
    KEY_ROUTEROS,
    KEY_NSTREME,
    KEY_TDMA,
    KEY_WDS,
    KEY_BRIDGE,
    KEY_FIRSTSEEN,
    KEY_LASTSEEN,
    KEY_LATITUDE,
    KEY_LONGITUDE,
    KEY_SIGNALS
};

static const gchar *const keys_signals[] =
{
    "t",
    "s",
    "lat",
    "lon"
};

enum
{
    KEY_SIGNALS_TIMESTAMP = KEY_UNKNOWN+1,
    KEY_SIGNALS_RSSI,
    KEY_SIGNALS_LATITUDE,
    KEY_SIGNALS_LONGITUDE
};

typedef struct read_context
{
    gint level;
    gint key;
    gboolean level_signals;
    gboolean merge;
    gboolean changed;
    network_t network;
    signals_node_t *signal;
} read_ctx_t;

typedef struct save_context
{
    yajl_gen gen;
    gboolean strip_signals;
    gboolean strip_gps;
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


void
log_open(GSList   *filenames,
         gboolean  merge)
{
    gchar *filename;
    gzFile gzfp;
    gint n, err;
    guchar buffer[READ_BUFFER_LEN];
    const gchar *err_string;

    yajl_handle json;
    yajl_status status;
    read_ctx_t context;

    if(!ui_can_discard_unsaved())
        return;

    context.merge = merge;
    context.changed = FALSE;

    if(!merge)
        ui_clear();

    ui_view_lock(ui.treeview);

    while(filenames != NULL)
    {
        filename = (gchar*)filenames->data;
        gzfp = gzopen(filename, "r");
        if(!gzfp)
        {
            ui_dialog(GTK_WINDOW(ui.window),
                      GTK_MESSAGE_ERROR,
                      "Error",
                      "Failed to open a file:\n%s", filename);
        }
        else
        {
            context.key = KEY_UNKNOWN;
            context.level = LEVEL_ROOT;
            context.level_signals = FALSE;
            context.signal = NULL;
            network_init(&context.network);

            json = yajl_alloc(&json_callbacks, NULL, &context);
            do
            {
                n = gzread(gzfp, buffer, READ_BUFFER_LEN-1);
                status = yajl_parse(json, buffer, n);

                if(n < READ_BUFFER_LEN-1)
                {
                    if(gzeof(gzfp))
                        break;
                    err_string = gzerror(gzfp, &err);
                    if(err)
                    {
                        ui_dialog(GTK_WINDOW(ui.window),
                                  GTK_MESSAGE_ERROR,
                                  "Error",
                                  "Failed to read a file:\n%s\n\n%s",
                                  filename, err_string);
                        break;
                    }
                }
            } while (status == yajl_status_ok);
            gzclose(gzfp);

            if(status == yajl_status_ok)
                status = yajl_complete_parse(json);

            yajl_free(json);

            if(status != yajl_status_ok)
            {
                network_free(&context.network);
                g_free(context.signal);
                ui_dialog(GTK_WINDOW(ui.window),
                          GTK_MESSAGE_ERROR,
                          "Error",
                          "The selected file is corrupted or is not a valid MTscan log:\n%s",
                          filename);
            }
            else if(!merge)
            {
                /* Take the allocated filename for the UI */
                ui_set_title(filename);
                filenames->data = NULL;
            }
        }
        filenames = filenames->next;
    }

    ui_view_unlock(ui.treeview);

    if(context.changed)
    {
        ui_status_update_networks();
        if(merge)
        {
            /* Set filename to NULL after merge */
            ui_set_title(NULL);
            ui_changed();
        }
    }
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
    }
    else if(ctx->level == LEVEL_NETWORK)
    {
        if(ctx->key == KEY_FREQUENCY)
            ctx->network.frequency = lround(value*1000.0);
        else if(ctx->key == KEY_LATITUDE)
            ctx->network.latitude = value;
        else if(ctx->key == KEY_LONGITUDE)
            ctx->network.longitude = value;
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
       ctx->network.address)
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
            ctx->network.address = g_strndup((gchar*)string, length);
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
        if(ctx->network.address)
        {
            mtscan_model_add(ui.model, &ctx->network, ctx->merge);
            ctx->changed = TRUE;
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

gboolean
log_save(gchar       *filename,
         gboolean     strip_signals,
         gboolean     strip_gps,
         GList       *iterlist)
{
    gzFile gzfp = NULL;
    FILE *fp = NULL;
    gchar *ext;
    gboolean compression;
    save_ctx_t ctx;
    const guchar *json_string;
    size_t json_length;
    gint wrote;
    GList *i;

    ext = strrchr(filename, '.');
    compression = (ext && !g_ascii_strcasecmp(ext, ".gz"));

    if(compression)
        gzfp = gzopen(filename, "wb");
    else
        fp = fopen(filename, "w");

    if(!gzfp && !fp)
    {
        ui_dialog(GTK_WINDOW(ui.window),
                  GTK_MESSAGE_ERROR,
                  "Error",
                  "Unable to save a file:\n%s", filename);
        return FALSE;
    }

    ctx.gen = yajl_gen_alloc(NULL);
    ctx.strip_signals = strip_signals;
    ctx.strip_gps = strip_gps;
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
    yajl_gen_get_buf(ctx.gen, &json_string, &json_length);

    if(compression)
    {
        wrote = gzwrite(gzfp, json_string, json_length);
        gzclose(gzfp);
    }
    else
    {
        wrote = fwrite(json_string, sizeof(gchar), json_length, fp);
        fclose(fp);
    }

    yajl_gen_free(ctx.gen);

    if(json_length != wrote)
    {
        ui_dialog(GTK_WINDOW(ui.window),
                  GTK_MESSAGE_ERROR,
                  "Error",
                  "Unable to save a file:\n%s\n\nWrote only %d of %d uncompressed bytes.",
                  filename, wrote, json_length);
        return FALSE;
    }
    return TRUE;
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

    gtk_tree_model_get(GTK_TREE_MODEL(ui.model->store), iter,
                       COL_ADDRESS, &net.address,
                       COL_FREQUENCY, &net.frequency,
                       COL_CHANNEL, &net.channel,
                       COL_MODE, &net.mode,
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
                       COL_FIRSTSEEN, &net.firstseen,
                       COL_LASTSEEN, &net.lastseen,
                       COL_LATITUDE, &net.latitude,
                       COL_LONGITUDE, &net.longitude,
                       COL_SIGNALS, &net.signals,
                       -1);

    yajl_gen_string(ctx->gen, (guchar*)net.address, strlen(net.address));
    yajl_gen_map_open(ctx->gen);

    buffer = model_format_frequency(net.frequency);
    yajl_gen_string(ctx->gen, (guchar*)keys[KEY_FREQUENCY], strlen(keys[KEY_FREQUENCY]));
    yajl_gen_number(ctx->gen, buffer, strlen(buffer));

    yajl_gen_string(ctx->gen, (guchar*)keys[KEY_CHANNEL], strlen(keys[KEY_CHANNEL]));
    yajl_gen_string(ctx->gen, (guchar*)net.channel, strlen(net.channel));

    yajl_gen_string(ctx->gen, (guchar*)keys[KEY_MODE], strlen(keys[KEY_MODE]));
    yajl_gen_string(ctx->gen, (guchar*)net.mode, strlen(net.mode));

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

    yajl_gen_string(ctx->gen, (guchar*)keys[KEY_NSTREME], strlen(keys[KEY_NSTREME]));
    yajl_gen_integer(ctx->gen, net.flags.nstreme);

    yajl_gen_string(ctx->gen, (guchar*)keys[KEY_TDMA], strlen(keys[KEY_TDMA]));
    yajl_gen_integer(ctx->gen, net.flags.tdma);

    yajl_gen_string(ctx->gen, (guchar*)keys[KEY_WDS], strlen(keys[KEY_WDS]));
    yajl_gen_integer(ctx->gen, net.flags.wds);

    yajl_gen_string(ctx->gen, (guchar*)keys[KEY_BRIDGE], strlen(keys[KEY_BRIDGE]));
    yajl_gen_integer(ctx->gen, net.flags.bridge);

    yajl_gen_string(ctx->gen, (guchar*)keys[KEY_FIRSTSEEN], strlen(keys[KEY_FIRSTSEEN]));
    yajl_gen_integer(ctx->gen, net.firstseen);

    yajl_gen_string(ctx->gen, (guchar*)keys[KEY_LASTSEEN], strlen(keys[KEY_LASTSEEN]));
    yajl_gen_integer(ctx->gen, net.lastseen);

    if(!isnan(net.latitude) && !isnan(net.longitude) && !ctx->strip_gps)
    {
        buffer = model_format_gps(net.latitude);
        yajl_gen_string(ctx->gen, (guchar*)keys[KEY_LATITUDE], strlen(keys[KEY_LATITUDE]));
        yajl_gen_number(ctx->gen, buffer, strlen(buffer));

        buffer = model_format_gps(net.longitude);
        yajl_gen_string(ctx->gen, (guchar*)keys[KEY_LONGITUDE], strlen(keys[KEY_LONGITUDE]));
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
                buffer = model_format_gps(sample->latitude);
                yajl_gen_string(ctx->gen, (guchar*)keys_signals[KEY_SIGNALS_LATITUDE], strlen(keys_signals[KEY_SIGNALS_LATITUDE]));
                yajl_gen_number(ctx->gen, buffer, strlen(buffer));

                buffer = model_format_gps(sample->longitude);
                yajl_gen_string(ctx->gen, (guchar*)keys_signals[KEY_SIGNALS_LONGITUDE], strlen(keys_signals[KEY_SIGNALS_LONGITUDE]));
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
    return FALSE;
}
