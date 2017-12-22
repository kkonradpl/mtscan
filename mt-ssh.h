#ifndef MTSCAN_MT_SSH_H_
#define MTSCAN_MT_SSH_H_
#include <glib.h>

typedef struct mt_ssh     mt_ssh_t;
typedef struct mt_ssh_msg mt_ssh_msg_t;
typedef struct mt_ssh_net mt_ssh_net_t;

typedef enum mt_ssh_ret
{
    MT_SSH_INVALID,
    MT_SSH_CLOSED,
    MT_SSH_CANCELED,
    MT_SSH_ERR_NEW,
    MT_SSH_ERR_SET_OPTIONS,
    MT_SSH_ERR_CONNECT,
    MT_SSH_ERR_VERIFY,
    MT_SSH_ERR_AUTH,
    MT_SSH_ERR_CHANNEL_NEW,
    MT_SSH_ERR_CHANNEL_OPEN,
    MT_SSH_ERR_CHANNEL_REQ_PTY_SIZE,
    MT_SSH_ERR_CHANNEL_REQ_SHELL
} mt_ssh_ret_t;

typedef enum mt_ssh_msg_type
{
    MT_SSH_MSG_CONNECTING,
    MT_SSH_MSG_AUTHENTICATING,
    MT_SSH_MSG_AUTH_VERIFY,
    MT_SSH_MSG_CONNECTED,
    MT_SSH_MSG_IDENTITY,
    MT_SSH_MSG_SCANLIST,
    MT_SSH_MSG_FAILURE,
    MT_SSH_MSG_HEARTBEAT,
    MT_SSH_MSG_SCANNER_START,
    MT_SSH_MSG_SCANNER_STOP,
} mt_ssh_msg_type_t;

typedef enum mt_ssh_cmd_type
{
    MT_SSH_CMD_AUTH,
    MT_SSH_CMD_SCANLIST,
    MT_SSH_CMD_STOP,
    MT_SSH_CMD_SCAN,
} mt_ssh_cmd_type_t;

typedef enum mt_ssh_mode
{
    MT_SSH_MODE_NONE,
    MT_SSH_MODE_SCANNER
} mt_ssh_mode_t;

mt_ssh_t*         mt_ssh_new(void         (*cb)(mt_ssh_t*),
                             void         (*cb_msg)(const mt_ssh_msg_t*),
                             void         (*cb_net)(const mt_ssh_net_t*),
                             mt_ssh_mode_t  mode_default,
                             const gchar   *hostname,
                             gint           port,
                             const gchar   *login,
                             const gchar   *password,
                             const gchar   *interface,
                             gint           duration,
                             gboolean       remote,
                             gboolean       background);
void              mt_ssh_free(mt_ssh_t*);
void              mt_ssh_cancel(mt_ssh_t*);
void              mt_ssh_cmd(mt_ssh_t*, mt_ssh_cmd_type_t, const gchar*);
const gchar*      mt_ssh_get_hostname(const mt_ssh_t*);
const gchar*      mt_ssh_get_port(const mt_ssh_t*);
const gchar*      mt_ssh_get_login(const mt_ssh_t*);
const gchar*      mt_ssh_get_password(const mt_ssh_t*);
const gchar*      mt_ssh_get_interface(const mt_ssh_t*);
gint              mt_ssh_get_duration(const mt_ssh_t*);
gboolean          mt_ssh_get_remote_mode(const mt_ssh_t*);
gboolean          mt_ssh_get_background(const mt_ssh_t*);
mt_ssh_ret_t      mt_ssh_get_return_state(mt_ssh_t*);
const gchar*      mt_ssh_get_return_error(mt_ssh_t*);

const mt_ssh_t*   mt_ssh_msg_get_context(const mt_ssh_msg_t*);
mt_ssh_msg_type_t mt_ssh_msg_get_type(const mt_ssh_msg_t*);
const gchar*      mt_ssh_msg_get_data(const mt_ssh_msg_t*);

const mt_ssh_t*   mt_ssh_net_get_context(const mt_ssh_net_t*);
gint64            mt_ssh_net_get_timestamp(const mt_ssh_net_t*);
gint64            mt_ssh_net_get_address(const mt_ssh_net_t*);
gint              mt_ssh_net_get_frequency(const mt_ssh_net_t*);
const gchar*      mt_ssh_net_get_channel(const mt_ssh_net_t*);
const gchar*      mt_ssh_net_get_mode(const mt_ssh_net_t*);
const gchar*      mt_ssh_net_get_ssid(const mt_ssh_net_t*);
const gchar*      mt_ssh_net_get_radioname(const mt_ssh_net_t*);
gint8             mt_ssh_net_get_rssi(const mt_ssh_net_t*);
gint8             mt_ssh_net_get_noise(const mt_ssh_net_t*);
const gchar*      mt_ssh_net_get_routeros_ver(const mt_ssh_net_t*);
gboolean          mt_ssh_net_get_privacy(const mt_ssh_net_t*);
gboolean          mt_ssh_net_get_routeros(const mt_ssh_net_t*);
gboolean          mt_ssh_net_get_nstreme(const mt_ssh_net_t*);
gboolean          mt_ssh_net_get_tdma(const mt_ssh_net_t*);
gboolean          mt_ssh_net_get_wds(const mt_ssh_net_t*);
gboolean          mt_ssh_net_get_bridge(const mt_ssh_net_t*);

#endif
