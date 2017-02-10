#define _GNU_SOURCE
#include <string.h>
#include <math.h>
#include "model.h"
#include "signals.h"
#include "ui-icons.h"
#include "conf.h"

#define UNIX_TIMESTAMP() (g_get_real_time() / 1000000)
#define GPS_DOUBLE_PREC 1e-6

static const GdkColor new_network_color      = { 0, 0x9000, 0xEE00, 0x9000 };
static const GdkColor new_network_color_dark = { 0, 0x2F00, 0x4F00, 0x2F00 };

static gint model_sort_ascii_string(GtkTreeModel*, GtkTreeIter*, GtkTreeIter*, gpointer);
static gint model_sort_rssi(GtkTreeModel*, GtkTreeIter*, GtkTreeIter*, gpointer);
static gint model_sort_gps(GtkTreeModel*, GtkTreeIter*, GtkTreeIter*, gpointer);
static gint model_sort_version(GtkTreeModel*, GtkTreeIter*, GtkTreeIter*, gpointer);
static void model_free_foreach(gpointer, gpointer, gpointer);
static gboolean model_clear_active_foreach(gpointer, gpointer, gpointer);
static gboolean model_update_network(mtscan_model_t*, network_t*);

mtscan_model_t*
mtscan_model_new()
{
    mtscan_model_t *model = g_malloc(sizeof(mtscan_model_t));
    model->store = gtk_list_store_new(COL_COUNT,
                                      G_TYPE_INT,      /* COL_STATE     */
                                      GDK_TYPE_PIXBUF, /* COL_ICON      */
                                      GDK_TYPE_COLOR,  /* COL_BG        */
                                      G_TYPE_STRING,   /* COL_ADDRESS   */
                                      G_TYPE_INT,      /* COL_FREQUENCY */
                                      G_TYPE_STRING,   /* COL_CHANNEL   */
                                      G_TYPE_STRING,   /* COL_MODE      */
                                      G_TYPE_STRING,   /* COL_SSID      */
                                      G_TYPE_STRING,   /* COL_RADIONAME */
                                      G_TYPE_INT,      /* COL_MAXRSSI   */
                                      G_TYPE_INT,      /* COL_RSSI      */
                                      G_TYPE_INT,      /* COL_NOISE     */
                                      G_TYPE_BOOLEAN,  /* COL_PRIVACY   */
                                      G_TYPE_BOOLEAN,  /* COL_ROUTEROS  */
                                      G_TYPE_BOOLEAN,  /* COL_NSTREME   */
                                      G_TYPE_BOOLEAN,  /* COL_TDMA      */
                                      G_TYPE_BOOLEAN,  /* COL_WDS       */
                                      G_TYPE_BOOLEAN,  /* COL_BRIDGE    */
                                      G_TYPE_STRING,   /* COL_ROS_VER   */
                                      G_TYPE_INT64,    /* COL_FIRSTSEEN */
                                      G_TYPE_INT64,    /* COL_LASTSEEN  */
                                      G_TYPE_DOUBLE,   /* COL_LATITUDE  */
                                      G_TYPE_DOUBLE,   /* COL_LONGITUDE */
                                      G_TYPE_POINTER); /* COL_SIGNALS   */

    gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(model->store), COL_SSID, model_sort_ascii_string, GINT_TO_POINTER(COL_SSID), NULL);
    gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(model->store), COL_RADIONAME, model_sort_ascii_string, GINT_TO_POINTER(COL_RADIONAME), NULL);
    gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(model->store), COL_RSSI, model_sort_rssi, NULL, NULL);
    gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(model->store), COL_LATITUDE, model_sort_gps, GINT_TO_POINTER(COL_LATITUDE), NULL);
    gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(model->store), COL_LONGITUDE, model_sort_gps, GINT_TO_POINTER(COL_LONGITUDE), NULL);
    gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(model->store), COL_ROUTEROS_VER, model_sort_version, GINT_TO_POINTER(COL_ROUTEROS_VER), NULL);

    model->map = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)gtk_tree_iter_free);
    model->active = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
    model->active_timeout = MODEL_DEFAULT_ACTIVE_TIMEOUT;
    model->new_timeout = MODEL_DEFAULT_NEW_TIMEOUT;
    model->disabled_sorting = FALSE;
    model->buffer = NULL;
	model->clear_active_all = FALSE;
    return model;
}


