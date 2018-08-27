/*
 *  MTscan - MikroTik RouterOS wireless scanner
 *  Copyright (c) 2015-2017  Konrad Kosmatka
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>
#include <libssh/libssh.h>
#include "mt-ssh.h"

#ifdef G_OS_WIN32
#include "win32.h"
#endif // G_OS_WIN32

/* ---------------------------------------
   PTY_COLS 140 was fine up to v6.29.x,
   starting from v6.30.x increased to 150
   due to extended channel info.
   ---------------------------------------
   PTY_COLS 150 was fine up to v6.34.x
   starting from v6.30.x increased to 190
   due to included ROS version.
   --------------------------------------- */
#define PTY_COLS               190
#define PTY_ROWS               300
#define SOCKET_BUFFER  PTY_COLS*20
#define READ_TIMEOUT_MSEC      100

#define DEBUG    0
#define DUMP_PTY 0

#define SSID_LEN        32
#define RADIO_NAME_LEN  16
#define ROS_VERSION_LEN 16

#define MAC_ADDR_HEX_LEN 12

typedef struct mt_ssh_shdr
{
    guint address;
    guint ssid;
    guint channel;
    guint band;          /* pre v6.30 */
    guint channel_width; /* pre v6.30 */
    guint rssi;
    guint noise;
    guint snr;
    guint radioname;
    guint ros_version;
} mt_ssh_shdr_t;

typedef struct mt_ssh_cmd
{
    mt_ssh_cmd_type_t  type;
    gchar             *data;
} mt_ssh_cmd_t;

typedef struct mt_ssh_msg
{
    const mt_ssh_t    *src;
    mt_ssh_msg_type_t  type;
    gpointer           data;
} mt_ssh_msg_t;

typedef struct mt_ssh_info
{
    mt_ssh_info_type_t  type;
    gchar              *data;
} mt_ssh_info_t;

typedef struct mt_ssh_net
{
    gint64  timestamp;
    gint64  address;
    guint8  flags;
    gint    frequency;
    gchar  *channel;
    gchar  *mode;
    gchar  *ssid;
    gchar  *radioname;
    gint8   rssi;
    gint8   noise;
    gchar  *routeros_ver;
} mt_ssh_net_t;

typedef struct mt_ssh_snf
{
    gint processed_packets;
    gint memory_size;
    gint memory_saved_packets;
    gint memory_over_limit_packets;
    gint stream_dropped_packets;
    gint stream_sent_packets;
    gint real_file_limit;
    gint real_memory_limit;
} mt_ssh_snf_t;

typedef struct mt_ssh
{
    /* Configuration */
    mt_ssh_mode_t mode_default;
    gchar        *hostname;
    gchar        *port;
    gchar        *login;
    gchar        *password;
    gchar        *iface;
    gint          duration;
    gboolean      remote_mode;
    gboolean      background;

    /* Callback pointers */
    void (*cb)    (mt_ssh_t*, mt_ssh_ret_t, const gchar*);
    void (*cb_msg)(const mt_ssh_t*, mt_ssh_msg_type_t, gconstpointer);

    /* Command queue */
    GAsyncQueue *queue;

    /* Thread cancelation flag */
    volatile gboolean canceled;

    /* Return values */
    mt_ssh_ret_t  return_state;
    gchar        *return_error;

    /* Private data */
    ssh_channel    channel;
    gboolean       verify_auth;
    gchar         *identity;
    gint64         hwaddr;
    gchar         *str_prompt;
    gint           state;
    gboolean       sent_command;
    gboolean       dispatch_interface_check;
    gchar         *dispatch_scanlist_set;
    gboolean       dispatch_scanlist_get;
    gint           dispatch_mode;
    GString       *scanlist;
    mt_ssh_shdr_t *scan_header;
    gint           scan_line;
    gboolean       scan_too_long;
    mt_ssh_snf_t  *sniffer;
} mt_ssh_t;


enum
{
    MT_SSH_NET_FLAG_ACTIVE    = (1 << 0),
    MT_SSH_NET_FLAG_PRIVACY   = (1 << 1),
    MT_SSH_NET_FLAG_ROUTEROS  = (1 << 2),
    MT_SSH_NET_FLAG_NSTREME   = (1 << 3),
    MT_SSH_NET_FLAG_TDMA      = (1 << 4),
    MT_SSH_NET_FLAG_WDS       = (1 << 5),
    MT_SSH_NET_FLAG_BRIDGE    = (1 << 6)
};

enum
{
    MT_SSH_STATE_WAITING_FOR_PROMPT = 0,
    MT_SSH_STATE_WAITING_FOR_PROMPT_DIRTY,
    MT_SSH_STATE_PROMPT,
    MT_SSH_STATE_INTERFACE,
    MT_SSH_STATE_SCANLIST,
    MT_SSH_STATE_WAITING_FOR_SCAN,
    MT_SSH_STATE_SCANNING,
    MT_SSH_STATE_SNIFFING,
    MT_SSH_N_STATES
};

#if DEBUG
static const gchar *str_states[] =
{
    "STATE_WAITING_FOR_PROMPT",
    "STATE_WAITING_FOR_PROMPT_DIRTY",
    "STATE_PROMPT",
    "STATE_INTERFACE",
    "STATE_SCANLIST",
    "STATE_WAITING_FOR_SCAN",
    "STATE_SCANNING",
    "STATE_SNIFFING",
};
#endif

static const gchar str_prompt_start[] = "\x1b[9999B[";
static const gchar str_prompt_end[]   = "] > ";

static mt_ssh_cmd_t*  mt_ssh_cmd_new(mt_ssh_cmd_type_t, gchar*);
static void           mt_ssh_cmd_free(mt_ssh_cmd_t*);
static mt_ssh_msg_t*  mt_ssh_msg_new(const mt_ssh_t*, mt_ssh_msg_type_t, gpointer);
static void           mt_ssh_msg_free(mt_ssh_msg_t*);
static mt_ssh_info_t* mt_ssh_info_new(mt_ssh_info_type_t, gchar*);
static void           mt_ssh_info_free(mt_ssh_info_t*);
static mt_ssh_net_t*  mt_ssh_net_new();
static void           mt_ssh_net_free(mt_ssh_net_t*);
static mt_ssh_snf_t*  mt_ssh_snf_new();
static void           mt_ssh_snf_free(mt_ssh_snf_t*);

gboolean mt_ssh_cb(gpointer user_data);
gboolean mt_ssh_cb_msg(gpointer user_data);

static gpointer mt_ssh_thread(gpointer);
static gboolean mt_ssh_verify(mt_ssh_t*, ssh_session);

static void mt_ssh(mt_ssh_t*);
static void mt_ssh_request(mt_ssh_t*, const gchar*);
static void mt_ssh_set_state(mt_ssh_t*, gint);
static void mt_ssh_interface(mt_ssh_t*, gchar*);
static void mt_ssh_scanning(mt_ssh_t*, gchar*);
static void mt_ssh_sniffing(mt_ssh_t*, gchar*);
static void mt_ssh_commands(mt_ssh_t*, guint);
static void mt_ssh_dispatch(mt_ssh_t*);

