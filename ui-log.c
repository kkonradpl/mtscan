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

#include <gtk/gtk.h>
#include "ui.h"
#include "ui-view.h"
#include "ui-dialogs.h"
#include "log.h"
#include "conf.h"

typedef struct ui_log_open_context
{
    gboolean merge;
    gboolean changed;
} ui_log_open_context_t;

static void ui_log_open_net_cb(network_t*, gpointer);


void
ui_log_open(GSList   *list,
            gboolean  merge,
            gboolean  strip_samples)
{
    ui_log_open_context_t context;
    GSList *errors = NULL;
    const gchar *filename;
    gint count;
    GString *text;
    GSList *it;
    gchar *str;

    if(!list)
        return;

    if(!ui_can_discard_unsaved())
        return;

    context.merge = merge;
    context.changed = FALSE;

    if(!context.merge)
        ui_clear();

    ui_set_title(NULL);
    ui_view_lock(ui.treeview);

    while(list != NULL)
    {
        filename = (gchar*)list->data;

        count = log_read(filename,
                         ui_log_open_net_cb,
                         &context,
                         strip_samples);

        if(count <= 0)
        {
            switch(count)
            {
                case LOG_READ_ERROR_OPEN:
                    errors = g_slist_append(errors, g_markup_printf_escaped("<b>Failed to open a file:</b>\n%s", filename));
                    break;

                case LOG_READ_ERROR_READ:
                    errors = g_slist_append(errors, g_markup_printf_escaped("<b>Failed to read a file:</b>\n%s", filename));
                    break;

                case LOG_READ_ERROR_PARSE:
                case LOG_READ_ERROR_EMPTY:
                    errors = g_slist_append(errors, g_markup_printf_escaped("<b>Failed to parse a file:</b>\n%s", filename));
                    break;

                default:
                    errors = g_slist_append(errors, g_markup_printf_escaped("<b>Unknown error:</b>\n%s", filename));
                    break;
            }
        }
        else if(!context.merge)
            ui_set_title(filename);

        list = list->next;
    }

    if(context.changed)
    {
        if(conf_get_interface_geoloc())
            mtscan_model_geoloc_all(ui.model);
        ui_status_update_networks();
        if(context.merge)
            ui_changed();
    }

    ui_view_unlock(ui.treeview);

    if(errors)
    {
        text = g_string_new("<big><b>Some errors occurred:</b></big>");
        for(it = errors; it; it=it->next)
        {
            text = g_string_append(text, "\n\n");
            text = g_string_append(text, (gchar*)it->data);
        }

        str = g_string_free(text, FALSE);
        ui_dialog(GTK_WINDOW(ui.window), GTK_MESSAGE_ERROR, APP_NAME, str);
        g_free(str);

        g_slist_free_full(errors, g_free);
    }
}

static void
ui_log_open_net_cb(network_t *network,
                   gpointer   user_data)
{
    ui_log_open_context_t *context = (ui_log_open_context_t*)user_data;

    mtscan_model_add(ui.model,
                     network,
                     context->merge);

    context->changed = TRUE;
}

gboolean
ui_log_save(const gchar *filename,
            gboolean     strip_signals,
            gboolean     strip_gps,
            gboolean     strip_azi,
            GList       *iterlist,
            gboolean     show_message)
{
    log_save_error_t *error;
    error = log_save(filename, strip_signals, strip_gps, strip_azi, iterlist);

    if(error)
    {
        if(show_message)
        {
            if (error->length != error->wrote)
            {
                ui_dialog(GTK_WINDOW(ui.window),
                          GTK_MESSAGE_ERROR,
                          "Error",
                          "Unable to save a file:\n%s\n\nWrote only %d of %d uncompressed bytes so far.%s",
                          filename, error->wrote, error->length,
                          (error->existing_file) ? "\n\nThe existing file has been renamed." : "");
            }
            else
            {
                ui_dialog(GTK_WINDOW(ui.window),
                          GTK_MESSAGE_ERROR,
                          "Error",
                          "Unable to save a file:\n%s%s",
                          filename,
                          (error->existing_file) ? "\n\nThe existing file has been renamed." : "");
            }
        }
        g_free(error);
        return FALSE;
    }
    return TRUE;
}

gboolean
ui_log_save_full(const gchar *filename,
                 gboolean     strip_signals,
                 gboolean     strip_gps,
                 gboolean     strip_azi,
                 GList       *iterlist,
                 gboolean     show_message)
{
    if(ui_log_save(filename, strip_signals, strip_gps, strip_azi, iterlist, show_message))
    {
        /* update the window title */
        ui.changed = FALSE;
        ui.log_ts = UNIX_TIMESTAMP();
        ui_set_title(filename);
        return TRUE;
    }
    return FALSE;
}