static gint
model_sort_ascii_string(GtkTreeModel *model,
                        GtkTreeIter  *a,
                        GtkTreeIter  *b,
                        gpointer      data)
{
    gint column = GPOINTER_TO_INT(data);
    gchar *v1, *v2;
    gint ret;

    gtk_tree_model_get(model, a,
                       column, &v1,
                       -1);

    gtk_tree_model_get(model, b,
                       column, &v2,
                       -1);

    /* Don't care about UTF-8 chars now,
       as these are escaped by RouterOS */
    ret = g_ascii_strcasecmp(v1, v2);

    g_free(v1);
    g_free(v2);
    return ret;
}

static gint
model_sort_rssi(GtkTreeModel *model,
                GtkTreeIter  *a,
                GtkTreeIter  *b,
                gpointer      data)
{
    gint state, state2;
    gint rssi, rssi2;
    gint64 lastseen, lastseen2;

    gtk_tree_model_get(model, a,
                       COL_STATE, &state,
                       COL_RSSI, &rssi,
                       COL_LASTSEEN, &lastseen,
                       -1);

    gtk_tree_model_get(model, b,
                       COL_STATE, &state2,
                       COL_RSSI, &rssi2,
                       COL_LASTSEEN, &lastseen2,
                       -1);

    if(state == MODEL_STATE_INACTIVE && state2 == MODEL_STATE_INACTIVE)
    {
        if(lastseen - lastseen2 == 0)
            return rssi - rssi2;
        else
            return lastseen - lastseen2;
    }
    else if(state != MODEL_STATE_INACTIVE && state2 == MODEL_STATE_INACTIVE)
    {
        return 1;
    }
    else if(state == MODEL_STATE_INACTIVE && state2 != MODEL_STATE_INACTIVE)
    {
        return -1;
    }
    else
    {
        return rssi - rssi2;
    }
}

static gint
model_sort_gps(GtkTreeModel *model,
               GtkTreeIter  *a,
               GtkTreeIter  *b,
               gpointer      data)
{
    gint column = GPOINTER_TO_INT(data);
    gdouble v1, v2, diff;
    gboolean v1_isnan, v2_isnan;

    gtk_tree_model_get(model, a,
                       column, &v1,
                       -1);

    gtk_tree_model_get(model, b,
                       column, &v2,
                       -1);

    v1_isnan = isnan(v1);
    v2_isnan = isnan(v2);

    if(v1_isnan && v2_isnan)
        return 0;

    if(!v1_isnan && v2_isnan)
        return -1;

    if(v1_isnan && !v2_isnan)
        return 1;

    diff = v1 - v2;
    if(fabs(diff) < GPS_DOUBLE_PREC)
        return 0;

    if(diff < 0)
        return -1;

    return 1;
}

static gint
model_sort_version(GtkTreeModel *model,
                   GtkTreeIter  *a,
                   GtkTreeIter  *b,
                   gpointer      data)
{
    gint column = GPOINTER_TO_INT(data);
    gchar *v1, *v2;
    gint ret;

    gtk_tree_model_get(model, a,
                       column, &v1,
                       -1);

    gtk_tree_model_get(model, b,
                       column, &v2,
                       -1);
#ifndef G_OS_WIN32
    ret = strverscmp(v1, v2);
#else
    ret = strcasecmp(v1, v2);
#endif
    g_free(v1);
    g_free(v2);
    return ret;
}

