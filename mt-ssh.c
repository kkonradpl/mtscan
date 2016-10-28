#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
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

#define DEBUG    0
#define DUMP_PTY 0

#define SSID_LEN        32
#define RADIO_NAME_LEN  16
#define ROS_VERSION_LEN 16

typedef struct scan_header
{
    guint address;
    guint ssid;
    guint channel;      /* post v6.30 */
    guint band;          /* pre v6.30 */
    guint channel_width; /* pre v6.30 */
    guint frequency;     /* pre v6.30 */
    guint rssi;
    guint noise;
    guint snr;
    guint radioname;
    guint ros_version;
} scan_header_t;

static const gchar str_badcommand[]     = "expected end of command";
static const gchar str_nosuchitem[]     = "no such item";
static const gchar str_failure[]        = "failure:";
static const gchar str_scannotrunning[] = "scan not running";
static const gchar str_endofscan[]      = "-- ";
static const gchar str_flags[]          = "Flags: ";
static const gchar str_scan_stop[]      = "Q\r";


static gboolean mt_ssh_verify(mtscan_conn_t*, ssh_session);
static void mt_ssh_read(mtscan_conn_t*, ssh_channel);
static void mt_ssh_set_state(mtscan_conn_t*, gint);
static void mt_ssh_scan_start(mtscan_conn_t*, ssh_channel);
static void mt_ssh_scan_stop(mtscan_conn_t*, ssh_channel);
static void mt_ssh_scan_list(mtscan_conn_t*, ssh_channel, const gchar*);

static gint count_lines(const gchar*);
static void remove_char(gchar*, gchar);

static scan_header_t parse_scan_header(const gchar*);
static network_flags_t parse_scan_flags(const gchar*, guint);
static gchar* parse_scan_address(const gchar*, guint);
static gint parse_scan_freq(const gchar*, guint, guint);
static gint parse_scan_int(const gchar*, guint, guint);
static gchar* parse_scan_string(const gchar*, guint, gint, gint);
static gchar* parse_scan_channel(const gchar*, guint);
static gchar* parse_scan_mode(const gchar*, guint);


gpointer
mt_ssh_thread(gpointer data)
{
    mtscan_conn_t *conn = (mtscan_conn_t*)data;
    ssh_session session;
    ssh_channel channel;
    gchar *login;
    gint verbosity = SSH_LOG_PROTOCOL;
    glong timeout = 30;

#ifdef DEBUG
    printf("Thread start: %p\n", (void*)conn);
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

    mt_ssh_read(conn, channel);
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
    printf("Thread stop %p\n", (void*)conn);
#endif
    return NULL;
}