static void mt_ssh_interface_check(mt_ssh_t*);
static void mt_ssh_scan(mt_ssh_t*);
static void mt_ssh_sniff(mt_ssh_t*);
static void mt_ssh_stop(mt_ssh_t*, gboolean);
static void mt_ssh_scanlist_get(mt_ssh_t*);
static void mt_ssh_scanlist_set(mt_ssh_t*, const gchar*);

static gint str_count_lines(const gchar*);
static void str_remove_char(gchar*, gchar);

static mt_ssh_shdr_t* parse_scan_header(const gchar*);
static guchar         parse_scan_flags(const gchar*, guint);
static gint64         parse_scan_address(const gchar*, guint);
static gint           parse_scan_channel(const gchar*, guint, gchar**, gchar**);
static gint           parse_scan_int(const gchar*, guint, guint);
static gchar*         parse_scan_string(const gchar*, guint, gint);
static gchar*         parse_scanlist(const gchar*);


mt_ssh_t*
mt_ssh_new(void         (*cb)(mt_ssh_t*, mt_ssh_ret_t, const gchar*),
           void         (*cb_msg)(const mt_ssh_t*, mt_ssh_msg_type_t, gconstpointer),
           mt_ssh_mode_t  mode_default,
           const gchar   *hostname,
           gint           port,
           const gchar   *login,
           const gchar   *password,
           const gchar   *iface,
           gint           duration,
           gboolean       remote,
           gboolean       background)
{
    mt_ssh_t *context;
    context = g_malloc0(sizeof(mt_ssh_t));

    /* Configuration */
    context->hostname = g_strdup(hostname);
    context->port = g_strdup_printf("%d", port);
    context->login = g_strdup(login);
    context->password = g_strdup(password);
    context->iface = g_strdup(iface);
    context->duration = duration;
    context->mode_default = mode_default;
    context->remote_mode = remote;
    context->background = background;

    /* Callback pointers */
    context->cb = cb;
    context->cb_msg = cb_msg;

    /* Command queue */
    context->queue = g_async_queue_new_full((GDestroyNotify)mt_ssh_cmd_free);

    /* Thread cancelation flag */
    context->canceled = FALSE;

    /* Return values */
    context->return_state = MT_SSH_INVALID;
    context->return_error = NULL;

    /* Private data */
    context->str_prompt = g_strdup_printf("%s%s@", str_prompt_start, login);
    context->state = MT_SSH_STATE_WAITING_FOR_PROMPT;
    context->dispatch_scanlist_get = TRUE;
    context->dispatch_mode = mode_default;
    context->dispatch_interface_check = TRUE;

    g_thread_unref(g_thread_new("mt_ssh_thread", mt_ssh_thread, context));
    return context;
}

void
mt_ssh_free(mt_ssh_t *context)
{
    /* Configuration */
    if(context)
    {
        g_free(context->hostname);
        g_free(context->port);
        g_free(context->login);
        g_free(context->password);
        g_free(context->iface);

        /* Command queue */
        g_async_queue_unref(context->queue);

        /* Return values */
        g_free(context->return_error);

        /* Private data */
        if(context->scanlist)
            g_string_free(context->scanlist, TRUE);
        g_free(context->dispatch_scanlist_set);
        g_free(context->str_prompt);
        g_free(context->identity);
        g_free(context->scan_header);
        mt_ssh_snf_free(context->sniffer);

        g_free(context);
    }
}

void
mt_ssh_cancel(mt_ssh_t *context)
{
    if(context)
        context->canceled = TRUE;
}

void
mt_ssh_cmd(mt_ssh_t          *context,
           mt_ssh_cmd_type_t  type,
           const gchar       *data)
{
    if(context)
        g_async_queue_push(context->queue, mt_ssh_cmd_new(type, g_strdup(data)));
}

const gchar*
mt_ssh_get_hostname(const mt_ssh_t *context)
{
    return context->hostname;
}

const gchar*
mt_ssh_get_port(const mt_ssh_t *context)
{
    return context->port;
}

const gchar*
mt_ssh_get_login(const mt_ssh_t *context)
{
    return context->login;
}

const gchar*
mt_ssh_get_password(const mt_ssh_t *context)
{
    return context->password;
}

const gchar*
mt_ssh_get_interface(const mt_ssh_t *context)
{
    return context->iface;
}

gint
mt_ssh_get_duration(const mt_ssh_t *context)
{
    return context->duration;
}

gboolean
mt_ssh_get_remote_mode(const mt_ssh_t *context)
{
    return context->remote_mode;
}

gboolean
mt_ssh_get_background(const mt_ssh_t *context)
{
    return context->background;
}

static mt_ssh_cmd_t*
mt_ssh_cmd_new(mt_ssh_cmd_type_t  type,
               gchar             *data)
{
    mt_ssh_cmd_t *cmd;
    cmd = g_malloc(sizeof(mt_ssh_cmd_t));

    cmd->type = type;
    cmd->data = data;
    return cmd;
}


static void
mt_ssh_cmd_free(mt_ssh_cmd_t *cmd)
{
    g_free(cmd->data);
    g_free(cmd);
}

static mt_ssh_msg_t*
mt_ssh_msg_new(const mt_ssh_t    *src,
               mt_ssh_msg_type_t  type,
               gpointer           data)
{
    mt_ssh_msg_t *msg;
    msg = g_malloc(sizeof(mt_ssh_msg_t));

    msg->src = src;
    msg->type = type;
    msg->data = data;
    return msg;
}


static void
mt_ssh_msg_free(mt_ssh_msg_t *msg)
{
    switch(msg->type)
    {
        case MT_SSH_MSG_INFO:
            mt_ssh_info_free(msg->data);
            break;

        case MT_SSH_MSG_NET:
            mt_ssh_net_free(msg->data);
            break;

        case MT_SSH_MSG_SNF:
            mt_ssh_snf_free(msg->data);
            break;
    }
    g_free(msg);
}

static mt_ssh_info_t*
mt_ssh_info_new(mt_ssh_info_type_t  type,
                gchar              *data)
{
    mt_ssh_info_t *info = g_malloc(sizeof(mt_ssh_info_t));

    info->type = type;
    info->data = data;
    return info;
}

static void
mt_ssh_info_free(mt_ssh_info_t *info)
{
    g_free(info->data);
    g_free(info);
}

mt_ssh_info_type_t
mt_ssh_info_get_type(const mt_ssh_info_t *info)
{
    return info->type;
}

const gchar*
mt_ssh_info_get_data(const mt_ssh_info_t *info)
{
    return info->data;
}

static mt_ssh_net_t*
mt_ssh_net_new()
{
    mt_ssh_net_t *net;
    net = g_malloc0(sizeof(mt_ssh_net_t));
    return net;
}

static void
mt_ssh_net_free(mt_ssh_net_t *net)
{
    g_free(net->radioname);
    g_free(net->ssid);
    g_free(net->channel);
    g_free(net->mode);
    g_free(net->routeros_ver);
    g_free(net);
}