void
mtscan_model_free(mtscan_model_t *model)
{
    g_hash_table_foreach(model->map, model_free_foreach, model);
    g_hash_table_destroy(model->map);
    g_hash_table_destroy(model->active);
    g_object_unref(model->store);
    g_free(model);
}

static void
model_free_foreach(gpointer key,
                   gpointer value,
                   gpointer data)
{
    mtscan_model_t *model = (mtscan_model_t*)data;
    GtkTreeModel *store = (GtkTreeModel*)model->store;
    GtkTreeIter *iter = (GtkTreeIter*)value;
    signals_t *signals;

    gtk_tree_model_get(store, iter,
                       COL_SIGNALS, &signals, -1);

    signals_free(signals);
}

void
mtscan_model_clear(mtscan_model_t *model)
{
    mtscan_model_buffer_clear(model);
    g_hash_table_remove_all(model->active);
    g_hash_table_foreach(model->map, model_free_foreach, model);
    g_hash_table_remove_all(model->map);
    gtk_list_store_clear(GTK_LIST_STORE(model->store));
}

void
mtscan_model_clear_active(mtscan_model_t *model)
{
    model->clear_active_all = TRUE;
	g_hash_table_foreach_remove(model->active, model_clear_active_foreach, model);
	model->clear_active_all = FALSE;
}

static gboolean
model_clear_active_foreach(gpointer key,
                           gpointer value,
                           gpointer data)
{
    mtscan_model_t *model = (mtscan_model_t*)data;
    GtkTreeModel *store = (GtkTreeModel*)model->store;
    GtkTreeIter *iter = (GtkTreeIter*)value;
    gint64 firstseen, lastseen;
    gboolean privacy;
    gint rssi;
    gint state;

    gtk_tree_model_get(store, iter,
                       COL_STATE, &state,
                       COL_PRIVACY, &privacy,
                       COL_RSSI, &rssi,
                       COL_FIRSTSEEN, &firstseen,
                       COL_LASTSEEN, &lastseen,
                       -1);

    if(model->clear_active_all ||
       UNIX_TIMESTAMP() > lastseen+model->active_timeout)
    {
        gtk_list_store_set(GTK_LIST_STORE(store), iter,
                           COL_STATE, MODEL_STATE_INACTIVE,
                           COL_ICON, ui_icon(MODEL_NO_SIGNAL, privacy),
                           COL_BG, NULL,
                           -1);
        model->clear_active_changed = TRUE;
        return TRUE;
    }

    if(state == MODEL_STATE_NEW &&
       UNIX_TIMESTAMP() > firstseen+model->new_timeout)
    {
        gtk_list_store_set(GTK_LIST_STORE(store), iter,
                           COL_STATE, MODEL_STATE_ACTIVE,
                           COL_BG, NULL,
                           -1);
        model->clear_active_changed = TRUE;
    }

    return FALSE;
}

void
mtscan_model_remove(mtscan_model_t *model,
                    GtkTreeIter    *iter)
{
    gchar *address;
    signals_t *signals;

    gtk_tree_model_get(GTK_TREE_MODEL(model->store), iter,
                       COL_ADDRESS, &address,
                       COL_SIGNALS, &signals,
                       -1);

    g_hash_table_remove(model->active, address);
    g_hash_table_remove(model->map, address);
    g_free(address);
    signals_free(signals);
    gtk_list_store_remove(model->store, iter);
}

void
mtscan_model_buffer_add(mtscan_model_t *model,
                        network_t      *net)
{
    model->buffer = g_slist_prepend(model->buffer, (gpointer)net);
}

void
mtscan_model_buffer_clear(mtscan_model_t *model)
{
    GSList *current = model->buffer;
    while(current)
    {
        network_t *net = (network_t*)(current->data);
        network_free(net);
        g_free(net);
        current = current->next;
    }
    g_slist_free(model->buffer);
    model->buffer = NULL;
}

