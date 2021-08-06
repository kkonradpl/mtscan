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

#include "model.h"
#include "ui-callbacks.h"
#include "ui-toolbar.h"
#include "ui-view.h"
#include "gps.h"
#include "ui-scanlist.h"
#include "ui-dialogs.h"
#include "conf.h"
#include "misc.h"
#include "tzsp-receiver.h"

static void ui_callback_network_real(network_t*);

static gboolean ui_callback_timeout(gpointer);
static gboolean ui_callback_heartbeat_timeout(gpointer);

void
ui_callback_status(const mt_ssh_t *context,
                   const gchar    *status,
                   const gchar    *extended_error)
{
    if(ui.conn != context)
        return;

    if(!ui.conn_dialog)
        return;

    ui_connection_set_status(ui.conn_dialog, status, extended_error);
}

void
ui_callback_verify(const mt_ssh_t *context,
                   const gchar    *data)
{
    gboolean verify;

    if(ui.conn != context)
        return;

    if(!ui.conn_dialog)
        return;

    verify = ui_connection_verify(ui.conn_dialog, data);

    if(ui.conn != context)
        return;

    if(verify)
        mt_ssh_cmd(ui.conn, MT_SSH_CMD_AUTH, NULL);
    else
        mt_ssh_cancel(ui.conn);
}

void
ui_callback_connected(const mt_ssh_t *context,
                      const gchar    *hwaddr)
{
    if(ui.conn != context)
        return;

    ui_connected(mt_ssh_get_name(context),
                 mt_ssh_get_login(context),
                 mt_ssh_get_hostname(context),
                 mt_ssh_get_interface(context),
                 hwaddr);

    if(ui.conn_dialog)
        ui_connection_connected(ui.conn_dialog);
}

void
ui_callback_disconnected(const mt_ssh_t *context,
                         gboolean        cancelled)
{
    ui_connection_mode_t mode;

    if(ui.conn != context)
        return;

    mode = (ui.active ? UI_CONNECTION_MODE_RECONNECT : UI_CONNECTION_MODE_RECONNECT_IDLE);

    ui_disconnected();
    ui.conn = NULL;

    if(ui.conn_dialog)
    {
        /* Reconnect will be handled there */
        ui_connection_disconnected(ui.conn_dialog);
    }
    else if(!cancelled &&
            conf_get_preferences_reconnect())
    {
        /* Try to reconnect in the background */
        ui.conn_dialog = ui_connection_new(-1, mode);
    }
}

void
ui_callback_state(const mt_ssh_t *context,
                  gint            value)
{
    if(ui.conn != context)
        return;

    ui.active = (gboolean)value;
    ui_toolbar_scan_set_state(ui.active);

    gtk_widget_set_sensitive(GTK_WIDGET(ui.b_scan), TRUE);
    gtk_widget_set_sensitive(GTK_WIDGET(ui.b_restart), TRUE);
    ui.activity_ts = UNIX_TIMESTAMP();

    if(ui.tzsp_rx)
    {
        if(value == MTSCAN_MODE_SNIFFER)
            tzsp_receiver_enable(ui.tzsp_rx);
        else if(value == MTSCAN_MODE_NONE)
            tzsp_receiver_disable(ui.tzsp_rx);
    }
}

void
ui_callback_failure(const mt_ssh_t *context,
                    const gchar    *error)
{
    if(context != ui.conn)
        return;

    ui_dialog(GTK_WINDOW(ui.window),
              GTK_MESSAGE_ERROR,
              APP_NAME,
              "<big><b>Command failure:</b></big>\n%s",
              error);
}

void
ui_callback_network(const mt_ssh_t *context,
                    network_t      *net)
{
    if(ui.conn != context)
    {
        network_free(net);
        g_free(net);
        return;
    }

    ui_callback_network_real(net);
}

