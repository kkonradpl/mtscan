#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>
#include <gtk/gtk.h>
#include <libssh/libssh.h>
#include "conn.h"
#include "mt-ssh.h"
#include "ui-update.h"
#include "ui-connect.h"
#include "network.h"
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

#define DEBUG    1
#define DUMP_PTY 0

#define SSID_LEN        32
#define RADIO_NAME_LEN  16
#define ROS_VERSION_LEN 16

typedef struct scan_header
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
} scan_header_t;

typedef struct mtscan_priv
{
    mtscan_conn_t *conn;
    ssh_channel    channel;
    gchar         *identity;
    gchar         *str_prompt;
    gint           state;
    gboolean       sent_command;
    gchar         *dispatch_scanlist_set;
    gboolean       dispatch_scanlist_get;
    gboolean       dispatch_scan;
    GString       *scanlist;
    scan_header_t  scan_head;
    gint           scan_line;
    gboolean       scan_too_long;
} mtscan_priv_t;

enum
{
    STATE_WAITING_FOR_PROMPT = 0,
    STATE_WAITING_FOR_PROMPT_DIRTY,
    STATE_PROMPT_DIRTY,
    STATE_PROMPT,
    STATE_SCANLIST,
    STATE_WAITING_FOR_SCAN,
    STATE_SCANNING,
    N_STATES
};

static const gchar *str_states[] =
{
    "STATE_WAITING_FOR_PROMPT",
    "STATE_WAITING_FOR_PROMPT_DIRTY",
    "STATE_PROMPT_DIRTY",
    "STATE_PROMPT",
    "STATE_SCANLIST",
    "STATE_WAITING_FOR_SCAN",
    "STATE_SCANNING"
};

static const gchar str_prompt_start[] = "\x1b[9999B[";
static const gchar str_prompt_end[]   = "] > ";

static mtscan_priv_t* mt_ssh_priv_new(mtscan_conn_t*, ssh_channel);
static void mt_ssh_priv_free(mtscan_priv_t*);

static gboolean mt_ssh_verify(mtscan_conn_t*, ssh_session);

static void mt_ssh(mtscan_priv_t*);
static void mt_ssh_request(mtscan_priv_t*, const gchar*);
static void mt_ssh_set_state(mtscan_priv_t*, gint);
static void mt_ssh_scanning(mtscan_priv_t*, gchar*);
static void mt_ssh_commands(mtscan_priv_t*);
static void mt_ssh_dispatch(mtscan_priv_t*);

static void mt_ssh_scan_start(mtscan_priv_t*);
static void mt_ssh_scan_stop(mtscan_priv_t*, gboolean);
static void mt_ssh_scanlist_get(mtscan_priv_t*);
static void mt_ssh_scanlist_set(mtscan_priv_t*, const gchar*);
static void mt_ssh_scanlist_finish(mtscan_priv_t*);

static gint str_count_lines(const gchar*);
static void str_remove_char(gchar*, gchar);

static scan_header_t parse_scan_header(const gchar*);
static network_flags_t parse_scan_flags(const gchar*, guint);
static gchar* parse_scan_address(const gchar*, guint);
static gint parse_scan_channel(const gchar*, guint, gchar**, gchar**);
static gint parse_scan_int(const gchar*, guint, guint);
static gchar* parse_scan_string(const gchar*, guint, gint);
static gchar* parse_scanlist(const gchar*);

static mtscan_priv_t*
mt_ssh_priv_new(mtscan_conn_t* conn,
                ssh_channel    channel)
{
    mtscan_priv_t *priv = g_malloc0(sizeof(mtscan_priv_t));
    priv->conn = conn;
    priv->channel = channel;
    priv->str_prompt = g_strdup_printf("%s%s@", str_prompt_start, priv->conn->login);
    priv->state = STATE_WAITING_FOR_PROMPT;
    priv->dispatch_scanlist_get = TRUE;
    priv->dispatch_scan = TRUE;
    priv->scan_line = -1;
    return priv;
}