gint64
mt_ssh_net_get_timestamp(const mt_ssh_net_t *net)
{
    return net->timestamp;
}

gint64
mt_ssh_net_get_address(const mt_ssh_net_t *net)
{
    return net->address;
}

gint
mt_ssh_net_get_frequency(const mt_ssh_net_t *net)
{
    return net->frequency;
}

const gchar*
mt_ssh_net_get_channel(const mt_ssh_net_t *net)
{
    return net->channel;
}

const gchar*
mt_ssh_net_get_mode(const mt_ssh_net_t *net)
{
    return net->mode;
}

const gchar*
mt_ssh_net_get_ssid(const mt_ssh_net_t *net)
{
    return net->ssid;
}

const gchar*
mt_ssh_net_get_radioname(const mt_ssh_net_t *net)
{
    return net->radioname;
}

gint8
mt_ssh_net_get_rssi(const mt_ssh_net_t *net)
{
    return net->rssi;
}

gint8
mt_ssh_net_get_noise(const mt_ssh_net_t *net)
{
    return net->noise;
}

const gchar*
mt_ssh_net_get_routeros_ver(const mt_ssh_net_t *net)
{
    return net->routeros_ver;
}

gboolean
mt_ssh_net_get_privacy(const mt_ssh_net_t *net)
{
    return net->flags & MT_SSH_NET_FLAG_PRIVACY;
}

gboolean
mt_ssh_net_get_routeros(const mt_ssh_net_t *net)
{
    return net->flags & MT_SSH_NET_FLAG_ROUTEROS;
}

gboolean
mt_ssh_net_get_nstreme(const mt_ssh_net_t *net)
{
    return net->flags & MT_SSH_NET_FLAG_NSTREME;
}

gboolean
mt_ssh_net_get_tdma(const mt_ssh_net_t *net)
{
    return net->flags & MT_SSH_NET_FLAG_TDMA;
}

gboolean
mt_ssh_net_get_wds(const mt_ssh_net_t *net)
{
    return net->flags & MT_SSH_NET_FLAG_WDS;
}

gboolean
mt_ssh_net_get_bridge(const mt_ssh_net_t *net)
{
    return net->flags & MT_SSH_NET_FLAG_BRIDGE;
}

static mt_ssh_snf_t*
mt_ssh_snf_new()
{
    mt_ssh_snf_t *snf = g_malloc0(sizeof(mt_ssh_snf_t));
    return snf;
}

static void
mt_ssh_snf_free(mt_ssh_snf_t *snf)
{
    g_free(snf);
}

gint
mt_ssh_snf_get_processed_packets(const mt_ssh_snf_t *snf)
{
    return snf->processed_packets;
}

gint
mt_ssh_snf_get_memory_size(const mt_ssh_snf_t *snf)
{
    return snf->memory_size;
}

gint
mt_ssh_snf_get_memory_saved_packets(const mt_ssh_snf_t *snf)
{
    return snf->memory_saved_packets;
}

gint
mt_ssh_snf_get_memory_over_limit_packets(const mt_ssh_snf_t *snf)
{
    return snf->memory_over_limit_packets;
}

gint
mt_ssh_snf_get_stream_dropped_packets(const mt_ssh_snf_t *snf)
{
    return snf->stream_dropped_packets;
}

gint
mt_ssh_snf_get_stream_sent_packets(const mt_ssh_snf_t *snf)
{
    return snf->stream_sent_packets;
}

gint
mt_ssh_snf_get_real_file_limit(const mt_ssh_snf_t *snf)
{
    return snf->real_file_limit;
}

gint
mt_ssh_snf_get_real_memory_limit(const mt_ssh_snf_t *snf)
{
    return snf->real_memory_limit;
}

gboolean
mt_ssh_cb(gpointer user_data)
{
    mt_ssh_t *context = (mt_ssh_t*)user_data;
    mt_ssh_ret_t return_state;
    const gchar *return_error;

    if(context->canceled &&
       context->return_state != MT_SSH_ERR_VERIFY &&
       context->return_state != MT_SSH_ERR_INTERFACE)
    {
        return_state = MT_SSH_CANCELED;
        return_error = NULL;
    }
    else
    {
        return_state = context->return_state;
        return_error = context->return_error;
    }

    context->cb(context, return_state, return_error);
    return FALSE;
}

gboolean
mt_ssh_cb_msg(gpointer user_data)
{
    mt_ssh_msg_t *msg = (mt_ssh_msg_t*)user_data;

    if(msg->src->cb_msg)
        msg->src->cb_msg(msg->src, msg->type, msg->data);

    mt_ssh_msg_free(msg);
    return FALSE;
}

gpointer
mt_ssh_thread(gpointer data)
{
    mt_ssh_t *context = (mt_ssh_t*)data;
    ssh_session session;
    ssh_channel channel;
    gchar *login;
    gint verbosity = (DEBUG ? SSH_LOG_PROTOCOL : SSH_LOG_WARNING);
    glong timeout = 30;

#if DEBUG
    printf("mt-ssh thread start: %p\n", (void*)context);
#endif

    if(!(session = ssh_new()))
    {
        context->return_state = MT_SSH_ERR_NEW;
        goto cleanup_callback;
    }

    if(ssh_options_set(session, SSH_OPTIONS_HOST, context->hostname) != SSH_OK ||
       ssh_options_set(session, SSH_OPTIONS_PORT_STR, context->port) != SSH_OK ||
       ssh_options_set(session, SSH_OPTIONS_TIMEOUT, &timeout) != SSH_OK ||
       ssh_options_set(session, SSH_OPTIONS_LOG_VERBOSITY, &verbosity) != SSH_OK)
    {
        context->return_state = MT_SSH_ERR_SET_OPTIONS;
        goto cleanup_free_ssh;
    }

    if(context->canceled)
        goto cleanup_free_ssh;

    g_idle_add(mt_ssh_cb_msg, mt_ssh_msg_new(context, MT_SSH_MSG_INFO, mt_ssh_info_new(MT_SSH_INFO_CONNECTING, NULL)));

    if(ssh_connect(session) != SSH_OK)
    {
        context->return_state = MT_SSH_ERR_CONNECT;
        context->return_error = g_strdup(ssh_get_error(session));
        goto cleanup_free_ssh;
    }

    if(context->canceled)
        goto cleanup_disconnect;

    if(!mt_ssh_verify(context, session))
    {
        context->return_state = MT_SSH_ERR_VERIFY;
        goto cleanup_disconnect;
    }

    if(context->canceled)
        goto cleanup_disconnect;

    g_idle_add(mt_ssh_cb_msg, mt_ssh_msg_new(context, MT_SSH_MSG_INFO, mt_ssh_info_new(MT_SSH_INFO_AUTHENTICATING, NULL)));

    /* Disable console colors with extra SSH login parameter:
     * https://forum.mikrotik.com/viewtopic.php?t=21234#p101304 */
    login = g_strdup_printf("%s+ct", context->login);
    if(ssh_userauth_password(session, login, context->password) != SSH_AUTH_SUCCESS)
    {
        g_free(login);
        context->return_state = MT_SSH_ERR_AUTH;
        goto cleanup_disconnect;
    }
    g_free(login);

    if(context->canceled)
        goto cleanup_disconnect;

    if(!(channel = ssh_channel_new(session)))
    {
        context->return_state = MT_SSH_ERR_CHANNEL_NEW;
        goto cleanup_disconnect;
    }

    if(ssh_channel_open_session(channel) != SSH_OK)
    {
        context->return_state = MT_SSH_ERR_CHANNEL_OPEN;
        goto cleanup_free_channel;
    }

    if(ssh_channel_request_pty_size(channel, "vt100", PTY_COLS, PTY_ROWS) != SSH_OK)
    {
        context->return_state = MT_SSH_ERR_CHANNEL_REQ_PTY_SIZE;
        goto cleanup_full;
    }

    if(ssh_channel_request_shell(channel) != SSH_OK)
    {
        context->return_state = MT_SSH_ERR_CHANNEL_REQ_SHELL;
        goto cleanup_full;
    }

    if(context->canceled)
        goto cleanup_full;

    g_idle_add(mt_ssh_cb_msg, mt_ssh_msg_new(context, MT_SSH_MSG_INFO, mt_ssh_info_new(MT_SSH_INFO_CONNECTED, NULL)));

    context->channel = channel;
    mt_ssh(context);

    if(!context->canceled)
        context->return_state = MT_SSH_CLOSED;

cleanup_full:
    ssh_channel_close(channel);
    ssh_channel_send_eof(channel);
cleanup_free_channel:
    ssh_channel_free(channel);
cleanup_disconnect:
    ssh_disconnect(session);
cleanup_free_ssh:
    ssh_free(session);
cleanup_callback:
#if DEBUG
    printf("mt-ssh thread stop %p\n", (void*)context);
#endif
    g_idle_add(mt_ssh_cb, context);
    return NULL;
}

