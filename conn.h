#ifndef MTSCAN_CONN_H_
#define MTSCAN_CONN_H_
#include <gtk/gtk.h>

enum
{
    COMMAND_AUTH,
    COMMAND_DISCONNECT,
    COMMAND_SCAN_START,
    COMMAND_SCAN_STOP,
    COMMAND_SCAN_RESTART,
    COMMAND_GET_SCANLIST,
    COMMAND_SET_SCANLIST
};

enum
{
    CONN_MSG_CONNECTING,
    CONN_MSG_AUTHENTICATING,
    CONN_MSG_AUTH_VERIFY,
    CONN_MSG_AUTH_ERROR,
    CONN_MSG_CONNECTED,
    CONN_MSG_SCANNING,
    CONN_MSG_SCANNING_ERROR
};

enum
{
    CONN_CLOSED,
    CONN_ERR_SSH_NEW,
    CONN_ERR_SSH_SET_OPTIONS,
    CONN_ERR_SSH_CONNECT,
    CONN_ERR_SSH_VERIFY,
    CONN_ERR_SSH_AUTH,
    CONN_ERR_SSH_CHANNEL_NEW,
    CONN_ERR_SSH_CHANNEL_OPEN,
    CONN_ERR_SSH_CHANNEL_REQ_PTY_SIZE,
    CONN_ERR_SSH_CHANNEL_REQ_SHELL,
    CONN_STATE_UNKNOWN = G_MAXINT
};

typedef struct mtscan_conn
{
    gchar *hostname;
    gchar *port;
    gchar *login;
    gchar *password;
    gchar *iface;
    gint duration;
    gboolean remote_mode;
    gboolean background;

    gint return_state;
    gchar *err;

    volatile gboolean canceled;
    GAsyncQueue *queue;
} mtscan_conn_t;

typedef struct mtscan_conn_msg
{
    gint type;
    gpointer data;
    mtscan_conn_t *conn;
} mtscan_conn_msg_t;

typedef struct mtscan_conn_cmd
{
    gint type;
    gchar *data;
} mtscan_conn_cmd_t;

mtscan_conn_t* conn;

mtscan_conn_t* conn_new(const gchar*, gint, const gchar*, const gchar*, const gchar*, gint, gboolean, gboolean);
void conn_free(mtscan_conn_t*);

mtscan_conn_msg_t* conn_msg_new(gint, gpointer, mtscan_conn_t*);
void conn_msg_free(mtscan_conn_msg_t*);

mtscan_conn_cmd_t* conn_command_new(gint, gpointer);
void conn_command_free(mtscan_conn_cmd_t*);

#endif