static void
mt_ssh_priv_free(mtscan_priv_t* priv)
{
    if(priv->scanlist)
        g_string_free(priv->scanlist, TRUE);
    g_free(priv->dispatch_scanlist_set);
    g_free(priv->str_prompt);
    g_free(priv->identity);
    g_free(priv);
}

gpointer
mt_ssh_thread(gpointer data)
{
    mtscan_conn_t *conn = (mtscan_conn_t*)data;
    mtscan_priv_t *priv;
    ssh_session session;
    ssh_channel channel;
    gchar *login;
    gint verbosity = (DEBUG ? SSH_LOG_PROTOCOL : SSH_LOG_WARNING);
    glong timeout = 30;

#ifdef DEBUG
    printf("mt-ssh thread start: %p\n", (void*)conn);
#endif

    if(!(session = ssh_new()))
    {
        conn->return_state = CONN_ERR_SSH_NEW;
        goto cleanup_callback;
    }

    if(ssh_options_set(session, SSH_OPTIONS_HOST, conn->hostname) != SSH_OK ||
       ssh_options_set(session, SSH_OPTIONS_PORT_STR, conn->port) != SSH_OK ||
       ssh_options_set(session, SSH_OPTIONS_TIMEOUT, &timeout) != SSH_OK ||
       ssh_options_set(session, SSH_OPTIONS_LOG_VERBOSITY, &verbosity) != SSH_OK)
    {
        conn->return_state = CONN_ERR_SSH_SET_OPTIONS;
        goto cleanup_free_ssh;
    }

    if(conn->canceled)
        goto cleanup_free_ssh;

    g_idle_add(connection_callback_info, conn_msg_new(CONN_MSG_CONNECTING, NULL, conn));

    if(ssh_connect(session) != SSH_OK)
    {
        conn->return_state = CONN_ERR_SSH_CONNECT;
        conn->err = g_strdup(ssh_get_error(session));
        goto cleanup_free_ssh;
    }

    if(conn->canceled)
        goto cleanup_disconnect;

    if(!mt_ssh_verify(conn, session))
    {
        conn->return_state = CONN_ERR_SSH_VERIFY;
        goto cleanup_disconnect;
    }

    if(conn->canceled)
        goto cleanup_disconnect;

    g_idle_add(connection_callback_info, conn_msg_new(CONN_MSG_AUTHENTICATING, NULL, conn));

    /* Disable console colors with extra SSH login parameter:
     * https://forum.mikrotik.com/viewtopic.php?t=21234#p101304 */
    login = g_strdup_printf("%s+ct", conn->login);
    if(ssh_userauth_password(session, login, conn->password) != SSH_AUTH_SUCCESS)
    {
        g_free(login);
        conn->return_state = CONN_ERR_SSH_AUTH;
        goto cleanup_disconnect;
    }
    g_free(login);

    if(conn->canceled)
        goto cleanup_disconnect;

    if(!(channel = ssh_channel_new(session)))
    {
        conn->return_state = CONN_ERR_SSH_CHANNEL_NEW;
        goto cleanup_disconnect;
    }

    if(ssh_channel_open_session(channel) != SSH_OK)
    {
        conn->return_state = CONN_ERR_SSH_CHANNEL_OPEN;
        goto cleanup_free_channel;
    }

    if(ssh_channel_request_pty_size(channel, "vt100", PTY_COLS, PTY_ROWS) != SSH_OK)
    {
        conn->return_state = CONN_ERR_SSH_CHANNEL_REQ_PTY_SIZE;
        goto cleanup_full;
    }

    if(ssh_channel_request_shell(channel) != SSH_OK)
    {
        conn->return_state = CONN_ERR_SSH_CHANNEL_REQ_SHELL;
        goto cleanup_full;
    }

    if(conn->canceled)
        goto cleanup_full;

    g_idle_add(connection_callback_info, conn_msg_new(CONN_MSG_CONNECTED, NULL, conn));

    priv = mt_ssh_priv_new(conn, channel);
    mt_ssh(priv);
    mt_ssh_priv_free(priv);

    conn->return_state = CONN_CLOSED;

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
    g_idle_add(connection_callback, conn);
#ifdef DEBUG
    printf("mt-ssh thread stop %p\n", (void*)conn);
#endif
    return NULL;
}