static gboolean
mt_ssh_verify(mt_ssh_t    *context,
              ssh_session  session)
{
    guchar *hash;
    size_t hlen;
    gchar *hexa;
    gchar *message;
    gint state = ssh_is_server_known(session);

#if LIBSSH_VERSION_INT >= 0x0600
    ssh_key srv_pubkey;

#if LIBSSH_VERSION_INT <= 0x0705
    if(ssh_get_publickey(session, &srv_pubkey) < 0)
#else
    if(ssh_get_server_publickey(session, &srv_pubkey) < 0)
#endif
        return FALSE;

    if(ssh_get_publickey_hash(srv_pubkey, SSH_PUBLICKEY_HASH_SHA1, &hash, &hlen) < 0)
    {
        ssh_key_free(srv_pubkey);
        return FALSE;
    }
    ssh_key_free(srv_pubkey);
#else
    if((hlen = ssh_get_pubkey_hash(session, &hash) < 0))
        return FALSE;
#endif

    switch(state)
    {
    case SSH_SERVER_KNOWN_OK:
        break;

    case SSH_SERVER_KNOWN_CHANGED:
        hexa = ssh_get_hexa(hash, hlen);
        message = g_strdup_printf("Host key verification failed. Current key hash:\n%s\n"
                                  "For security reasons, the connection will be closed.",
                                  hexa);
        context->return_error = message;
        ssh_string_free_char(hexa);
        ssh_clean_pubkey_hash(&hash);
        return FALSE;

    case SSH_SERVER_FOUND_OTHER:
    case SSH_SERVER_FILE_NOT_FOUND:
    case SSH_SERVER_NOT_KNOWN:
        hexa = ssh_get_hexa(hash, hlen);
        message = g_strdup_printf("The authenticity of host '%s' can't be established.\n"
                                  "The key hash is:\n%s\n"
                                  "Are you sure you want to continue connecting?",
                                  context->hostname, hexa);

        g_idle_add(mt_ssh_cb_msg, mt_ssh_msg_new(context, MT_SSH_MSG_INFO, mt_ssh_info_new(MT_SSH_INFO_AUTH_VERIFY, message)));
        ssh_string_free_char(hexa);

        while(!context->verify_auth)
        {
            if(context->canceled)
            {
                ssh_clean_pubkey_hash(&hash);
                return FALSE;
            }
            mt_ssh_commands(context, 1000);
        }

        if(ssh_write_knownhost(session) < 0)
        {
            context->return_error = g_strdup_printf("Failed to add the host to the list of known hosts:\n%s", strerror(errno));
            ssh_clean_pubkey_hash(&hash);
            return FALSE;
        }
        break;

    case SSH_SERVER_ERROR:
        context->return_error = g_strdup(ssh_get_error(session));
        ssh_clean_pubkey_hash(&hash);
        return FALSE;
    }

    ssh_clean_pubkey_hash(&hash);
    return TRUE;
}