static gboolean
mt_ssh_verify(mtscan_conn_t     *conn,
              ssh_session session)
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
                break;
            }
            else
            {
                return FALSE;
            }
            g_free(msg->data);
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
mt_ssh_read(mtscan_conn_t     *conn,
            ssh_channel channel)
{
    gchar buffer[SOCKET_BUFFER+1], *start, *ptr, **data;
    gint n, i, lines, offset = 0;
    scan_header_t head = {0};
    mtscan_conn_cmd_t *msg;
    gboolean list_too_long = FALSE;
    gboolean dispatch_scan = TRUE;
    gchar *dispatch_scanlist = NULL;
    gchar *str_prompt = g_strdup_printf("\x1b[9999B[%s@", conn->login);
    struct timeval timeout;
    ssh_channel in_channels[2];

    while(ssh_channel_is_open(channel) && !ssh_channel_is_eof(channel))
    {
        while((msg = g_async_queue_try_pop(conn->queue)))
        {
            switch(msg->type)
            {
            case COMMAND_SCAN_START:
                if(msg->data)
                {
                    conn->duration = atoi((gchar*)msg->data);
                    g_free(msg->data);
                }
                if(conn->state == STATE_WAITING_FOR_PROMPT)
                    dispatch_scan = TRUE;
                else if(conn->state == STATE_PROMPT)
                    mt_ssh_scan_start(conn, channel);
                break;

            case COMMAND_SCAN_RESTART:
                if(conn->state == STATE_PROMPT)
                {
                    mt_ssh_scan_start(conn, channel);
                    break;
                }
                dispatch_scan = TRUE;
            case COMMAND_SCAN_STOP:
                conn->remote_mode = FALSE;
                mt_ssh_scan_stop(conn, channel);
                break;

            case COMMAND_DISCONNECT:
                conn->canceled = TRUE;
                break;

            case COMMAND_SET_SCANLIST:
                if(conn->state == STATE_PROMPT)
                {
                    mt_ssh_scan_list(conn, channel, msg->data);
                    g_free(msg->data);
                    break;
                }
                if(conn->state == STATE_SCANNING)
                {
                    dispatch_scan = TRUE;
                    mt_ssh_scan_stop(conn, channel);
                }
                g_free(dispatch_scanlist);
                dispatch_scanlist = msg->data;
                break;
            }
            g_free(msg);
        }

        if(conn->canceled)
        {
            mt_ssh_scan_stop(conn, channel);
            goto cleanup;
        }

        timeout.tv_sec = 0;
        timeout.tv_usec = READ_TIMEOUT_MSEC * 1000;
        in_channels[0] = channel;
        in_channels[1] = NULL;
        if(ssh_channel_select(in_channels, NULL, NULL, &timeout) == SSH_EINTR)
            continue;
        if(ssh_channel_is_eof(channel) != 0)
            goto cleanup;
        if(in_channels[0] == NULL)
            continue;

        n = ssh_channel_poll(channel, 0);
        if(n < 0)
            goto cleanup;
        if(n == 0)
            continue;

        n = ssh_channel_read(channel, buffer+offset, sizeof(buffer)-1-offset, 0);
        if(n < 0)
            goto cleanup;
        if(n == 0)
            continue;

        start = buffer;
        n += offset;
        buffer[n] = 0;
        remove_char(buffer+offset, '\n');
#if DUMP_PTY
        fprintf(stderr, "%s", buffer+offset);
        fflush(stderr);
#endif
        offset = 0;
        lines = count_lines(buffer);

        data = g_malloc(lines * sizeof(char*));
        for(i=0; (ptr = strsep(&start, "\r")); i++)
            data[i] = ptr;
#if DEBUG
        printf("-------------------- Buffered %d lines\n", lines);
#endif
        for(i=0;i<lines;i++)
        {
            network_t *net;
            network_flags_t flags;
            gchar *address;
            gint length = strlen(data[i]);

#if DEBUG
            printf("[%d] %s\n", i, data[i]);
#endif
            if(conn->state == STATE_WAITING_FOR_PROMPT ||
               conn->state == STATE_SCANNING)
            {
                /* Look for "[login@ …] > " string */
                if(!strncmp(data[i], str_prompt, strlen(str_prompt)) &&
                   data[i][length-4] == ']' &&
                   data[i][length-3] == ' ' &&
                   data[i][length-2] == '>' &&
                   data[i][length-1] == ' ')

                {
                    if(conn->state == STATE_SCANNING && !conn->remote_mode)
                        g_idle_add(ui_update_scanning_state, GINT_TO_POINTER(FALSE));

                    mt_ssh_set_state(conn, STATE_PROMPT);
                    if(dispatch_scanlist)
                    {
                        mt_ssh_scan_list(conn, channel, dispatch_scanlist);
                        g_free(dispatch_scanlist);
                        dispatch_scanlist = NULL;
                    }
                    else if(dispatch_scan || conn->remote_mode)
                    {
                        mt_ssh_scan_start(conn, channel);
                        dispatch_scan = FALSE;
                    }
                    continue;
                }
            }

            if(conn->state < STATE_WAITING_FOR_SCAN)
                continue;

            if(!strncmp(data[i], str_failure, strlen(str_failure)) ||
               !strncmp(data[i], str_badcommand, strlen(str_badcommand)) ||
               !strncmp(data[i], str_nosuchitem, strlen(str_nosuchitem)))
            {
                mt_ssh_set_state(conn, STATE_WAITING_FOR_PROMPT);
                g_idle_add(connection_callback_info, conn_msg_new(CONN_MSG_SCANNING_ERROR, g_strdup(data[i]), conn));
                continue;
            }

            if(!strncmp(data[i], str_scannotrunning, strlen(str_scannotrunning)))
            {
                mt_ssh_set_state(conn, STATE_WAITING_FOR_PROMPT);
                dispatch_scan = TRUE;
                continue;
            }

            if(conn->state == STATE_SCANNING &&
               !strncmp(data[i], str_endofscan, strlen(str_endofscan)))
            {
                /* This is a complete end line of scan */
#if DEBUG
                printf("<%d> Found END of a scan frame\n", i);
#endif
                g_idle_add(ui_update_finish, NULL);
                conn->scan_line = -1;
                continue;
            }

            if(i == lines-1)
            {
                /* This line may be incomplete. */
#if DEBUG
                printf("<%d> Scan frame is incomplete. Waiting for more data.\n", i);
#endif
                break;
            }

            if(conn->scan_line < 0)
            {
                /* Waiting for a first line of scan output */
                if(strstr(data[i], str_flags))
                {
                    /* This is a start of scan */
                    conn->scan_line = 0;
                    mt_ssh_set_state(conn, STATE_SCANNING);
#if DEBUG
                    printf("<%d> Found START of a scan frame\n", i);
#endif
                }
                continue;
            }

            if(++conn->scan_line == 1)
            {
                /* Second line should contain a header */
                if(head.address == 0)
                {
                    /* Got header-like line for the first time, try to parse it */
                    head = parse_scan_header(data[i]);
                    if(head.address)
                    {
#if DEBUG
                        printf("<%d> Found header. Address index = %d\n", i, head.address);
#endif
                        g_idle_add(connection_callback_info, conn_msg_new(CONN_MSG_SCANNING, NULL, conn));
                    }
                }
                continue;
            }

            if(head.address == 0)
            {
                /* Header hasn't been found yet, give up */
                printf("<%d> ERROR: No scan header found!\n", i);
                continue;
            }

            /* Parse all flags at the beginning of line */
            flags = parse_scan_flags(data[i], head.address-1);
            if(!flags.active)
            {
                /* This network is not active at the moment, ignore it */
                continue;
            }

            if(!(address = parse_scan_address(data[i], head.address)))
            {
                /* This line doesn't contain a valid MAC address */
                continue;
            }

            net = g_malloc0(sizeof(network_t));
            net->address = address;
            net->ssid = parse_scan_string(data[i], head.ssid, SSID_LEN, 0);
            if(!head.channel_width && !head.frequency)
            {
                /* Post v6.30 */
                net->frequency = parse_scan_freq(data[i], head.channel, 1);
                net->channel = parse_scan_channel(data[i], head.channel);
                net->mode = parse_scan_mode(data[i], head.channel);
            }
            else
            {
                /* Pre v6.30 */
                net->frequency = parse_scan_freq(data[i], head.frequency, 1);
                net->channel = g_strdup_printf("%d", parse_scan_int(data[i], head.channel_width, 1));
                net->mode = NULL;
            }
            net->rssi = parse_scan_int(data[i], head.rssi, 3);
            net->noise = parse_scan_int(data[i], head.noise-2, 4); // __NF !
            net->radioname = parse_scan_string(data[i], head.radioname, RADIO_NAME_LEN, 1);
            net->routeros_ver = parse_scan_string(data[i], head.ros_version, ROS_VERSION_LEN, 1);
            net->flags = flags;
            net->firstseen = UNIX_TIMESTAMP();

            g_idle_add(ui_update_network, net);

            if(conn->scan_line > PTY_ROWS/2)
            {
                /* List is getting too long… restart! */
                list_too_long = TRUE;
            }
        }

        /* This last line may be incomplete.
           Set an offset and move it at the buffer beginning… */
        offset = strlen(data[lines-1]);
        if(lines > 1)
        {
            for(i=0; i<offset; i++)
                buffer[i] = data[lines-1][i];
        }

        /* Restart scanning… */
        if(list_too_long && !conn->remote_mode)
        {
            list_too_long = FALSE;
            dispatch_scan = TRUE;
            mt_ssh_scan_stop(conn, channel);
        }

        g_free(data);
    }

cleanup:
    g_free(str_prompt);
}