static void
ui_callback_network_real(network_t *net)
{
    const mtscan_gps_data_t *gps_data;
    if(gps_get_data(&gps_data) == GPS_OK)
    {
        net->latitude = gps_data->lat;
        net->longitude = gps_data->lon;
    }

    if(conf_get_preferences_clip_invalid_signal())
        if(net->rssi <= -100 && net->rssi != MODEL_NO_SIGNAL)
            net->rssi = -99;

    mtscan_model_buffer_add(ui.model, net);
}

void
ui_callback_heartbeat(const mt_ssh_t *context)
{
    gint ret;

    if(ui.conn != context)
        return;

    gtk_widget_freeze_child_notify(ui.treeview);
    ret = mtscan_model_buffer_and_inactive_update(ui.model);

    if(ret & MODEL_UPDATE_NEW_ALARM ||
       ret & MODEL_UPDATE_NEW_HIGHLIGHT ||
       ret & MODEL_UPDATE_NEW)
    {
        ui_view_check_position(ui.treeview);
        if(conf_get_interface_sound())
        {
            if(conf_get_preferences_sounds_new_network_al() && (ret & MODEL_UPDATE_NEW_ALARM))
                mtscan_sound(APP_SOUND_NETWORK3);
            else if(conf_get_preferences_sounds_new_network_hi() && (ret & MODEL_UPDATE_NEW_HIGHLIGHT))
                mtscan_sound(APP_SOUND_NETWORK2);
            else if(conf_get_preferences_sounds_new_network() && (ret & MODEL_UPDATE_NEW))
                mtscan_sound(APP_SOUND_NETWORK);
        }
    }

    if(ret != MODEL_UPDATE_NONE)
    {
        if(ret != MODEL_UPDATE_ONLY_INACTIVE)
            ui_changed();
        ui_status_update_networks();
    }

    gtk_widget_thaw_child_notify(ui.treeview);

    ui.activity = ui.mode;
    ui.activity_ts = UNIX_TIMESTAMP();
    gtk_widget_queue_draw(ui.activity_icon);

    if(ui.activity_timeout)
        g_source_remove(ui.activity_timeout);
    ui.activity_timeout = g_timeout_add(500, ui_callback_heartbeat_timeout, &ui);

    if(ui.data_timeout)
        g_source_remove(ui.data_timeout);
    ui.data_timeout = g_timeout_add(ui.model->active_timeout * 1000, ui_callback_timeout, &ui);
}

static gboolean
ui_callback_timeout(gpointer user_data)
{
    mtscan_gtk_t *ui = (mtscan_gtk_t*)user_data;

    mtscan_model_clear_active(ui->model);
    ui_status_update_networks();
    ui->data_timeout = 0;
    return G_SOURCE_REMOVE;
}

static gboolean
ui_callback_heartbeat_timeout(gpointer user_data)
{
    mtscan_gtk_t *ui = (mtscan_gtk_t*)user_data;

    ui->activity = MTSCAN_MODE_NONE;
    gtk_widget_queue_draw(ui->activity_icon);
    ui->activity_timeout = 0;
    return G_SOURCE_REMOVE;
}

void
ui_callback_scanlist(const mt_ssh_t *context,
                     const gchar    *data)
{
    if(ui.conn != context)
        return;

    ui_scanlist_current(ui.scanlist, data);
}

void
ui_callback_tzsp(tzsp_receiver_t *context)
{
    if(ui.tzsp_rx == context)
        ui.tzsp_rx = NULL;

    tzsp_receiver_free(context);
}

void
ui_callback_tzsp_network(const tzsp_receiver_t *context,
                         network_t             *network)
{
    if(ui.tzsp_rx != context)
    {
        network_free(network);
        g_free(network);
        return;
    }

    ui_callback_network_real(network);
}

void
ui_callback_geoloc(gint64 addr)
{
    if(addr >= 0)
        mtscan_model_geoloc(ui.model, addr);
    else
    {
        ui_view_lock(ui.treeview);
        mtscan_model_geoloc_all(ui.model);
        ui_view_unlock(ui.treeview);
    }
}