gint
mtscan_model_buffer_and_inactive_update(mtscan_model_t *model)
{
    GSList *current;
    gint state;

    if(!model->buffer)
        state = MODEL_UPDATE_NONE;
    else
    {
        current = model->buffer;
        state = MODEL_UPDATE;
        while(current)
        {
            network_t* net = (network_t*)(current->data);
            if(model_update_network(model, net))
                state = MODEL_UPDATE_NEW;
            network_free(net);
            g_free(net);
            current = current->next;
        }
        g_slist_free(model->buffer);
        model->buffer = NULL;
    }

    model->clear_active_changed = FALSE;
	g_hash_table_foreach_remove(model->active, model_clear_active_foreach, model);
    if(model->clear_active_changed && state == MODEL_UPDATE_NONE)
        state = MODEL_UPDATE_ONLY_INACTIVE;

    return state;
}

static gboolean
model_update_network(mtscan_model_t *model,
                     network_t      *net)
{
    GtkTreeIter *iter_ptr;
    GtkTreeIter iter;
    gchar *address;
    gchar *current_ssid;
    gint current_maxrssi;
    gint current_state;
    gboolean new_network_found;

    if(g_hash_table_lookup_extended(model->map, net->address, (gpointer*)&address, (gpointer*)&iter_ptr))
    {
        /* Update a network, check current values first */
        gtk_tree_model_get(GTK_TREE_MODEL(model->store), iter_ptr,
                           COL_STATE, &current_state,
                           COL_SSID, &current_ssid,
                           COL_MAXRSSI, &current_maxrssi,
                           COL_SIGNALS, &net->signals,
                           -1);

        /* Update state to active (keep MODEL_STATE_NEW untouched) */
        if(current_state == MODEL_STATE_INACTIVE)
            current_state = MODEL_STATE_ACTIVE;

        /* Preserve hidden SSIDs */
        if(current_ssid && current_ssid[0] &&
           net->ssid && !net->ssid[0])
        {
            g_free(net->ssid);
            net->ssid = current_ssid;
        }
        else
        {
            g_free(current_ssid);
        }

#if MIKROTIK_LOW_SIGNAL_BUGFIX
        if(net->rssi >= -12 && net->rssi <= -10 && maxrssi <= -15)
            net->rssi = -100;
#endif
#if MIKROTIK_HIGH_SIGNAL_BUGFIX
        if(net->rssi >= 10)
            net->rssi = maxrssi;
#endif

        if(conf_get_interface_signals())
            signals_append(net->signals, signals_node_new(net->firstseen, net->rssi, net->latitude, net->longitude));

        /* At new signal peak, update additionally COL_MAXRSSI, COL_LATITUDE and COL_LONGITUDE */
        if(net->rssi > current_maxrssi)
        {
            gtk_list_store_set(model->store, iter_ptr,
                               COL_STATE, current_state,
                               COL_ICON, ui_icon(net->rssi, net->flags.privacy),
                               COL_FREQUENCY, net->frequency,
                               COL_CHANNEL, (net->channel ? net->channel : ""),
                               COL_MODE, (net->mode ? net->mode : ""),
                               COL_SSID, (net->ssid ? net->ssid : ""),
                               COL_RADIONAME, (net->radioname ? net->radioname : ""),
                               COL_MAXRSSI, net->rssi,
                               COL_RSSI, net->rssi,
                               COL_NOISE, net->noise,
                               COL_PRIVACY, net->flags.privacy,
                               COL_ROUTEROS, net->flags.routeros,
                               COL_NSTREME, net->flags.nstreme,
                               COL_TDMA, net->flags.tdma,
                               COL_WDS, net->flags.wds,
                               COL_BRIDGE, net->flags.bridge,
                               COL_ROUTEROS_VER, (net->routeros_ver ? net->routeros_ver : ""),
                               COL_LASTSEEN, net->firstseen,
                               COL_LATITUDE, net->latitude,
                               COL_LONGITUDE, net->longitude,
                               -1);
        }
        else
        {
            gtk_list_store_set(model->store, iter_ptr,
                               COL_STATE, current_state,
                               COL_ICON, ui_icon(net->rssi, net->flags.privacy),
                               COL_FREQUENCY, net->frequency,
                               COL_CHANNEL, (net->channel ? net->channel : ""),
                               COL_MODE, (net->mode ? net->mode : ""),
                               COL_SSID, (net->ssid ? net->ssid : ""),
                               COL_RADIONAME, (net->radioname ? net->radioname : ""),
                               COL_RSSI, net->rssi,
                               COL_NOISE, net->noise,
                               COL_PRIVACY, net->flags.privacy,
                               COL_ROUTEROS, net->flags.routeros,
                               COL_NSTREME, net->flags.nstreme,
                               COL_TDMA, net->flags.tdma,
                               COL_WDS, net->flags.wds,
                               COL_BRIDGE, net->flags.bridge,
                               COL_ROUTEROS_VER, (net->routeros_ver ? net->routeros_ver : ""),
                               COL_LASTSEEN, net->firstseen,
                               -1);
        }

        /* Add address to the active network list */
        g_hash_table_insert(model->active, address, iter_ptr);
        new_network_found = FALSE;
    }
    else
    {
        /* Add a new network */
#if MIKROTIK_LOW_SIGNAL_BUGFIX
        if(net->rssi >= -12 && net->rssi <= -10)
            net->rssi = -100;
#endif
#if MIKROTIK_HIGH_SIGNAL_BUGFIX
        if(net->rssi >= 10)
            net->rssi = -100;
#endif

        net->signals = signals_new();
        if(conf_get_interface_signals())
            signals_append(net->signals, signals_node_new(net->firstseen, net->rssi, net->latitude, net->longitude));

        gtk_list_store_insert_with_values(model->store, &iter, -1,
                                          COL_STATE, MODEL_STATE_NEW,
                                          COL_ICON, ui_icon(net->rssi, net->flags.privacy),
                                          COL_BG, (!conf_get_interface_dark_mode() ? &new_network_color : &new_network_color_dark),
                                          COL_ADDRESS, net->address,
                                          COL_FREQUENCY, net->frequency,
                                          COL_CHANNEL, (net->channel ? net->channel : ""),
                                          COL_MODE, (net->mode ? net->mode : ""),
                                          COL_SSID, (net->ssid ? net->ssid : ""),
                                          COL_RADIONAME, (net->radioname ? net->radioname : ""),
                                          COL_MAXRSSI, net->rssi,
                                          COL_RSSI, net->rssi,
                                          COL_NOISE, net->noise,
                                          COL_PRIVACY, net->flags.privacy,
                                          COL_ROUTEROS, net->flags.routeros,
                                          COL_NSTREME, net->flags.nstreme,
                                          COL_TDMA, net->flags.tdma,
                                          COL_WDS, net->flags.wds,
                                          COL_BRIDGE, net->flags.bridge,
                                          COL_ROUTEROS_VER, (net->routeros_ver ? net->routeros_ver : ""),
                                          COL_FIRSTSEEN, net->firstseen,
                                          COL_LASTSEEN, net->firstseen,
                                          COL_LATITUDE, net->latitude,
                                          COL_LONGITUDE, net->longitude,
                                          COL_SIGNALS, net->signals,
                                          -1);

        iter_ptr = gtk_tree_iter_copy(&iter);
        g_hash_table_insert(model->map, net->address, iter_ptr);
        g_hash_table_insert(model->active, net->address, iter_ptr);
        new_network_found = TRUE;

        /* Address is used now in the hash table */
        net->address = NULL;
    }

    /* Signals are stored in GtkListStore just as pointer,
       so set it to NULL before freeing the struct */
    net->signals = NULL;
    return new_network_found;
}