static void
mt_ssh_set_state(mtscan_conn_t *conn,
                 gint    state)
{
#if DEBUG
    printf("[!] Changing state to ");
    switch(state)
    {
    case STATE_WAITING_FOR_PROMPT:
        printf("STATE_WAITING_FOR_PROMPT");
        break;
    case STATE_PROMPT:
        printf("STATE_PROMPT");
        break;
    case STATE_WAITING_FOR_SCAN:
        printf("STATE_WAITING_FOR_SCAN");
        break;
    case STATE_SCANNING:
        printf("STATE_SCANNING");
        break;
    default:
        printf("unknown");
        break;
    }
    printf("\n");
#endif
    conn->state = state;

}

static void
mt_ssh_scan_start(mtscan_conn_t     *conn,
                  ssh_channel channel)
{
    gchar *str_scan;
    if(conn->state == STATE_PROMPT)
    {
        if(conn->background && conn->duration)
            str_scan = g_strdup_printf(":delay 250ms; /interface wireless scan %s background=yes duration=%d\r", conn->iface, conn->duration);
        else if(conn->background)
            str_scan = g_strdup_printf(":delay 250ms; /interface wireless scan %s background=yes\r", conn->iface);
        else if(conn->duration)
            str_scan = g_strdup_printf("/interface wireless scan %s duration=%d\r", conn->iface, conn->duration);
        else
            str_scan = g_strdup_printf("/interface wireless scan %s\r", conn->iface);

        ssh_channel_write(channel, str_scan, strlen(str_scan));
        g_free(str_scan);
        mt_ssh_set_state(conn, STATE_WAITING_FOR_SCAN);
        conn->scan_line = -1;
        g_idle_add(ui_update_scanning_state, GINT_TO_POINTER(TRUE));
    }
}

