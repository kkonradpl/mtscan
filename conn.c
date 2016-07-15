#include "conn.h"

mtscan_conn_t*
conn_new(const gchar *hostname,
         gint         port,
         const gchar *login,
         const gchar *password,
         const gchar *interface,
         gint         duration,
         gboolean     remote,
         gboolean     background)
{
    mtscan_conn_t *ptr = g_new(mtscan_conn_t, 1);

    ptr->hostname = g_strdup(hostname);
    ptr->port = g_strdup_printf("%d", port);
    ptr->login = g_strdup(login);
    ptr->password = g_strdup(password);
    ptr->iface = g_strdup(interface);
    ptr->duration = duration;
    ptr->remote_mode = remote;
    ptr->background = background;

    ptr->state = STATE_WAITING_FOR_PROMPT;
    ptr->scan_line = -1;

    ptr->return_state = CONN_STATE_UNKNOWN;
    ptr->err = NULL;

    ptr->canceled = FALSE;
    ptr->queue = g_async_queue_new_full((GDestroyNotify)conn_command_free);

    return ptr;
}

void
conn_free(mtscan_conn_t *ptr)
{
    g_free(ptr->hostname);
    g_free(ptr->port);
    g_free(ptr->login);
    g_free(ptr->password);
    g_free(ptr->iface);
    g_free(ptr->err);
    g_async_queue_unref(ptr->queue);
    g_free(ptr);
}

mtscan_conn_msg_t*
conn_msg_new(gint     type,
             gpointer data,
             mtscan_conn_t  *conn)
{
    mtscan_conn_msg_t *ptr = g_new(mtscan_conn_msg_t, 1);
    ptr->type = type;
    ptr->data = data;
    ptr->conn = conn;
    return ptr;
}

void
conn_msg_free(mtscan_conn_msg_t *ptr)
{
    g_free(ptr->data);
    g_free(ptr);
}

mtscan_conn_cmd_t*
conn_command_new(gint type,
                 gpointer data)
{
    mtscan_conn_cmd_t *ptr = g_new(mtscan_conn_cmd_t, 1);
    ptr->type = type;
    ptr->data = data;
    return ptr;
}

void
conn_command_free(mtscan_conn_cmd_t *ptr)
{
    g_free(ptr->data);
    g_free(ptr);
}