void
mtscan_model_add(mtscan_model_t *model,
                 network_t      *net,
                 gboolean        merge)
{
    GtkTreeIter *iter_merge;
    GtkTreeIter iter;
    gint current_maxrssi;
    gint64 current_firstseen;
    gint64 current_lastseen;
    signals_t *current_signals;

    if(merge && (iter_merge = g_hash_table_lookup(model->map, net->address)))
    {
        /* Merge a network, check current values first */
        gtk_tree_model_get(GTK_TREE_MODEL(model->store), iter_merge,
                           COL_MAXRSSI, &current_maxrssi,
                           COL_FIRSTSEEN, &current_firstseen,
                           COL_LASTSEEN, &current_lastseen,
                           COL_SIGNALS, &current_signals,
                           -1);

        /* Merge signal samples */
        if(signals_merge_and_free(&current_signals, net->signals))
            gtk_list_store_set(model->store, iter_merge, COL_SIGNALS, current_signals, -1);

        /* Update the first seen date, if required */
        if(net->firstseen < current_firstseen)
        {
            gtk_list_store_set(model->store, iter_merge,
                               COL_FIRSTSEEN, net->firstseen,
                               -1);
        }

        /* Update the last seen date along with other values, if required */
        if(net->lastseen > current_lastseen)
        {
            gtk_list_store_set(model->store, iter_merge,
                               COL_FREQUENCY, net->frequency,
                               COL_ICON, ui_icon(MODEL_NO_SIGNAL, net->flags.privacy),
                               COL_CHANNEL, (net->channel ? net->channel : ""),
                               COL_MODE, (net->mode ? net->mode : ""),
                               COL_SSID, (net->ssid ? net->ssid : ""),
                               COL_RADIONAME, (net->radioname ? net->radioname : ""),
                               COL_PRIVACY, net->flags.privacy,
                               COL_ROUTEROS, net->flags.routeros,
                               COL_NSTREME, net->flags.nstreme,
                               COL_TDMA, net->flags.tdma,
                               COL_WDS, net->flags.wds,
                               COL_BRIDGE, net->flags.bridge,
                               COL_ROUTEROS_VER, (net->routeros_ver ? net->routeros_ver : ""),
                               COL_LASTSEEN, net->lastseen,
                               -1);
        }

        /* Update the max signal level together with its coordinates, if required */
        if(net->rssi > current_maxrssi)
        {
            gtk_list_store_set(model->store, iter_merge,
                               COL_MAXRSSI, net->rssi,
                               COL_LATITUDE, net->latitude,
                               COL_LONGITUDE, net->longitude,
                               -1);
        }
    }
    else
    {
        /* Add a new network */
        gtk_list_store_insert_with_values(model->store, &iter, -1,
                                          COL_STATE, MODEL_STATE_INACTIVE,
                                          COL_ICON, ui_icon(MODEL_NO_SIGNAL, net->flags.privacy),
                                          COL_ADDRESS, net->address,
                                          COL_FREQUENCY, net->frequency,
                                          COL_CHANNEL, (net->channel ? net->channel : ""),
                                          COL_MODE, (net->mode ? net->mode : ""),
                                          COL_SSID, (net->ssid ? net->ssid : ""),
                                          COL_RADIONAME, (net->radioname ? net->radioname : ""),
                                          COL_MAXRSSI, net->rssi,
                                          COL_RSSI, MODEL_NO_SIGNAL,
                                          COL_NOISE, net->noise,
                                          COL_PRIVACY, net->flags.privacy,
                                          COL_ROUTEROS, net->flags.routeros,
                                          COL_NSTREME, net->flags.nstreme,
                                          COL_TDMA, net->flags.tdma,
                                          COL_WDS, net->flags.wds,
                                          COL_BRIDGE, net->flags.bridge,
                                          COL_ROUTEROS_VER, (net->routeros_ver ? net->routeros_ver : ""),
                                          COL_FIRSTSEEN, net->firstseen,
                                          COL_LASTSEEN, net->lastseen,
                                          COL_LATITUDE, net->latitude,
                                          COL_LONGITUDE, net->longitude,
                                          COL_SIGNALS, net->signals,
                                          -1);

        g_hash_table_insert(model->map, net->address, gtk_tree_iter_copy(&iter));

        /* Address is now used in the hash table */
        net->address = NULL;
    }

    /* Signals are stored in GtkListStore just as pointer,
       so set it to NULL before freeing the struct */
    net->signals = NULL;
}