static void
mt_ssh_scan_stop(mtscan_conn_t     *conn,
                 ssh_channel channel)
{
    if(conn->state >= STATE_WAITING_FOR_SCAN)
    {
        ssh_channel_write(channel, str_scan_stop, strlen(str_scan_stop));
        mt_ssh_set_state(conn, STATE_WAITING_FOR_PROMPT);
        g_idle_add(ui_update_scanning_state, GINT_TO_POINTER(FALSE));
    }
}

static void
mt_ssh_scan_list(mtscan_conn_t      *conn,
                 ssh_channel  channel,
                 const gchar *scan_list)
{
    gchar *str_scanlist;
    str_scanlist = g_strdup_printf("/interface wireless set %s scan-list=%s\r", conn->iface, scan_list);
    ssh_channel_write(channel, str_scanlist, strlen(str_scanlist));
    g_free(str_scanlist);
    mt_ssh_set_state(conn, STATE_WAITING_FOR_PROMPT);
}

static gint
count_lines(const gchar *ptr)
{
    gint i;
    for(i=1; (ptr = strstr(ptr, "\r")); i++)
        ptr++;
    return i;
}

static void
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

static scan_header_t
parse_scan_header(const gchar *str)
{
    scan_header_t p = {0};
    gchar *ptr;

    if((ptr = strstr(str, "ADDRESS")))
        p.address = ptr-str;
    else
    {
        fprintf(stderr, "ADDRESS position not found!");
        return p;
    }

    if((ptr = strstr(str, "SSID")))
        p.ssid = ptr-str;
    else
        fprintf(stderr, "SSID position not found!");

    /* Post v6.30 */
    if((ptr = strstr(str, "CHANNEL")))
        p.channel = ptr-str;
    /* ---------- */

    /* Pre v6.30 */
    if((ptr = strstr(str, "BAND")))
        p.band = ptr-str;

    if((ptr = strstr(str, "CHANNEL-WIDTH")))
        p.channel_width = ptr-str;

    if((ptr = strstr(str, "FREQ")))
        p.frequency = ptr-str;
    /* --------- */

    if((ptr = strstr(str, "SIG")))
        p.rssi = ptr-str;
    else
        fprintf(stderr, "SIG position not found!");

    if((ptr = strstr(str, "NF")))
        p.noise = ptr-str;
    else
        fprintf(stderr, "NF position not found!");

    if((ptr = strstr(str, "SNR")))
        p.snr = ptr-str;
    else
        fprintf(stderr, "SNR position not found!");

    if((ptr = strstr(str, "RADIO-NAME")))
        p.radioname = ptr-str;
    else
        fprintf(stderr, "RADIO-NAME position not found!");

    if((ptr = strstr(str, "ROUTEROS-VERSION")))
        p.ros_version = ptr-str;
    else
        fprintf(stderr, "ROUTEROS-VERSION position not found!");

    return p;
}