static gboolean
mt_ssh_verify(mtscan_conn_t *conn,
              ssh_session    session)
{
    guchar *hash;
    size_t hlen;
    gchar *hexa;
    mtscan_conn_cmd_t *msg;
    gchar *message;
    gint state = ssh_is_server_known(session);

#if LIBSSH_VERSION_MAJOR >= 0 && LIBSSH_VERSION_MINOR >= 6
    ssh_key srv_pubkey;
    if(ssh_get_publickey(session, &srv_pubkey) < 0)
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
        g_idle_add(connection_callback_info, conn_msg_new(CONN_MSG_AUTH_ERROR, message, conn));
        ssh_string_free_char(hexa);
        ssh_clean_pubkey_hash(&hash);
        return FALSE;

    case SSH_SERVER_FOUND_OTHER:
        message = g_strdup_printf("The host key for this server was not found but an other type of key exists.\n"
                                  "For security reasons, the connection will be closed.\n");
        g_idle_add(connection_callback_info, conn_msg_new(CONN_MSG_AUTH_ERROR, message, conn));
        ssh_clean_pubkey_hash(&hash);
        return FALSE;

    case SSH_SERVER_FILE_NOT_FOUND:
    case SSH_SERVER_NOT_KNOWN:
        hexa = ssh_get_hexa(hash, hlen);
        message = g_strdup_printf("The authenticity of host '%s' can't be established.\n"
                                  "The key hash is:\n%s\n"
                                  "Are you sure you want to continue connecting?",
                                  conn->hostname, hexa);
        g_idle_add(connection_callback_info, conn_msg_new(CONN_MSG_AUTH_VERIFY, message, conn));
        ssh_string_free_char(hexa);

        while((msg = g_async_queue_pop(conn->queue)))
        {
            if(msg->type == COMMAND_AUTH)
            {
                conn_command_free(msg);
                break;
            }
            else
            {
                conn_command_free(msg);
                ssh_clean_pubkey_hash(&hash);
                return FALSE;
            }
        }

        if(ssh_write_knownhost(session) < 0)
        {
            ssh_clean_pubkey_hash(&hash);
            message = g_strdup_printf("Failed to add the host to the list of known hosts:\n%s\n", strerror(errno));
            g_idle_add(connection_callback_info, conn_msg_new(CONN_MSG_AUTH_ERROR, message, conn));
            return FALSE;
        }
        break;

    case SSH_SERVER_ERROR:
        ssh_clean_pubkey_hash(&hash);
        g_idle_add(connection_callback_info, conn_msg_new(CONN_MSG_AUTH_ERROR, g_strdup(ssh_get_error(session)), conn));
        return FALSE;
    }

    ssh_clean_pubkey_hash(&hash);
    return TRUE;
}