static void
mt_ssh(mt_ssh_t *context)
{
    gchar buffer[SOCKET_BUFFER+1];
    gchar *line, *ptr;
    gint n, i, lines;
    size_t length, offset = 0;
    struct timeval timeout;
    ssh_channel in_channels[2];

    while(ssh_channel_is_open(context->channel) &&
          !ssh_channel_is_eof(context->channel))
    {
        /* Handle commands from the queue */
        mt_ssh_commands(context, 0);
        mt_ssh_dispatch(context);

        if(context->canceled)
            return;

        timeout.tv_sec = 0;
        timeout.tv_usec = READ_TIMEOUT_MSEC * 1000;
        in_channels[0] = context->channel;
        in_channels[1] = NULL;
        if(ssh_channel_select(in_channels, NULL, NULL, &timeout) == SSH_EINTR)
            continue;
        if(ssh_channel_is_eof(context->channel))
            return;
        if(in_channels[0] == NULL)
            continue;

        n = ssh_channel_poll(context->channel, 0);
        if(n < 0)
            return;
        if(n == 0)
            continue;

        n = ssh_channel_read(context->channel, buffer+offset, sizeof(buffer)-1-offset, 0);
        if(n < 0)
            return;
        if(n == 0)
            continue;

        n += offset;
        buffer[n] = 0;
#if DUMP_PTY
        fprintf(stderr, "%s", buffer+offset);
        fflush(stderr);
#endif
        str_remove_char(buffer+offset, '\n');
        lines = str_count_lines(buffer);

        ptr = buffer;
        for(i=0; (line = strsep(&ptr, "\r")); i++)
        {
            length = strlen(line);

            if(!context->identity &&
               length > (strlen(context->str_prompt) + strlen(str_prompt_end)) &&
               !strncmp(line, context->str_prompt, strlen(context->str_prompt)) &&
               !strcmp(line+(length-strlen(str_prompt_end)), str_prompt_end))

            {
                /* Save the device identity from prompt */
                context->identity = g_strndup(line + strlen(context->str_prompt),
                                              length - strlen(context->str_prompt) - strlen(str_prompt_end));
#if DEBUG
                printf("[!] Identity: %s\n", context->identity);
#endif
                g_free(context->str_prompt);
                context->str_prompt = g_strdup_printf("%s%s@%s%s", str_prompt_start, context->login, context->identity, str_prompt_end);
                g_idle_add(mt_ssh_cb_msg, mt_ssh_msg_new(context, MT_SSH_MSG_INFO, mt_ssh_info_new(MT_SSH_INFO_IDENTITY, g_strdup(context->identity))));
            }

            if(context->identity &&
               context->state != MT_SSH_STATE_PROMPT &&
               !context->sent_command &&
               !strncmp(line, context->str_prompt, strlen(context->str_prompt)))
            {
                if(context->state == MT_SSH_STATE_WAITING_FOR_PROMPT)
                {
                    mt_ssh_set_state(context, MT_SSH_STATE_PROMPT);
                    mt_ssh_dispatch(context);
                }
                else
                {
                    mt_ssh_set_state(context, MT_SSH_STATE_WAITING_FOR_PROMPT);
                    ssh_channel_write(context->channel, "\03", 1); /* CTRL+C */
                }
                offset = 0;
#if DEBUG
                printf("[%d] %s\n", i, line);
#endif
                continue;
            }

            if(i == lines-1)
            {
                /* The last line may be incomplete.
                 * Set an offset and move it at the beginning of buffer … */
                if(lines > 1)
                {
                    for(i=0; i<length; i++)
                        buffer[i] = line[i];
                }
                offset = length;
                break;
            }

            if(context->sent_command)
            {
                /* The command is echoed back to terminal, ignore it */
                context->sent_command = FALSE;
                continue;
            }

#if DEBUG
            printf("[%d] %s\n", i, line);
#endif

            if(context->state == MT_SSH_STATE_INTERFACE)
            {
                mt_ssh_interface(context, line);
            }
            else if(context->state == MT_SSH_STATE_SCANLIST)
            {
                /* Buffer the scan-list */
                g_string_append(context->scanlist, line);
            }
            else if(context->state == MT_SSH_STATE_WAITING_FOR_SCAN ||
                    context->state == MT_SSH_STATE_SCANNING)
            {
                mt_ssh_scanning(context, line);
            }
            else if(context->state == MT_SSH_STATE_SNIFFING)
            {
                mt_ssh_sniffing(context, line);
            }
        }

        if(context->scan_too_long &&
           !context->remote_mode)
        {
            /* Restart scanning */
            context->scan_too_long = FALSE;
            mt_ssh_stop(context, TRUE);
        }
    }
}

static void
mt_ssh_request(mt_ssh_t    *context,
               const gchar *req)
{
    uint32_t len = (uint32_t)strlen(req);
    ssh_channel_write(context->channel, req, len);
    context->sent_command = TRUE;
}

static void
mt_ssh_set_state(mt_ssh_t *context,
                 gint      state)
{
    gchar *buffered;
    if(context->state == state)
        return;

#if DEBUG
    g_assert(state >= 0 && state < MT_SSH_N_STATES);
    printf("[!] Changing state from %s (%d) to %s (%d)\n",
           str_states[context->state], context->state,
           str_states[state], state);
#endif

    if(context->state == MT_SSH_STATE_INTERFACE &&
       context->hwaddr < 0)
    {
        context->return_state = MT_SSH_ERR_INTERFACE;
        context->canceled = TRUE;
        return;
    }

    if(context->state == MT_SSH_STATE_SCANLIST)
    {
        buffered = g_string_free(context->scanlist, FALSE);
        context->scanlist = NULL;
        g_idle_add(mt_ssh_cb_msg, mt_ssh_msg_new(context, MT_SSH_MSG_INFO, mt_ssh_info_new(MT_SSH_INFO_SCANLIST, parse_scanlist(buffered))));
        g_free(buffered);
    }

    if(state == MT_SSH_STATE_SCANNING)
    {
        g_idle_add(mt_ssh_cb_msg, mt_ssh_msg_new(context, MT_SSH_MSG_INFO, mt_ssh_info_new(MT_SSH_INFO_SCANNER_START, NULL)));
    }

    if(context->state == MT_SSH_STATE_SCANNING &&
       !context->remote_mode)
    {
        g_idle_add(mt_ssh_cb_msg, mt_ssh_msg_new(context, MT_SSH_MSG_INFO, mt_ssh_info_new(MT_SSH_INFO_SCANNER_STOP, NULL)));
    }

    if(state == MT_SSH_STATE_SNIFFING)
    {
        context->sniffer = mt_ssh_snf_new();
        g_idle_add(mt_ssh_cb_msg, mt_ssh_msg_new(context, MT_SSH_MSG_INFO, mt_ssh_info_new(MT_SSH_INFO_SNIFFER_START, NULL)));
    }

    if(context->state == MT_SSH_STATE_SNIFFING)
    {
        g_free(context->sniffer);
        context->sniffer = NULL;
        g_idle_add(mt_ssh_cb_msg, mt_ssh_msg_new(context, MT_SSH_MSG_INFO, mt_ssh_info_new(MT_SSH_INFO_SNIFFER_STOP, NULL)));
    }

    context->state = state;
}

static void
mt_ssh_interface(mt_ssh_t *context,
                 gchar    *line)
{
    gchar* data;

    if((context->hwaddr = parse_scan_address(line, 0)) >= 0)
    {
        data = g_strdup_printf("%012" G_GINT64_MODIFIER "X", context->hwaddr);
        g_idle_add(mt_ssh_cb_msg, mt_ssh_msg_new(context, MT_SSH_MSG_INFO, mt_ssh_info_new(MT_SSH_INFO_INTERFACE, data)));
        mt_ssh_set_state(context, MT_SSH_STATE_WAITING_FOR_PROMPT);
    }
}

