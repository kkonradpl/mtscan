#include <glib.h>
#include <string.h>
#include "tzsp/tzsp-socket.h"
#include "tzsp/mac80211.h"
#include "network.h"
#include "tzsp-receiver.h"
#include "tzsp/cambium.h"

typedef struct tzsp_receiver
{
    tzsp_socket_t *tzsp_socket;
    gint channel_width;
    gint frequency_base;
    void (*cb_final)(tzsp_receiver_t*);
    void (*cb_network)(const tzsp_receiver_t*, network_t*);
    uint8_t hw_addr[6];
} tzsp_receiver_t;

typedef struct tzsp_receiver_net
{
    tzsp_receiver_t *context;
    network_t *network;
} tzsp_receiver_net_t;

static gpointer tzsp_receiver_thread(gpointer);
static void tzsp_receiver_packet(const uint8_t*, uint32_t, const int8_t*, const uint8_t*, const uint8_t*, gpointer);
static gboolean tzsp_receiver_callback_network(gpointer);
static gboolean tzsp_receiver_callback_final(gpointer);

tzsp_receiver_t*
tzsp_receiver_new(guint16       udp_port,
                  guint8        hw_addr[6],
                  gint          channel_width,
                  gint          frequency_base,
                  void        (*cb_final) (tzsp_receiver_t*),
                  void        (*cb_network)(const tzsp_receiver_t*, network_t*))
{
    tzsp_socket_t *socket = NULL;
    tzsp_receiver_t *context;

    socket = tzsp_socket_new();
    if(socket == NULL ||
       tzsp_socket_init(socket, udp_port, NULL, NULL) != TZSP_SOCKET_OK)
    {
        tzsp_socket_free(socket);
        return NULL;
    }

    context = g_malloc0(sizeof(tzsp_receiver_t));
    context->tzsp_socket = socket;
    tzsp_socket_set_func(socket, tzsp_receiver_packet, context);

    memcpy(context->hw_addr, hw_addr, 6);
    context->channel_width = channel_width;
    context->frequency_base = frequency_base;
    context->cb_final = cb_final;
    context->cb_network = cb_network;

    g_thread_unref(g_thread_new("tzsp_receiver_thread", tzsp_receiver_thread, context));
    return context;
}

void
tzsp_receiver_free(tzsp_receiver_t *context)
{
    g_free(context);
}

static gpointer
tzsp_receiver_thread(gpointer user_data)
{
    tzsp_receiver_t *context = (tzsp_receiver_t*)user_data;
    tzsp_socket_loop(context->tzsp_socket);
    g_idle_add(tzsp_receiver_callback_final, context);
    return NULL;
}

