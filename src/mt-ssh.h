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

#ifndef MTSCAN_MT_SSH_H_
#define MTSCAN_MT_SSH_H_
#include <glib.h>

typedef struct mt_ssh      mt_ssh_t;
typedef struct mt_ssh_info mt_ssh_info_t;
typedef struct mt_ssh_net  mt_ssh_net_t;
typedef struct mt_ssh_snf  mt_ssh_snf_t;

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
    MT_SSH_ERR_CHANNEL_REQ_SHELL,
    MT_SSH_ERR_INTERFACE
} mt_ssh_ret_t;

typedef enum mt_ssh_msg_type
{
    MT_SSH_MSG_INFO,
    MT_SSH_MSG_NET,
    MT_SSH_MSG_SNF
} mt_ssh_msg_type_t;

typedef enum mt_ssh_info_type
{
    MT_SSH_INFO_CONNECTING,
    MT_SSH_INFO_AUTHENTICATING,
    MT_SSH_INFO_AUTH_VERIFY,
    MT_SSH_INFO_CONNECTED,
    MT_SSH_INFO_IDENTITY,
    MT_SSH_INFO_INTERFACE,
    MT_SSH_INFO_SCANLIST,
    MT_SSH_INFO_FAILURE,
    MT_SSH_INFO_HEARTBEAT,
    MT_SSH_INFO_SCANNER_START,
    MT_SSH_INFO_SCANNER_STOP,
    MT_SSH_INFO_SNIFFER_START,
    MT_SSH_INFO_SNIFFER_STOP
} mt_ssh_info_type_t;

typedef enum mt_ssh_cmd_type
{
    MT_SSH_CMD_AUTH,
    MT_SSH_CMD_SCANLIST,
    MT_SSH_CMD_STOP,
    MT_SSH_CMD_SCAN,
    MT_SSH_CMD_SNIFF
} mt_ssh_cmd_type_t;

typedef enum mt_ssh_mode
{
    MT_SSH_MODE_NONE,
    MT_SSH_MODE_SCANNER,
    MT_SSH_MODE_SNIFFER
} mt_ssh_mode_t;

mt_ssh_t*          mt_ssh_new(void         (*cb)(mt_ssh_t*, mt_ssh_ret_t, const gchar*),
                              void         (*cb_msg)(const mt_ssh_t*, mt_ssh_msg_type_t, gconstpointer),
                              mt_ssh_mode_t  mode_default,
                              const gchar   *name,
                              const gchar   *hostname,
                              gint           port,
                              const gchar   *login,
                              const gchar   *password,
                              const gchar   *iface,
                              gint           duration,
                              gboolean       remote,
                              gboolean       background,
                              gboolean       skip_verification);
void               mt_ssh_free(mt_ssh_t*);
void               mt_ssh_cancel(mt_ssh_t*);
void               mt_ssh_cmd(mt_ssh_t*, mt_ssh_cmd_type_t, const gchar*);
const gchar*       mt_ssh_get_name(const mt_ssh_t*);
const gchar*       mt_ssh_get_hostname(const mt_ssh_t*);
const gchar*       mt_ssh_get_port(const mt_ssh_t*);
const gchar*       mt_ssh_get_login(const mt_ssh_t*);
const gchar*       mt_ssh_get_password(const mt_ssh_t*);
const gchar*       mt_ssh_get_interface(const mt_ssh_t*);
gint               mt_ssh_get_duration(const mt_ssh_t*);
gboolean           mt_ssh_get_remote_mode(const mt_ssh_t*);
gboolean           mt_ssh_get_background(const mt_ssh_t*);

const gchar*       mt_ssh_get_identity(const mt_ssh_t*);

mt_ssh_info_type_t mt_ssh_info_get_type(const mt_ssh_info_t*);
const gchar*       mt_ssh_info_get_data(const mt_ssh_info_t*);

gint64             mt_ssh_net_get_timestamp(const mt_ssh_net_t*);
gint64             mt_ssh_net_get_address(const mt_ssh_net_t*);
gint               mt_ssh_net_get_frequency(const mt_ssh_net_t*);
const gchar*       mt_ssh_net_get_channel(const mt_ssh_net_t*);
const gchar*       mt_ssh_net_get_mode(const mt_ssh_net_t*);
const gchar*       mt_ssh_net_get_ssid(const mt_ssh_net_t*);
const gchar*       mt_ssh_net_get_radioname(const mt_ssh_net_t*);
gint8              mt_ssh_net_get_rssi(const mt_ssh_net_t*);
gint8              mt_ssh_net_get_noise(const mt_ssh_net_t*);
const gchar*       mt_ssh_net_get_routeros_ver(const mt_ssh_net_t*);
gboolean           mt_ssh_net_get_privacy(const mt_ssh_net_t*);
gboolean           mt_ssh_net_get_routeros(const mt_ssh_net_t*);
gboolean           mt_ssh_net_get_nstreme(const mt_ssh_net_t*);
gboolean           mt_ssh_net_get_tdma(const mt_ssh_net_t*);
gboolean           mt_ssh_net_get_wds(const mt_ssh_net_t*);
gboolean           mt_ssh_net_get_bridge(const mt_ssh_net_t*);

gint               mt_ssh_snf_get_processed_packets(const mt_ssh_snf_t*);
gint               mt_ssh_snf_get_memory_size(const mt_ssh_snf_t*);
gint               mt_ssh_snf_get_memory_saved_packets(const mt_ssh_snf_t*);
gint               mt_ssh_snf_get_memory_over_limit_packets(const mt_ssh_snf_t*);
gint               mt_ssh_snf_get_stream_dropped_packets(const mt_ssh_snf_t*);
gint               mt_ssh_snf_get_stream_sent_packets(const mt_ssh_snf_t*);
gint               mt_ssh_snf_get_real_file_limit(const mt_ssh_snf_t*);
gint               mt_ssh_snf_get_real_memory_limit(const mt_ssh_snf_t*);

#endif