static void
mt_ssh_scanning(mt_ssh_t *context,
                gchar    *line)
{
    static const gchar str_badcommand[]     = "expected end of command";
    static const gchar str_nosuchitem[]     = "no such item";
    static const gchar str_failure[]        = "failure:";
    static const gchar str_scanlistempty[]  = "scanlist empty";
    static const gchar str_scannotrunning[] = "scan not running";
    static const gchar str_scanstart[]      = "Flags: ";
    static const gchar str_scanend[]        = "-- ";

    mt_ssh_net_t *net;
    guchar flags;
    gint64 address;
    gchar *ptr;

    if(context->state == MT_SSH_STATE_WAITING_FOR_SCAN &&
       (!strncmp(line, str_failure, strlen(str_failure)) ||
        !strncmp(line, str_badcommand, strlen(str_badcommand)) ||
        !strncmp(line, str_nosuchitem, strlen(str_nosuchitem))))
    {
        /* Handle possible failures */
        mt_ssh_set_state(context, MT_SSH_STATE_WAITING_FOR_PROMPT_DIRTY);
        g_idle_add(mt_ssh_cb_msg, mt_ssh_msg_new(context, MT_SSH_MSG_INFO, mt_ssh_info_new(MT_SSH_INFO_FAILURE, g_strdup(line))));
        return;
    }

    if(!strncmp(line, str_scanlistempty, strlen(str_failure)))
    {
        g_free(context->dispatch_scanlist_set);
        context->dispatch_scanlist_set = g_strdup("default");
        mt_ssh_set_state(context, MT_SSH_STATE_WAITING_FOR_PROMPT_DIRTY);
        ptr = g_strdup("Scan-list is empty, reverting to default.\n" \
                       "You may need to reboot your device before starting the scan again (bug in RouterOS).");
        g_idle_add(mt_ssh_cb_msg, mt_ssh_msg_new(context, MT_SSH_MSG_INFO, mt_ssh_info_new(MT_SSH_INFO_FAILURE, ptr)));
        return;
    }

    if(!strncmp(line, str_scannotrunning, strlen(str_scannotrunning)))
    {
        /* Scanning stops when SSH connection is interrupted for a while */
        mt_ssh_set_state(context, MT_SSH_STATE_WAITING_FOR_PROMPT_DIRTY);
        if(context->dispatch_mode == MT_SSH_MODE_NONE)
            context->dispatch_mode = MT_SSH_MODE_SCANNER;
        return;
    }

    if(context->state == MT_SSH_STATE_SCANNING &&
       !strncmp(line, str_scanend, strlen(str_scanend)))
    {
        /* This is the last line of scan results */
        if(context->scan_line > PTY_ROWS/2)
        {
            context->scan_too_long = TRUE;
#if DEBUG
            printf("<!> Scan results are getting too long, scheduling restart\n");
#endif
        }
        context->scan_line = -1;
        g_idle_add(mt_ssh_cb_msg, mt_ssh_msg_new(context, MT_SSH_MSG_INFO, mt_ssh_info_new(MT_SSH_INFO_HEARTBEAT, NULL)));
#if DEBUG
        printf("<!> Found END of a scan frame\n");
#endif
        return;
    }

    if(context->scan_line < 0)
    {
        if(strstr(line, str_scanstart))
        {
            /* This is the first line of scan results */
            context->scan_line = 0;
            mt_ssh_set_state(context, MT_SSH_STATE_SCANNING);
#if DEBUG
            printf("<!> Found START of a scan frame\n");
#endif
        }
        return;
    }

    if(++context->scan_line == 1)
    {
        /* Second line of scan results should contain a header */
        if(!context->scan_header)
        {
            /* Try to parse the header */
            context->scan_header = parse_scan_header(line);
#if DEBUG
            if(context->scan_header)
                printf("<!> Found header. Address index = %d\n", context->scan_header->address);
#endif
        }
        return;
    }

    if(!context->scan_header)
    {
#if DEBUG
        printf("<!> ERROR: No scan header found!\n");
#endif
        return;
    }

    /* Ignore everything after any escape sequence */
    if((ptr = strchr(line, '\x1b')))
        *ptr = '\0';

    /* Parse all flags at the beginning of a line */
    flags = parse_scan_flags(line, context->scan_header->address-1);

    if(!(flags & MT_SSH_NET_FLAG_ACTIVE))
    {
        /* This network is not active at the moment, ignore it */
        return;
    }

    if((address = parse_scan_address(line, context->scan_header->address)) < 0)
    {
        /* This line doesn't contain a valid MAC address */
#if DEBUG
        printf("<!> ERROR: Invalid MAC address!\n");
#endif
        return;
    }

    net = mt_ssh_net_new();
    net->timestamp = g_get_real_time() / 1000000;
    net->address = address;
    net->flags = flags;

    if(context->scan_header->ssid)
        net->ssid = parse_scan_string(line, context->scan_header->ssid, SSID_LEN);

    if(context->scan_header->channel)
        net->frequency = parse_scan_channel(line, context->scan_header->channel, &net->channel, &net->mode);

    if(context->scan_header->channel_width && !net->channel) /* Pre v6.30 */
        net->channel = g_strdup_printf("%d", parse_scan_int(line, context->scan_header->channel_width, 1));

    if(context->scan_header->rssi)
        net->rssi = (gint8)parse_scan_int(line, context->scan_header->rssi, 3);

    if(context->scan_header->noise > 2)
        net->noise = (gint8)parse_scan_int(line, context->scan_header->noise-2, 4); // __NF !

    if(context->scan_header->radioname)
        net->radioname = parse_scan_string(line, context->scan_header->radioname, RADIO_NAME_LEN);

    if(context->scan_header->ros_version)
        net->routeros_ver = parse_scan_string(line, context->scan_header->ros_version, ROS_VERSION_LEN);

    g_idle_add(mt_ssh_cb_msg, mt_ssh_msg_new(context, MT_SSH_MSG_NET, net));
}

static void
mt_ssh_sniffing(mt_ssh_t *context,
                gchar    *line)
{
    static const gchar str_badcommand[]     = "expected end of command";
    static const gchar str_nosuchitem[]     = "input does not match any value of interface";
    static const gchar str_failure[]        = "failure:";
    static const gchar str_badchannel[]     = "bad band and/or channel";
    static const gchar str_notrunning[]     = "sniffer not running";
    static const gchar str_sniffend[]       = "-- ";
    static const gchar str_processed[]      = "processed-packets: ";
    static const gchar str_memory_size[]    = "memory-size: ";
    static const gchar str_memory_saved[]   = "memory-saved-packets: ";
    static const gchar str_memory_over[]    = "memory-over-limit-packets: ";
    static const gchar str_stream_dropped[] = "stream-dropped-packets: ";
    static const gchar str_stream_sent[]    = "stream-sent-packets: ";
    static const gchar str_file_limit[]     = "real-file-limit: ";
    static const gchar str_memory_limit[]   = "real-memory-limit: ";
    const gchar *ptr;

    if(!strncmp(line, str_failure, strlen(str_failure)) ||
       !strncmp(line, str_badcommand, strlen(str_badcommand)) ||
       !strncmp(line, str_nosuchitem, strlen(str_nosuchitem)) ||
       !strncmp(line, str_badchannel, strlen(str_badchannel)))
    {
        /* Handle possible failures */
        mt_ssh_set_state(context, MT_SSH_STATE_WAITING_FOR_PROMPT_DIRTY);
        g_idle_add(mt_ssh_cb_msg, mt_ssh_msg_new(context, MT_SSH_MSG_INFO, mt_ssh_info_new(MT_SSH_INFO_FAILURE, g_strdup(line))));
        return;
    }

    if(!strncmp(line, str_notrunning, strlen(str_notrunning)))
    {
        /* Sniffer stops when SSH connection is interrupted for a while */
        mt_ssh_set_state(context, MT_SSH_STATE_WAITING_FOR_PROMPT_DIRTY);
        if(context->dispatch_mode == MT_SSH_MODE_NONE)
            context->dispatch_mode = MT_SSH_MODE_SNIFFER;
        return;
    }

    if(!strncmp(line, str_sniffend, strlen(str_sniffend)))
    {
        g_idle_add(mt_ssh_cb_msg, mt_ssh_msg_new(context, MT_SSH_MSG_SNF, context->sniffer));
        context->sniffer = mt_ssh_snf_new();
        g_idle_add(mt_ssh_cb_msg, mt_ssh_msg_new(context, MT_SSH_MSG_INFO, mt_ssh_info_new(MT_SSH_INFO_HEARTBEAT, NULL)));
#if DEBUG
        printf("<!> Found END of a sniffer frame\n");
#endif
        return;
    }

    if((ptr = strstr(line, str_processed)))
        context->sniffer->processed_packets = atoi(ptr + strlen(str_processed));
    else if((ptr = strstr(line, str_memory_size)))
        context->sniffer->memory_size = atoi(ptr + strlen(str_memory_size));
    else if((ptr = strstr(line, str_memory_saved)))
        context->sniffer->memory_saved_packets = atoi(ptr + strlen(str_memory_saved));
    else if((ptr = strstr(line, str_memory_over)))
        context->sniffer->memory_over_limit_packets = atoi(ptr + strlen(str_memory_over));
    else if((ptr = strstr(line, str_stream_dropped)))
        context->sniffer->stream_dropped_packets = atoi(ptr + strlen(str_stream_dropped));
    else if((ptr = strstr(line, str_stream_sent)))
        context->sniffer->stream_sent_packets = atoi(ptr + strlen(str_stream_sent));
    else if((ptr = strstr(line, str_file_limit)))
        context->sniffer->real_file_limit = atoi(ptr + strlen(str_file_limit));
    else if((ptr = strstr(line, str_memory_limit)))
        context->sniffer->real_memory_limit = atoi(ptr + strlen(str_memory_limit));
}