static void
tzsp_receiver_packet(const uint8_t *packet,
                     uint32_t       len,
                     const int8_t  *rssi,
                     const uint8_t *tzsp_channel,
                     const uint8_t *sensor_mac,
                     gpointer       user_data)
{
    /* Function called from the TZSP thread */
    tzsp_receiver_t *context = (tzsp_receiver_t*)user_data;
    mac80211_net_t *net_80211 = NULL;
    nv2_net_t *net_nv2 = NULL;
    cambium_net_t *net_cambium = NULL;
    const uint8_t *src;
    tzsp_receiver_net_t *data;
    gint channel = -1;

    /* Ignore pre-6.41 TZSP packets with no sensor address included */
    if(sensor_mac == NULL)
        return;

    /* Make sure that sensor address matches the desired one */
    if(memcmp(sensor_mac, context->hw_addr, 6) != 0)
        return;

    /* Try nv2 parser */
    net_nv2 = nv2_network(packet, len, &src);
    if(!net_nv2)
    {
        /* Try mac80211 parser */
        net_80211 = mac80211_network(packet, len, &src);
        if(!net_80211)
        {
            /* This is not a IEEE 802.11 frame */

            /* Try cambium parser */
            net_cambium = cambium_network(packet, len, &src);
            if(!net_cambium)
                return;
        }
        else if(net_80211->source != MAC80211_FRAME_BEACON &&
                net_80211->source != MAC80211_FRAME_PROBE_RESPONSE)
        {
            /* This is other IEEE 802.11 frame */
            mac80211_net_free(net_80211);
            return;
        }
    }

    data = g_malloc(sizeof(tzsp_receiver_net_t));
    data->context = context;
    data->network = g_malloc(sizeof(network_t));
    network_init(data->network);

    /* Fill the BSSID address */
    data->network->address  = (gint64) src[0] << 40;
    data->network->address |= (gint64) src[1] << 32;
    data->network->address |= (gint64) src[2] << 24;
    data->network->address |= (gint64) src[3] << 16;
    data->network->address |= (gint64) src[4] << 8;
    data->network->address |= (gint64) src[5] << 0;

    /* Fill the signal level value */
    if(rssi)
        data->network->rssi = *rssi;

    /* Default values */
    data->network->flags.routeros = 0;
    data->network->ubnt_airmax = 0;
    data->network->wps = 0;

    if(net_80211)
    {
        if(net_80211->ie_mikrotik)
        {
            if(!data->network->radioname)
                data->network->radioname = g_strdup(ie_mikrotik_get_radioname(net_80211->ie_mikrotik));

            if(!data->network->routeros_ver)
                data->network->routeros_ver = g_strdup(ie_mikrotik_get_version(net_80211->ie_mikrotik));

            data->network->frequency = ie_mikrotik_get_frequency(net_80211->ie_mikrotik) * 1000;
            data->network->flags.routeros = 1;
            data->network->flags.nstreme = ie_mikrotik_is_nstreme(net_80211->ie_mikrotik);
            data->network->flags.tdma = FALSE;
            data->network->flags.wds = ie_mikrotik_is_wds(net_80211->ie_mikrotik);
            data->network->flags.bridge = ie_mikrotik_is_bridge(net_80211->ie_mikrotik);
        }

        if(net_80211->ie_airmax)
            data->network->ubnt_airmax = 1;

        if(net_80211->ie_airmax_ac)
        {
            data->network->ubnt_airmax = 1;

            if(!data->network->ssid)
                data->network->ssid = g_strdup(ie_airmax_ac_get_ssid(net_80211->ie_airmax_ac));

            if(!data->network->radioname)
                data->network->radioname = g_strdup(ie_airmax_ac_get_radioname(net_80211->ie_airmax_ac));

            data->network->ubnt_ptp = ie_airmax_ac_is_ptp(net_80211->ie_airmax_ac);
            data->network->ubnt_ptmp = ie_airmax_ac_is_ptmp(net_80211->ie_airmax_ac);
            data->network->ubnt_mixed = ie_airmax_ac_is_mixed(net_80211->ie_airmax_ac);
        }

        if(net_80211->ie_wps)
        {
            data->network->wps = 1;
            if(net_80211->source == MAC80211_FRAME_PROBE_RESPONSE)
            {
                data->network->wps = 2;
                if(!data->network->wps_manufacturer)
                    data->network->wps_manufacturer = g_strdup(ie_wps_get_manufacturer(net_80211->ie_wps));
                if(!data->network->wps_model_name)
                    data->network->wps_model_name = g_strdup(ie_wps_get_model_name(net_80211->ie_wps));
                if(!data->network->wps_model_number)
                    data->network->wps_model_number = g_strdup(ie_wps_get_model_number(net_80211->ie_wps));
                if(!data->network->wps_serial_number)
                    data->network->wps_serial_number = g_strdup(ie_wps_get_serial_number(net_80211->ie_wps));
                if(!data->network->wps_device_name)
                    data->network->wps_device_name = g_strdup(ie_wps_get_device_name(net_80211->ie_wps));
            }
        }

        /* Network frequency based on TZSP channel on 5 GHz band */
        if(!data->network->frequency &&
           context->frequency_base == 5000 &&
           tzsp_channel)
        {
            /* HACK! Workaround for 4.9 GHz */
            if(net_80211->channel >= 160 && /* 4800 */
               net_80211->channel <= 199 && /* 4995 */
               *tzsp_channel >= 11 && /* 4800 */
               *tzsp_channel <= 50 && /* 4995 */
               (net_80211->channel - (int)*tzsp_channel) == (184 - 35))
            {
                /* Valid for Ubiquiti Airmax & Airmax AC (4920 - 4995 MHz) */
                /* Not tested below 4920 MHz */
                data->network->frequency = (4920 + ((net_80211->channel - 184) * 5)) * 1000;
            }
            else /* ≥ 5000 MHz */
            {
                data->network->frequency = (context->frequency_base + *tzsp_channel * 5) * 1000;
            }
        }

        if(!data->network->frequency)
        {
            if(net_80211->channel >= 0)
            {
                /* Use channel from beacon tag for other bands (i.e. 2.4 GHz) due to DSSS channel overlap */
                channel = net_80211->channel;
            }
            else if(tzsp_channel)
            {
                /* Fallback to TZSP channel, if beacon does not contain channel number */
                channel = *tzsp_channel;
            }

            if(channel >= 0)
            {
                if(context->frequency_base == 2407 && channel >= 128)
                {
                    /* Sub 2.4 GHz (negative unsigned 8-bit value, i.e. ≥ 128) */
                    data->network->frequency = (context->frequency_base - (256 - channel) * 5) * 1000;
                }
                else if(context->frequency_base == 2407 && channel == 14)
                {
                    /* Special case for channel 14 */
                    data->network->frequency = 2484 * 1000;
                }
                else
                {
                    /* Regular channel */
                    data->network->frequency = (context->frequency_base + channel * 5) * 1000;
                }
            }
        }

        if(!data->network->ssid)
            data->network->ssid = g_strdup(net_80211->ssid);

        if(!data->network->radioname)
            data->network->radioname = g_strdup(net_80211->radioname);

        data->network->streams = mac80211_net_get_chains(net_80211);
        data->network->flags.privacy = mac80211_net_is_privacy(net_80211);

        if(!data->network->channel)
        {
            if(mac80211_net_get_ext_channel(net_80211))
                data->network->channel = g_strdup_printf("%d-%s", context->channel_width, mac80211_net_get_ext_channel(net_80211));
            else
                data->network->channel = g_strdup_printf("%d", context->channel_width);
        }

        if(mac80211_net_is_he(net_80211))
            data->network->mode = g_strdup("ax");
        else if(mac80211_net_is_vht(net_80211))
            data->network->mode = g_strdup("ac");
        else if(mac80211_net_is_ht(net_80211))
        {
            if(data->network->frequency &&
               data->network->frequency < 3000000)
                data->network->mode = g_strdup("gn");
            else
                data->network->mode = g_strdup("an");
        }
        else if(mac80211_net_is_ofdm(net_80211))
        {
            if(data->network->frequency &&
               data->network->frequency < 3000000)
                data->network->mode = g_strdup("g");
            else
                data->network->mode = g_strdup("a");
        }
        else if(mac80211_net_is_dsss(net_80211))
        {
            data->network->mode = g_strdup("b");
        }
        nv2_net_free(net_nv2);
        mac80211_net_free(net_80211);
    }

    if(net_nv2)
    {
        data->network->ssid = g_strdup(nv2_net_get_ssid(net_nv2));
        data->network->radioname = g_strdup(nv2_net_get_radioname(net_nv2));
        data->network->routeros_ver = g_strdup(nv2_net_get_version(net_nv2));

        if(nv2_net_get_frequency(net_nv2))
            data->network->frequency = nv2_net_get_frequency(net_nv2) * 1000;

        data->network->flags.privacy = nv2_net_is_privacy(net_nv2);
        data->network->flags.routeros = 1;
        data->network->flags.nstreme = 0;
        data->network->flags.tdma = 1;
        data->network->flags.wds = nv2_net_is_wds(net_nv2);
        data->network->flags.bridge = nv2_net_is_bridge(net_nv2);

        if(nv2_net_get_ext_channel(net_nv2))
            data->network->channel = g_strdup_printf("%d-%s", context->channel_width, nv2_net_get_ext_channel(net_nv2));
        else
            data->network->channel = g_strdup_printf("%d", context->channel_width);

        data->network->streams = nv2_net_get_chains(net_nv2);

        if(nv2_net_is_vht(net_nv2))
            data->network->mode = g_strdup("ac");
        else if(nv2_net_is_ht(net_nv2))
        {
            if(nv2_net_get_frequency(net_nv2) < 3000)
                data->network->mode = g_strdup("gn");
            else
                data->network->mode = g_strdup("an");
        }
        else if(nv2_net_get_frequency(net_nv2) < 3000)
        {
            if(nv2_net_is_ofdm(net_nv2))
                data->network->mode = g_strdup("g");
            else
                data->network->mode = g_strdup("b");
        }
        else
        {
            data->network->mode = g_strdup("a");
        }
        nv2_net_free(net_nv2);
    }

    if(net_cambium)
    {
        data->network->ssid = g_strdup(cambium_net_get_ssid(net_cambium));

        if(cambium_net_get_frequency(net_cambium))
            data->network->frequency = cambium_net_get_frequency(net_cambium) * 1000;
        else if(tzsp_channel)
            data->network->frequency = (*tzsp_channel * 5 + context->frequency_base) * 1000;

        cambium_net_free(net_cambium);
    }

    data->network->firstseen = g_get_real_time() / 1000000;
    data->network->lastseen = data->network->firstseen;

    /* Network must be added from main thread */
    g_idle_add(tzsp_receiver_callback_network, data);
}

static gboolean
tzsp_receiver_callback_network(gpointer user_data)
{
    tzsp_receiver_net_t *data = (tzsp_receiver_net_t*)user_data;
    data->context->cb_network(data->context, data->network);
    g_free(data);
    return G_SOURCE_REMOVE;
}

static gboolean
tzsp_receiver_callback_final(gpointer user_data)
{
    tzsp_receiver_t *context = (tzsp_receiver_t*)user_data;

    tzsp_socket_free(context->tzsp_socket);
    context->tzsp_socket = NULL;

    context->cb_final(context);
    return G_SOURCE_REMOVE;
}

void
tzsp_receiver_enable(tzsp_receiver_t *context)
{
    tzsp_socket_enable(context->tzsp_socket);
}

void
tzsp_receiver_disable(tzsp_receiver_t *context)
{
    tzsp_socket_disable(context->tzsp_socket);
}

void
tzsp_receiver_cancel(tzsp_receiver_t *context)
{
    tzsp_socket_cancel(context->tzsp_socket);
}
