#ifndef MTSCAN_CONN_H_
#define MTSCAN_CONN_H_
#include <gtk/gtk.h>

#define STATE_WAITING_FOR_PROMPT 0
#define STATE_PROMPT             1
#define STATE_WAITING_FOR_SCAN   2
#define STATE_SCANNING           3

#define COMMAND_AUTH         0
#define COMMAND_DISCONNECT   1
#define COMMAND_SCAN_START   2
#define COMMAND_SCAN_STOP    3
#define COMMAND_SCAN_RESTART 4
#define COMMAND_SET_SCANLIST 5

#define CONN_MSG_CONNECTING                0
#define CONN_MSG_AUTHENTICATING            1
#define CONN_MSG_AUTH_VERIFY               2
#define CONN_MSG_AUTH_ERROR                3
#define CONN_MSG_CONNECTED                 4
#define CONN_MSG_SCANNING                  5
#define CONN_MSG_SCANNING_ERROR            6

#define CONN_STATE_UNKNOWN           G_MAXINT
#define CONN_CLOSED                         0
#define CONN_ERR_SSH_NEW                    1
#define CONN_ERR_SSH_SET_OPTIONS            2
#define CONN_ERR_SSH_CONNECT                3
#define CONN_ERR_SSH_VERIFY                 4
#define CONN_ERR_SSH_AUTH                   5
#define CONN_ERR_SSH_CHANNEL_NEW            6
#define CONN_ERR_SSH_CHANNEL_OPEN           7
#define CONN_ERR_SSH_CHANNEL_REQ_PTY_SIZE   8
#define CONN_ERR_SSH_CHANNEL_REQ_SHELL      9

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

    gint state;
    gint scan_line;

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
    gpointer data;
} mtscan_conn_cmd_t;

mtscan_conn_t* conn;

mtscan_conn_t* conn_new(const gchar*, gint, const gchar*, const gchar*, const gchar*, gint, gboolean, gboolean);
void conn_free(mtscan_conn_t*);

mtscan_conn_msg_t* conn_msg_new(gint, gpointer, mtscan_conn_t*);
void conn_msg_free(mtscan_conn_msg_t*);

mtscan_conn_cmd_t* conn_command_new(gint, gpointer);
void conn_command_free(mtscan_conn_cmd_t*);

#endif
