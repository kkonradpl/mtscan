#include <math.h>
#include "model.h"
#include "ui-update.h"
#include "ui-toolbar.h"
#include "ui-view.h"
#include "mt-ssh.h"
#include "gps.h"

static gboolean ui_update_heartbeat(gpointer);
static gboolean ui_update_timeout(gpointer);

gboolean
ui_update_scanning_state(gpointer data)
{
    ui.scanning = GPOINTER_TO_INT(data);
    ui_toolbar_scan_set_state(ui.scanning);
    if(ui.scanning)
        gtk_widget_set_sensitive(GTK_WIDGET(ui.b_restart), TRUE);
    else
        mtscan_model_buffer_clear(ui.model);
    gtk_widget_set_sensitive(GTK_WIDGET(ui.b_scan), TRUE);
    return FALSE;
}

gboolean
ui_update_network(gpointer data)
{
    network_t *net = (network_t*)data;
    const mtscan_gps_data_t *gps_data;

    if(gps_get_data(&gps_data) == GPS_OK)
    {
        net->latitude = gps_data->lat;
        net->longitude = gps_data->lon;
    }
    else
    {
        net->latitude = NAN;
        net->longitude = NAN;
    }
    net->azimuth = NAN;
    net->signals = NULL;
    mtscan_model_buffer_add(ui.model, net);
    return FALSE;
}

gboolean
ui_update_finish(gpointer data)
{
    static guint timeout_id = 0;

    gtk_widget_freeze_child_notify(ui.treeview);
    switch(mtscan_model_buffer_and_inactive_update(ui.model))
    {
        case MODEL_UPDATE_NEW:
            ui_view_check_position(ui.treeview);
            ui_play_sound(APP_SOUND_NETWORK);
        case MODEL_UPDATE:
            ui_changed();
        case MODEL_UPDATE_ONLY_INACTIVE:
            ui_status_update_networks();
            break;
    }
    gtk_widget_thaw_child_notify(ui.treeview);
    ui_update_heartbeat(GINT_TO_POINTER(TRUE));

    if(timeout_id)
        g_source_remove(timeout_id);
    timeout_id = g_timeout_add(ui.model->active_timeout*1000, ui_update_timeout, &timeout_id);

    return FALSE;
}

static gboolean
ui_update_heartbeat(gpointer data)
{
    static guint timeout_id = 0;
    ui.heartbeat_status = GPOINTER_TO_INT(data);
    gtk_widget_queue_draw(ui.heartbeat);

    if(ui.heartbeat_status)
    {
        if(timeout_id)
            g_source_remove(timeout_id);
        timeout_id = g_timeout_add(500, ui_update_heartbeat, GINT_TO_POINTER(FALSE));
    }
    else
    {
        timeout_id = 0;
    }
    return FALSE;
}

static gboolean
ui_update_timeout(gpointer data)
{
    guint *timeout_id = (guint*)data;
    mtscan_model_clear_active(ui.model);
    ui_status_update_networks();
    *timeout_id = 0;
    return FALSE;
}