static network_flags_t
parse_scan_flags(const gchar *str,
                 guint        length)
{
    network_flags_t flags = {0};
    gint i;
    for(i=0; i<length; i++)
    {
        switch(str[i])
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
            fprintf(stderr, "Unknown flag: '%c' (%d)\n", str[i], (int)str[i]);
            break;
        }
    }
    return flags;
}

static gchar*
parse_scan_address(const gchar *str,
                   guint        position)
{
    gchar *addr = g_malloc(sizeof(gchar)*13);

    if(strlen(str) > position &&
       sscanf(str+position, "%c%c:%c%c:%c%c:%c%c:%c%c:%c%c",
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
parse_scan_freq(const gchar *str,
                guint        position,
                guint        max_left_offset)
{
    size_t len = strlen(str);
    gint i;
    gfloat value = 0;
    if(len >= position+max_left_offset)
    {
        for(i=0; i<max_left_offset; i++)
        {
            if(isdigit(*(str+position+i)))
            {
                sscanf(str+position, "%f", &value);
                break;
            }
        }
    }
    return lround(value*1000);
}

static gint
parse_scan_int(const gchar *str,
               guint        position,
               guint        max_left_offset)
{
    size_t len = strlen(str);
    gint i, value = 0;
    if(len >= position+max_left_offset)
    {
        for(i=0; i<max_left_offset; i++)
        {
            if(isdigit(*(str+position+i)) || *(str+position+i) == '-')
            {
                sscanf(str+position, "%d", &value);
                break;
            }
        }
    }
    return value;
}

static gchar*
parse_scan_string(const gchar *str,
                  guint        position,
                  gint         length,
                  gint         check_for_term_sequences)
{
    gchar *output = NULL;
    size_t end, i;

    if(strlen(str) < position+length)
    {
        length = strlen(str)-position;
    }

    if(position > 0 && length > 0)
    {
        /* right-side space trim */
        end = position + length;
        while(end >= position && isspace(str[end]))
            end--;
        if(check_for_term_sequences)
        {
            for(i=position; i <= end; i++)
            {
                if(str[i] == 0x5B)
                {
                    return NULL;
                }
            }
        }
        output = g_strndup(str+position, end-position+1);
    }

    return output;
}


static gchar*
parse_scan_channel(const gchar *str,
                   guint        position)
{
    size_t len = strlen(str);
    gchar *ptr, *endptr, *channel = NULL;
    if(len > position)
    {
        if((ptr = strchr(str+position, '/')))
        {
            ptr++;
            if((endptr = strchr(ptr, '/')))
            {
                channel = g_strndup(ptr, (endptr-ptr));
            }
        }
    }
    return channel;
}

static gchar*
parse_scan_mode(const gchar *str,
                guint        position)
{
    size_t len = strlen(str);
    gchar *ptr, *endptr, *mode = NULL;
    if(len > position)
    {
        if((ptr = strchr(str+position, '/')))
        {
            ptr++;
            if((ptr = strchr(ptr, '/')))
            {
                ptr++;
                if((endptr = strchr(ptr, ' ')))
                {
                    mode = g_strndup(ptr, (endptr-ptr));
                }
            }
        }
    }
    return mode;
}