static void
mt_ssh_commands(mt_ssh_t *context,
                guint     timeout)
{
    mt_ssh_cmd_t *cmd;

    while((cmd = g_async_queue_timeout_pop(context->queue, timeout)))
    {
        switch(cmd->type)
        {
            case MT_SSH_CMD_AUTH:
                context->verify_auth = TRUE;
                break;

            case MT_SSH_CMD_SCANLIST:
                /* Clear existing request */
                g_free(context->dispatch_scanlist_set);
                context->dispatch_scanlist_set = g_strdup(cmd->data);
                /* Always check new scanlist */
                context->dispatch_scanlist_get = TRUE;
                mt_ssh_stop(context, TRUE);
                break;

            case MT_SSH_CMD_STOP:
                context->dispatch_mode = MT_SSH_MODE_NONE;
                context->remote_mode = FALSE;
                mt_ssh_stop(context, FALSE);
                break;

            case MT_SSH_CMD_SCAN:
                context->dispatch_mode = MT_SSH_MODE_SCANNER;
                context->duration = (cmd->data ? atoi(cmd->data) : 0);
                break;

            case MT_SSH_CMD_SNIFF:
                context->dispatch_mode = MT_SSH_MODE_SNIFFER;
                break;
        }
        mt_ssh_cmd_free(cmd);
    }
}
static void
mt_ssh_dispatch(mt_ssh_t *context)
{
    if(context->state != MT_SSH_STATE_PROMPT)
        return;

    if(context->dispatch_interface_check)
    {
        mt_ssh_interface_check(context);
        context->dispatch_interface_check = FALSE;
    }
    else if(context->dispatch_scanlist_set)
    {
        mt_ssh_scanlist_set(context, context->dispatch_scanlist_set);
        g_free(context->dispatch_scanlist_set);
        context->dispatch_scanlist_set = NULL;
    }
    else if(context->dispatch_scanlist_get)
    {
        mt_ssh_scanlist_get(context);
        context->dispatch_scanlist_get = FALSE;
    }
    else if(context->dispatch_mode == MT_SSH_MODE_SCANNER ||
            context->remote_mode)
    {
        mt_ssh_scan(context);
        context->dispatch_mode = MT_SSH_MODE_NONE;
    }
    else if(context->dispatch_mode == MT_SSH_MODE_SNIFFER)
    {
        mt_ssh_sniff(context);
        context->dispatch_mode = MT_SSH_MODE_NONE;
    }
}

static void
mt_ssh_interface_check(mt_ssh_t *context)
{
    gchar *str;

    str = g_strdup_printf(" :put [/interface wireless get \"%s\" mac-address ]\r", context->iface);
    mt_ssh_request(context, str);
    g_free(str);

    mt_ssh_set_state(context, MT_SSH_STATE_INTERFACE);
}

static void
mt_ssh_scan(mt_ssh_t *context)
{
    gchar *str_scan;

    if(context->background && context->duration)
        str_scan = g_strdup_printf(":delay 250ms; /interface wireless scan \"%s\" background=yes duration=%d\r", context->iface, context->duration);
    else if(context->background)
        str_scan = g_strdup_printf(":delay 250ms; /interface wireless scan \"%s\" background=yes\r", context->iface);
    else if(context->duration)
        str_scan = g_strdup_printf("/interface wireless scan \"%s\" duration=%d\r", context->iface, context->duration);
    else
        str_scan = g_strdup_printf("/interface wireless scan \"%s\"\r", context->iface);

    mt_ssh_request(context, str_scan);
    g_free(str_scan);

    context->scan_line = -1;
    mt_ssh_set_state(context, MT_SSH_STATE_WAITING_FOR_SCAN);
}

static void
mt_ssh_sniff(mt_ssh_t *context)
{
    gchar *str_sniff;
    str_sniff = g_strdup_printf("/interface wireless sniffer sniff \"%s\"\r", context->iface);
    mt_ssh_request(context, str_sniff);
    g_free(str_sniff);

    mt_ssh_set_state(context, MT_SSH_STATE_SNIFFING);
}

static void
mt_ssh_stop(mt_ssh_t *context,
            gboolean  restart)
{
    if(context->state != MT_SSH_STATE_WAITING_FOR_SCAN &&
       context->state != MT_SSH_STATE_SCANNING &&
       context->state != MT_SSH_STATE_SNIFFING)
        return;

    /* Do not use mt_ssh_request, as the 'Q' is not echoed back */
    ssh_channel_write(context->channel, "Q", 1);

    if(restart)
    {
        if(context->state == MT_SSH_STATE_SNIFFING)
            context->dispatch_mode = MT_SSH_MODE_SNIFFER;
        else
            context->dispatch_mode = MT_SSH_MODE_SCANNER;
    }

    mt_ssh_set_state(context, MT_SSH_STATE_WAITING_FOR_PROMPT_DIRTY);
}

static void
mt_ssh_scanlist_get(mt_ssh_t *context)
{
    gchar *str_scanlist_get;

    str_scanlist_get = g_strdup_printf("/interface wireless info scan-list \"%s\"\r", context->iface);
    mt_ssh_request(context, str_scanlist_get);
    g_free(str_scanlist_get);
    context->scanlist = g_string_new(NULL);
    mt_ssh_set_state(context, MT_SSH_STATE_SCANLIST);
}

static void
mt_ssh_scanlist_set(mt_ssh_t    *context,
                    const gchar *scanlist)
{
    gchar *str_scanlist_set;

    str_scanlist_set = g_strdup_printf("/interface wireless set \"%s\" scan-list=\"%s\"\r", context->iface, scanlist);
    mt_ssh_request(context, str_scanlist_set);
    g_free(str_scanlist_set);
    mt_ssh_set_state(context, MT_SSH_STATE_WAITING_FOR_PROMPT);
}

