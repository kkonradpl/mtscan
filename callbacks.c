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

#include "mt-ssh.h"
#include "ui-callbacks.h"

void
callback_mt_ssh(mt_ssh_t *context)
{
    const gchar* err = mt_ssh_get_return_error(context);

    switch(mt_ssh_get_return_state(context))
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
            ui_callback_status(context, (err ? err : "Unable to connect."), NULL);
            break;

        case MT_SSH_ERR_VERIFY:
            ui_callback_status(context, "Unable to verify the SSH server.", err);
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

        default:
            break;
    }

    ui_callback_disconnected(context);
    mt_ssh_free(context);
}

void
callback_mt_ssh_msg(const mt_ssh_msg_t *message)
{
    const mt_ssh_t *context = mt_ssh_msg_get_context(message);
    const gchar *data = mt_ssh_msg_get_data(message);

    switch(mt_ssh_msg_get_type(message))
    {
        case MT_SSH_MSG_CONNECTING:
            ui_callback_status(context, "Connecting...", NULL);
            break;

        case MT_SSH_MSG_AUTHENTICATING:
            ui_callback_status(context, "Authenticating...", NULL);
            break;

        case MT_SSH_MSG_AUTH_VERIFY:
            ui_callback_verify(context, data);
            break;

        case MT_SSH_MSG_CONNECTED:
            ui_callback_status(context, "Waiting for a prompt...", NULL);
            break;

        case MT_SSH_MSG_IDENTITY:
            ui_callback_connected(context, data);
            break;

        case MT_SSH_MSG_SCANLIST:
            ui_callback_scanlist(context, data);
            break;

        case MT_SSH_MSG_HEARTBEAT:
            ui_callback_heartbeat(context);
            break;

        case MT_SSH_MSG_FAILURE:
            ui_callback_scanning_error(context, data);
            ui_callback_scanning_state(context, FALSE);
            break;

        case MT_SSH_MSG_SCANNER_START:
            ui_callback_scanning_state(context, TRUE);
            break;

        case MT_SSH_MSG_SCANNER_STOP:
            ui_callback_scanning_state(context, FALSE);
            break;

        default:
            break;
    }
}

void
callback_mt_ssh_net(const mt_ssh_net_t *data)
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

    ui_callback_network(mt_ssh_net_get_context(data), net);
}