void
mtscan_model_set_active_timeout(mtscan_model_t *model,
                                gint            timeout)
{
    model->active_timeout = timeout;
}

void
mtscan_model_set_new_timeout(mtscan_model_t *model,
                             gint            timeout)
{
    model->new_timeout = timeout;
}

void
mtscan_model_disable_sorting(mtscan_model_t *model)
{
    if(gtk_tree_sortable_get_sort_column_id(GTK_TREE_SORTABLE(model->store), &model->last_sort_column, &model->last_sort_order))
        gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(model->store), GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID, model->last_sort_order);
    model->disabled_sorting = TRUE;
}

void
mtscan_model_enable_sorting(mtscan_model_t *model)
{
    if(model->disabled_sorting)
    {
        gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(model->store), model->last_sort_column, model->last_sort_order);
        model->disabled_sorting = FALSE;
    }
}

const gchar*
model_format_address(gchar *address)
{
    static gchar addr_separated[18];
    static gchar addr_unknown[] = "";

    if(address && strlen(address) == 12)
    {
        snprintf(addr_separated, sizeof(addr_separated),
                 "%c%c:%c%c:%c%c:%c%c:%c%c:%c%c",
                 address[0], address[1], address[2], address[3], address[4], address[5],
                 address[6], address[7], address[8], address[9], address[10], address[11]);
        return addr_separated;
    }
    return addr_unknown;
}