static gint
str_count_lines(const gchar *ptr)
{
    gint i;
    for(i=1; (ptr = strchr(ptr, '\r')); i++)
        ptr++;
    return i;
}

static void
str_remove_char(gchar *str,
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

static mt_ssh_shdr_t*
parse_scan_header(const gchar *buff)
{
    mt_ssh_shdr_t *p;
    gchar *ptr;

    if(!(ptr = strstr(buff, "ADDRESS")))
    {
#if DEBUG
        printf("ADDRESS position not found!");
#endif
        return NULL;
    }

    p = g_malloc0(sizeof(mt_ssh_shdr_t));
    p->address = (guint)(ptr-buff);

    if((ptr = strstr(buff, "SSID")))
        p->ssid = (guint)(ptr-buff);

    /* Pre v6.30 */
    if((ptr = strstr(buff, "BAND")))
        p->band = (guint)(ptr-buff);

    if((ptr = strstr(buff, "CHANNEL-WIDTH")))
        p->channel_width = (guint)(ptr-buff);

    if((ptr = strstr(buff, "FREQ")))
        p->channel = (guint)(ptr-buff);
    /* --------- */

    /* Post v6.30 */
    if(!p->channel && (ptr = strstr(buff, "CHANNEL")))
        p->channel = (guint)(ptr-buff);
    /* ---------- */

    if((ptr = strstr(buff, "SIG")))
        p->rssi = (guint)(ptr-buff);

    if((ptr = strstr(buff, "NF")))
        p->noise = (guint)(ptr-buff);

    if((ptr = strstr(buff, "SNR")))
        p->snr = (guint)(ptr-buff);

    if((ptr = strstr(buff, "RADIO-NAME")))
        p->radioname = (guint)(ptr-buff);

    if((ptr = strstr(buff, "ROUTEROS-VERSION")))
        p->ros_version = (guint)(ptr-buff);

    return p;
}

static guchar
parse_scan_flags(const gchar *buff,
                 guint        flags_len)
{
    size_t length = MIN(strlen(buff), flags_len);
    guchar flags = 0;
    gint i;

    for(i=0; i<length; i++)
    {
        switch(buff[i])
        {
        case 'A':
            flags |= MT_SSH_NET_FLAG_ACTIVE;
            break;
        case 'P':
            flags |= MT_SSH_NET_FLAG_PRIVACY;
            break;
        case 'R':
            flags |= MT_SSH_NET_FLAG_ROUTEROS;
            break;
        case 'N':
            flags |= MT_SSH_NET_FLAG_NSTREME;
            break;
        case 'T':
            flags |= MT_SSH_NET_FLAG_TDMA;
            break;
        case 'W':
            flags |= MT_SSH_NET_FLAG_WDS;
            break;
        case 'B':
            flags |= MT_SSH_NET_FLAG_BRIDGE;
            break;
        case ' ':
            break;
        default:
#if DEBUG
            printf("Unknown flag: '%c' (%d)\n", buff[i], (gint)buff[i]);
#endif
            break;
        }
    }
    return flags;
}

static gint64
parse_scan_address(const gchar *buff,
                   guint        position)
{
    gchar addr[MAC_ADDR_HEX_LEN+1];
    gchar *ptr;
    gint64 value;
    gint i;

    if(strlen(buff) < position+MAC_ADDR_HEX_LEN+5)
        return -1;

    for(i=0; i<MAC_ADDR_HEX_LEN; i+=2)
    {
        addr[i] = buff[position+i+(i/2)];
        addr[i+1] = buff[position+i+1+(i/2)];
    }

    addr[MAC_ADDR_HEX_LEN] = '\0';

    for(i=0; i<MAC_ADDR_HEX_LEN; i++)
    {
        if(!((addr[i] >= '0' && addr[i] <= '9') ||
             (addr[i] >= 'A' && addr[i] <= 'F') ||
             (addr[i] >= 'a' && addr[i] <= 'f')))
            return -1;
    }

    value = g_ascii_strtoll(addr, &ptr, 16);
    if(ptr != addr)
        return value;

    return -1;
}

static gint
parse_scan_channel(const gchar  *buff,
                   guint         position,
                   gchar       **channel_width,
                   gchar       **mode)
{
    gchar *ptr, *endptr, *endstr, *nextptr;
    gdouble frequency = 0.0;

    if(strlen(buff) > position &&
       isdigit(buff[position]))
    {
        sscanf(buff + position, "%lf", &frequency);

        endstr = strchr(buff + position, ' ');
        ptr = strchr(buff + position, '/');
        if(ptr++ && ptr < endstr)
        {
            endptr = strchr(ptr, '/');
            if(endptr && endptr < endstr)
            {
                *channel_width = g_strndup(ptr, (endptr - ptr));
                ptr = endptr+1;
                endptr = strchr(ptr, ' ');
                if(endptr)
                {
                    nextptr = strchr(ptr, '/');
                    if(nextptr && nextptr < endptr)
                        endptr = nextptr;
                    *mode = g_strndup(ptr, (endptr - ptr));
                }
            }
        }
    }
    return (gint)round(frequency*1000.0);
}

static gint
parse_scan_int(const gchar *buff,
               guint        position,
               guint        max_left_offset)
{
    gint buff_length = (gint)strlen(buff);
    gint i, value = 0;

    for(i=0; i<max_left_offset; i++)
    {
        if(buff_length < position+i)
            break;

        if(isdigit(buff[position+i]) ||
           buff[position+i] == '-')
        {
            sscanf(buff+position+i, "%d", &value);
            break;
        }
    }
    return value;
}

static gchar*
parse_scan_string(const gchar *buff,
                  guint        position,
                  gint         str_length)
{
    gchar *output = NULL;
    gint buff_length = (gint)strlen(buff);
    size_t end;

    if(buff_length < position+str_length)
        str_length = (gint)buff_length-position;

    if(str_length > 0)
    {
        /* right-side trim */
        end = position + str_length;
        while(end >= position && isspace(buff[end]))
            end--;
        output = g_strndup(buff+position, end-position+1);
    }

    return output;
}

static gchar*
parse_scanlist(const gchar *buff)
{
    static const gchar str_channels[] = "channels: ";
    GString *output = NULL;
    gint frequency;
    gchar *ptr;

    ptr = strstr(buff, str_channels);
    ptr = (ptr ? ptr + strlen(str_channels) : 0);

    while(ptr)
    {
        frequency = 0;
        sscanf(ptr, "%d", &frequency);
        if(frequency)
        {
            if(!output)
            {
                output = g_string_new(NULL);
                g_string_append_printf(output, "%d", frequency);
            }
            else
            {
                g_string_append_printf(output, ",%d", frequency);
            }
        }
        ptr = strstr(ptr, ",");
        ptr = (ptr ? ptr + 1 : NULL);
    }

    return (output ? g_string_free(output, FALSE) : NULL);
}