static void
mt_ssh(mtscan_priv_t *priv)
{
    gchar buffer[SOCKET_BUFFER+1];
    gchar *line, *ptr;
    gint n, i, lines;
    size_t length, offset = 0;
    struct timeval timeout;
    ssh_channel in_channels[2];

    while(ssh_channel_is_open(priv->channel) &&
          !ssh_channel_is_eof(priv->channel))
    {
        /* Handle commands from the queue */
        mt_ssh_commands(priv);

        if(priv->conn->canceled)
        {
            mt_ssh_scan_stop(priv, FALSE);
            return;
        }

        timeout.tv_sec = 0;
        timeout.tv_usec = READ_TIMEOUT_MSEC * 1000;
        in_channels[0] = priv->channel;
        in_channels[1] = NULL;
        if(ssh_channel_select(in_channels, NULL, NULL, &timeout) == SSH_EINTR)
            continue;
        if(ssh_channel_is_eof(priv->channel))
            return;
        if(in_channels[0] == NULL)
            continue;

        n = ssh_channel_poll(priv->channel, 0);
        if(n < 0)
            return;
        if(n == 0)
            continue;

        n = ssh_channel_read(priv->channel, buffer+offset, sizeof(buffer)-1-offset, 0);
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

            if(!priv->identity &&
               length > (strlen(priv->str_prompt) + strlen(str_prompt_end)) &&
               !strncmp(line, priv->str_prompt, strlen(priv->str_prompt)) &&
               !strcmp(line+(length-strlen(str_prompt_end)), str_prompt_end))

            {
                /* Save the device identity from prompt */
                priv->identity = g_strndup(line + strlen(priv->str_prompt),
                                           length - strlen(priv->str_prompt) - strlen(str_prompt_end));
#if DEBUG
                printf("[!] Identity: %s\n", priv->identity);
#endif
                g_free(priv->str_prompt);
                priv->str_prompt = g_strdup_printf("%s%s@%s%s", str_prompt_start, priv->conn->login, priv->identity, str_prompt_end);
                g_idle_add(connection_callback_info, conn_msg_new(CONN_MSG_SCANNING, NULL, priv->conn));
            }

            if(priv->identity &&
               priv->state != STATE_PROMPT &&
               !priv->sent_command &&
               !strncmp(line, priv->str_prompt, strlen(priv->str_prompt)))
            {
                if(priv->state == STATE_WAITING_FOR_PROMPT_DIRTY)
                {
                    mt_ssh_set_state(priv, STATE_PROMPT_DIRTY);
                    ssh_channel_write(priv->channel, "\03", 1); /* CTRL+C */
                }
                else
                {
                    mt_ssh_set_state(priv, STATE_PROMPT);
                    mt_ssh_dispatch(priv);
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

            if(priv->sent_command)
            {
                /* The command is echoed back to terminal, ignore it */
                priv->sent_command = FALSE;
                continue;
            }

#if DEBUG
            printf("[%d] %s\n", i, line);
#endif

            if(priv->state == STATE_SCANLIST)
            {
                /* Buffer the scan-list */
                g_string_append(priv->scanlist, line);
            }
            else if(priv->state == STATE_WAITING_FOR_SCAN ||
                    priv->state == STATE_SCANNING)
            {
                mt_ssh_scanning(priv, line);
            }
        }

        if(priv->scan_too_long &&
           !priv->conn->remote_mode)
        {
            /* Restart scanning */
            priv->scan_too_long = FALSE;
            mt_ssh_scan_stop(priv, TRUE);
        }
    }
}

static void
mt_ssh_request(mtscan_priv_t *priv,
               const gchar   *req)
{
    uint32_t len = (uint32_t)strlen(req);
    ssh_channel_write(priv->channel, req, len);
    priv->sent_command = TRUE;
}

static void
mt_ssh_set_state(mtscan_priv_t *priv,
                 gint           state)
{
    if(priv->state == state)
        return;

#if DEBUG
    g_assert(state >= 0 && state < N_STATES);
    printf("[!] Changing state from %s (%d) to %s (%d)\n",
           str_states[priv->state], priv->state,
           str_states[state], state);
#endif

    if(state == STATE_SCANNING)
    {
        g_idle_add(ui_update_scanning_state, GINT_TO_POINTER(TRUE));
    }
    else if(priv->state == STATE_SCANNING &&
            !priv->conn->remote_mode)
    {
        g_idle_add(ui_update_scanning_state, GINT_TO_POINTER(FALSE));
    }
    else if(priv->state == STATE_SCANLIST)
    {
        mt_ssh_scanlist_finish(priv);
    }

    priv->state = state;
}

static void
mt_ssh_scanning(mtscan_priv_t *priv,
                gchar         *line)
{
    static const gchar str_badcommand[]     = "expected end of command";
    static const gchar str_nosuchitem[]     = "no such item";
    static const gchar str_failure[]        = "failure:";
    static const gchar str_scannotrunning[] = "scan not running";
    static const gchar str_scanstart[]      = "Flags: ";
    static const gchar str_scanend[]        = "-- ";

    network_t *net;
    network_flags_t flags;
    gchar *address;
    gchar *ptr;

    if(priv->state == STATE_WAITING_FOR_SCAN &&
       (!strncmp(line, str_failure, strlen(str_failure)) ||
        !strncmp(line, str_badcommand, strlen(str_badcommand)) ||
        !strncmp(line, str_nosuchitem, strlen(str_nosuchitem))))
    {
        /* Handle possible failures */
        mt_ssh_set_state(priv, STATE_WAITING_FOR_PROMPT);
        g_idle_add(connection_callback_info, conn_msg_new(CONN_MSG_SCANNING_ERROR, g_strdup(line), priv->conn));
        return;
    }

    if(!strncmp(line, str_scannotrunning, strlen(str_scannotrunning)))
    {
        /* Scanning stops when SSH connection is interrupted for a while */
        mt_ssh_set_state(priv, STATE_WAITING_FOR_PROMPT);
        priv->dispatch_scan = TRUE;
        return;
    }

    if(priv->state == STATE_SCANNING &&
       !strncmp(line, str_scanend, strlen(str_scanend)))
    {
        /* This is the last line of scan results */
        if(priv->scan_line > PTY_ROWS/2)
        {
            priv->scan_too_long = TRUE;
#if DEBUG
            printf("<!> Scan results are getting too long, scheduling restart\n");
#endif
        }
        priv->scan_line = -1;
        g_idle_add(ui_update_finish, NULL);
#if DEBUG
        printf("<!> Found END of a scan frame\n");
#endif
        return;
    }

    if(priv->scan_line < 0)
    {
        if(strstr(line, str_scanstart))
        {
            /* This is the first line of scan results */
            priv->scan_line = 0;
            mt_ssh_set_state(priv, STATE_SCANNING);
#if DEBUG
            printf("<!> Found START of a scan frame\n");
#endif
        }
        return;
    }

    if(++priv->scan_line == 1)
    {
        /* Second line of scan results should contain a header */
        if(priv->scan_head.address == 0)
        {
            /* Try to parse the header */
            priv->scan_head = parse_scan_header(line);
#if DEBUG
            if(priv->scan_head.address)
                printf("<!> Found header. Address index = %d\n", priv->scan_head.address);
#endif
        }
        return;
    }

    if(priv->scan_head.address == 0)
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
    flags = parse_scan_flags(line, priv->scan_head.address-1);

    if(!flags.active)
    {
        /* This network is not active at the moment, ignore it */
        return;
    }

    if(!(address = parse_scan_address(line, priv->scan_head.address)))
    {
        /* This line doesn't contain a valid MAC address */
#if DEBUG
        printf("<!> ERROR: Invalid MAC address!\n");
#endif
        return;
    }

    net = g_malloc0(sizeof(network_t));
    net->address = address;
    net->flags = flags;
    net->firstseen = UNIX_TIMESTAMP();

    if(priv->scan_head.ssid)
        net->ssid = parse_scan_string(line, priv->scan_head.ssid, SSID_LEN);

    if(priv->scan_head.channel)
        net->frequency = parse_scan_channel(line, priv->scan_head.channel, &net->channel, &net->mode);

    if(priv->scan_head.channel_width && !net->channel) /* Pre v6.30 */
        net->channel = g_strdup_printf("%d", parse_scan_int(line, priv->scan_head.channel_width, 1));

    if(priv->scan_head.rssi)
        net->rssi = parse_scan_int(line, priv->scan_head.rssi, 3);

    if(priv->scan_head.noise > 2)
        net->noise = parse_scan_int(line, priv->scan_head.noise-2, 4); // __NF !

    if(priv->scan_head.radioname)
        net->radioname = parse_scan_string(line, priv->scan_head.radioname, RADIO_NAME_LEN);

    if(priv->scan_head.ros_version)
        net->routeros_ver = parse_scan_string(line, priv->scan_head.ros_version, ROS_VERSION_LEN);

    g_idle_add(ui_update_network, net);
}

static void
mt_ssh_commands(mtscan_priv_t *priv)
{
    mtscan_conn_cmd_t *msg;

    while((msg = g_async_queue_try_pop(priv->conn->queue)))
    {
        switch(msg->type)
        {
            case COMMAND_SCAN_START:
                priv->conn->duration = (msg->data ? atoi(msg->data) : 0);
                priv->dispatch_scan = TRUE;
                break;

            case COMMAND_SCAN_RESTART:
                priv->conn->remote_mode = FALSE;
                mt_ssh_scan_stop(priv, TRUE);
                break;

            case COMMAND_SCAN_STOP:
                priv->conn->remote_mode = FALSE;
                mt_ssh_scan_stop(priv, FALSE);
                priv->dispatch_scan = FALSE;
                break;

            case COMMAND_DISCONNECT:
                priv->conn->canceled = TRUE;
                break;

            case COMMAND_GET_SCANLIST:
                priv->dispatch_scanlist_get = TRUE;
                if(priv->state >= STATE_WAITING_FOR_SCAN)
                    mt_ssh_scan_stop(priv, TRUE);
                break;

            case COMMAND_SET_SCANLIST:
                /* Clear existing request */
                g_free(priv->dispatch_scanlist_set);
                priv->dispatch_scanlist_set = g_strdup(msg->data);
                /* Always check new scanlist */
                priv->dispatch_scanlist_get = TRUE;
                if(priv->state >= STATE_WAITING_FOR_SCAN)
                    mt_ssh_scan_stop(priv, TRUE);
                break;
        }
        conn_command_free(msg);
        mt_ssh_dispatch(priv);
    }
}
static void
mt_ssh_dispatch(mtscan_priv_t *priv)
{
    if(priv->state != STATE_PROMPT)
        return;

    if(priv->dispatch_scanlist_set)
    {
        mt_ssh_scanlist_set(priv, priv->dispatch_scanlist_set);
        g_free(priv->dispatch_scanlist_set);
        priv->dispatch_scanlist_set = NULL;
    }
    else if(priv->dispatch_scanlist_get)
    {
        mt_ssh_scanlist_get(priv);
        priv->dispatch_scanlist_get = FALSE;
    }
    else if(priv->dispatch_scan || priv->conn->remote_mode)
    {
        mt_ssh_scan_start(priv);
        priv->dispatch_scan = FALSE;
    }
}

static void
mt_ssh_scan_start(mtscan_priv_t *priv)
{
    gchar *str_scan;

    if(priv->conn->background && priv->conn->duration)
        str_scan = g_strdup_printf(":delay 250ms; /interface wireless scan %s background=yes duration=%d\r", priv->conn->iface, priv->conn->duration);
    else if(priv->conn->background)
        str_scan = g_strdup_printf(":delay 250ms; /interface wireless scan %s background=yes\r", priv->conn->iface);
    else if(priv->conn->duration)
        str_scan = g_strdup_printf("/interface wireless scan %s duration=%d\r", priv->conn->iface, priv->conn->duration);
    else
        str_scan = g_strdup_printf("/interface wireless scan %s\r", priv->conn->iface);

    mt_ssh_request(priv, str_scan);
    g_free(str_scan);
    priv->scan_line = -1;

    mt_ssh_set_state(priv, STATE_WAITING_FOR_SCAN);
}

static void
mt_ssh_scan_stop(mtscan_priv_t *priv,
                 gboolean       restart)
{
    if(priv->state < STATE_WAITING_FOR_SCAN)
        return;

    /* Do not use mt_ssh_request, as the 'Q' is not echoed back */
    ssh_channel_write(priv->channel, "Q", 1);
    mt_ssh_set_state(priv, STATE_WAITING_FOR_PROMPT_DIRTY);

    if(restart)
        priv->dispatch_scan = TRUE;
}

static void
mt_ssh_scanlist_get(mtscan_priv_t *priv)
{
    gchar *str_scanlist_get;

    str_scanlist_get = g_strdup_printf("/interface wireless info scan-list %s\r", priv->conn->iface);
    mt_ssh_request(priv, str_scanlist_get);
    g_free(str_scanlist_get);
    priv->scanlist = g_string_new(NULL);
    mt_ssh_set_state(priv, STATE_SCANLIST);
}

static void
mt_ssh_scanlist_set(mtscan_priv_t *priv,
                    const gchar  *scanlist)
{
    gchar *str_scanlist_set;

    str_scanlist_set = g_strdup_printf("/interface wireless set %s scan-list=%s\r", priv->conn->iface, scanlist);
    mt_ssh_request(priv, str_scanlist_set);
    g_free(str_scanlist_set);
    mt_ssh_set_state(priv, STATE_WAITING_FOR_PROMPT);
}

static void
mt_ssh_scanlist_finish(mtscan_priv_t *priv)
{
    gchar *str = g_string_free(priv->scanlist, FALSE);
    priv->scanlist = NULL;
    g_idle_add(ui_update_scanlist, parse_scanlist(str));
    g_free(str);
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

static scan_header_t
parse_scan_header(const gchar *buff)
{
    scan_header_t p = {0};
    gchar *ptr;

    if((ptr = strstr(buff, "ADDRESS")))
        p.address = (guint)(ptr-buff);
    else
    {
#if DEBUG
        printf("ADDRESS position not found!");
#endif
        return p;
    }

    if((ptr = strstr(buff, "SSID")))
        p.ssid = (guint)(ptr-buff);

    /* Pre v6.30 */
    if((ptr = strstr(buff, "BAND")))
        p.band = (guint)(ptr-buff);

    if((ptr = strstr(buff, "CHANNEL-WIDTH")))
        p.channel_width = (guint)(ptr-buff);

    if((ptr = strstr(buff, "FREQ")))
        p.channel = (guint)(ptr-buff);
    /* --------- */

    /* Post v6.30 */
    if(!p.channel && (ptr = strstr(buff, "CHANNEL")))
        p.channel = (guint)(ptr-buff);
    /* ---------- */

    if((ptr = strstr(buff, "SIG")))
        p.rssi = (guint)(ptr-buff);

    if((ptr = strstr(buff, "NF")))
        p.noise = (guint)(ptr-buff);

    if((ptr = strstr(buff, "SNR")))
        p.snr = (guint)(ptr-buff);

    if((ptr = strstr(buff, "RADIO-NAME")))
        p.radioname = (guint)(ptr-buff);

    if((ptr = strstr(buff, "ROUTEROS-VERSION")))
        p.ros_version = (guint)(ptr-buff);

    return p;
}

static network_flags_t
parse_scan_flags(const gchar *buff,
                 guint        flags_len)
{
    size_t length = MIN(strlen(buff), flags_len);
    network_flags_t flags = {0};
    gint i;

    for(i=0; i<length; i++)
    {
        switch(buff[i])
        {
        case 'A':
            flags.active = 1;
            break;
        case 'P':
            flags.privacy = 1;
            break;
        case 'R':
            flags.routeros = 1;
            break;
        case 'N':
            flags.nstreme = 1;
            break;
        case 'T':
            flags.tdma = 1;
            break;
        case 'W':
            flags.wds = 1;
            break;
        case 'B':
            flags.bridge = 1;
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

static gchar*
parse_scan_address(const gchar *buff,
                   guint        position)
{
    gchar *addr = g_malloc(sizeof(gchar)*13);

    if(strlen(buff) > position &&
       sscanf(buff + position, "%c%c:%c%c:%c%c:%c%c:%c%c:%c%c",
              &addr[0], &addr[1], &addr[2], &addr[3], &addr[4], &addr[5],
              &addr[6], &addr[7], &addr[8], &addr[9], &addr[10], &addr[11]) == 12)
    {
        addr[12] = 0;
        return addr;
    }
    g_free(addr);
    return NULL;
}

static gint
parse_scan_channel(const gchar  *buff,
                   guint         position,
                   gchar       **channel_width,
                   gchar       **mode)
{
    gchar *ptr, *endptr, *endstr;
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
                    *mode = g_strndup(ptr, (endptr - ptr));
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
    gint buff_length = strlen(buff);
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