const gchar*
model_format_frequency(gint value)
{
    static gchar output[9];
    gint frac, i;

    if((frac = value % 1000))
    {
        g_snprintf(output, sizeof(output), "%d.%03d", value/1000, frac);
        for(i=strlen(output)-1; i>=0 && output[i] == '0'; i--);
        output[i+1] = '\0';
        return output;
    }

    g_snprintf(output, sizeof(output), "%d", value/1000);
    return output;
}

const gchar*
model_format_date(gint64 value)
{
    static gchar output[20];
    gint day_now, month_now, year_now;
#ifdef G_OS_WIN32
    __time64_t ts_val = (__time64_t)value;
    __time64_t ts_now = _time64(NULL);
    struct tm *tm = _localtime64(&ts_now);
#else
    time_t ts_val = (time_t)value;
    time_t ts_now = time(NULL);
    struct tm *tm = localtime(&ts_now);
#endif

    day_now = tm->tm_mday;
    month_now = tm->tm_mon;
    year_now = tm->tm_year;

#ifdef G_OS_WIN32
    tm = _localtime64(&ts_val);
#else
    tm = localtime(&ts_val);
#endif

    if(tm->tm_mday == day_now &&
       tm->tm_mon == month_now &&
       tm->tm_year == year_now)
    {
        strftime(output, sizeof(output), "%H:%M:%S", tm);
    }
    else
    {
        strftime(output, sizeof(output), "%Y-%m-%d %H:%M:%S", tm);
    }

    return output;
}

const gchar*
model_format_gps(gdouble value)
{
    static gchar output[12];
    gint i;

    g_ascii_formatd(output, sizeof(output), "%.6f", value);
    for(i=strlen(output)-1; i>=0 && output[i] == '0'; i--);
    if(i >= 0 && output[i] == '.')
        i++;
    output[i+1] = '\0';
    return output;
}
