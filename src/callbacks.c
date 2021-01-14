/*
 *  MTscan - MikroTik RouterOS wireless scanner
 *  Copyright (c) 2015-2019  Konrad Kosmatka
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

#include "mt-ssh.h"
#include "ui-callbacks.h"

static void callback_mt_ssh_info(const mt_ssh_t *, const mt_ssh_info_t *);
static void callback_mt_ssh_net(const mt_ssh_t *, const mt_ssh_net_t *);
static void callback_mt_ssh_snf(const mt_ssh_t *, const mt_ssh_snf_t *);

void
callback_mt_ssh(mt_ssh_t     *context,
                mt_ssh_ret_t  return_state,
                const gchar  *return_error)
{
    switch(return_state)
    {
        case MT_SSH_CLOSED:
        case MT_SSH_CANCELED:
            break;

        case MT_SSH_ERR_NEW:
            ui_callback_status(context, "libssh init error", NULL);
            break;

        case MT_SSH_ERR_SET_OPTIONS:
            ui_callback_status(context, "libssh set options error", NULL);
            break;

        case MT_SSH_ERR_CONNECT:
            ui_callback_status(context, (return_error ? return_error : "Unable to connect."), NULL);
            break;

        case MT_SSH_ERR_VERIFY:
            ui_callback_status(context, "Unable to verify the SSH server.", return_error);
            break;

        case MT_SSH_ERR_AUTH:
            ui_callback_status(context, "Permission denied, please try again.", NULL);
            break;

        case MT_SSH_ERR_CHANNEL_NEW:
            ui_callback_status(context, "libssh channel init error", NULL);
            break;

        case MT_SSH_ERR_CHANNEL_OPEN:
            ui_callback_status(context, "libssh channel open error", NULL);
            break;

        case MT_SSH_ERR_CHANNEL_REQ_PTY_SIZE:
            ui_callback_status(context, "libssh channel request pty size error", NULL);
            break;

        case MT_SSH_ERR_CHANNEL_REQ_SHELL:
            ui_callback_status(context, "libssh channel request shell error", NULL);
            break;

        case MT_SSH_ERR_INTERFACE:
            ui_callback_status(context, "Invalid wireless interface", NULL);
            break;

        default:
            break;
    }

    ui_callback_disconnected(context, (return_state == MT_SSH_CANCELED));
    mt_ssh_free(context);
}

void
callback_mt_ssh_msg(const mt_ssh_t    *context,
                    mt_ssh_msg_type_t  type,
                    gconstpointer      data)
{
    switch(type)
    {
        case MT_SSH_MSG_INFO:
            callback_mt_ssh_info(context, data);
            break;

        case MT_SSH_MSG_NET:
            callback_mt_ssh_net(context, data);
            break;

        case MT_SSH_MSG_SNF:
            callback_mt_ssh_snf(context, data);
            break;
    }
}


static void
callback_mt_ssh_info(const mt_ssh_t      *context,
                     const mt_ssh_info_t *info)
{
    const gchar *data = mt_ssh_info_get_data(info);

    switch(mt_ssh_info_get_type(info))
    {
        case MT_SSH_INFO_CONNECTING:
            ui_callback_status(context, "Connecting...", NULL);
            break;

        case MT_SSH_INFO_AUTHENTICATING:
            ui_callback_status(context, "Authenticating...", NULL);
            break;

        case MT_SSH_INFO_AUTH_VERIFY:
            ui_callback_verify(context, data);
            break;

        case MT_SSH_INFO_CONNECTED:
            ui_callback_status(context, "Waiting for prompt...", NULL);
            break;

        case MT_SSH_INFO_IDENTITY:
            ui_callback_status(context, "Checking wireless interface...", NULL);
            break;

        case MT_SSH_INFO_INTERFACE:
            ui_callback_connected(context, data);
            break;

        case MT_SSH_INFO_SCANLIST:
            ui_callback_scanlist(context, data);
            break;

        case MT_SSH_INFO_HEARTBEAT:
            ui_callback_heartbeat(context);
            break;

        case MT_SSH_INFO_FAILURE:
            ui_callback_failure(context, data);
            ui_callback_state(context, MTSCAN_MODE_NONE);
            break;

        case MT_SSH_INFO_SCANNER_START:
            ui_callback_state(context, MTSCAN_MODE_SCANNER);
            break;

        case MT_SSH_INFO_SCANNER_STOP:
            ui_callback_state(context, MTSCAN_MODE_NONE);
            break;

        case MT_SSH_INFO_SNIFFER_START:
            ui_callback_state(context, MTSCAN_MODE_SNIFFER);
            break;

        case MT_SSH_INFO_SNIFFER_STOP:
            ui_callback_state(context, MTSCAN_MODE_NONE);
            break;

        default:
            break;
    }
}

static void
callback_mt_ssh_net(const mt_ssh_t     *context,
                    const mt_ssh_net_t *data)
{
    network_t *net = g_malloc(sizeof(network_t));
    network_init(net);
    net->address = mt_ssh_net_get_address(data);
    net->frequency = mt_ssh_net_get_frequency(data);
    net->channel = g_strdup(mt_ssh_net_get_channel(data));
    net->mode = g_strdup(mt_ssh_net_get_mode(data));
    net->ssid = g_strdup(mt_ssh_net_get_ssid(data));
    net->radioname = g_strdup(mt_ssh_net_get_radioname(data));
    net->rssi = mt_ssh_net_get_rssi(data);
    net->noise = mt_ssh_net_get_noise(data);
    net->routeros_ver = g_strdup(mt_ssh_net_get_routeros_ver(data));
    net->flags.privacy = mt_ssh_net_get_privacy(data);
    net->flags.routeros = mt_ssh_net_get_routeros(data);
    net->flags.nstreme = mt_ssh_net_get_nstreme(data);
    net->flags.tdma = mt_ssh_net_get_tdma(data);
    net->flags.wds = mt_ssh_net_get_wds(data);
    net->flags.bridge = mt_ssh_net_get_bridge(data);
    net->firstseen = mt_ssh_net_get_timestamp(data);
    net->lastseen = mt_ssh_net_get_timestamp(data);

    ui_callback_network(context, net);
}

static void
callback_mt_ssh_snf(const mt_ssh_t     *context,
                    const mt_ssh_snf_t *data)
{


}
