/*
 *  MTscan - MikroTik RouterOS wireless scanner
 *  Copyright (c) 2015-2018  Konrad Kosmatka
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

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>
#include <signal.h>
#include "tzsp-sniffer.h"
#include "tzsp-socket.h"
#include "mac80211.h"

#define TZSP_UDP_PORT 0x9090

static tzsp_sniffer_t *tzsp_sniffer = NULL;
static tzsp_socket_t *tzsp_socket = NULL;

static void
signal_handler(int signo)
{
    switch(signo)
    {
        case SIGINT:
        case SIGTERM:
        case SIGHUP:
            if(tzsp_sniffer)
                tzsp_sniffer_cancel(tzsp_sniffer);
            if(tzsp_socket)
                tzsp_socket_cancel(tzsp_socket);
            break;

        case SIGUSR1:
            if(tzsp_sniffer)
                tzsp_sniffer_enable(tzsp_sniffer);
            if(tzsp_socket)
                tzsp_socket_enable(tzsp_socket);
            break;

        case SIGUSR2:
            if(tzsp_sniffer)
                tzsp_sniffer_disable(tzsp_sniffer);
            if(tzsp_socket)
                tzsp_socket_disable(tzsp_socket);
            break;

        default:
            break;
    }
}

static void
show_usage(FILE *fp,
           char *arg)
{
    fprintf(fp, "network socket usage: %s [ -o <filename> ] [ -s <src-ip> ]\n", arg);
    fprintf(fp, "pcap capture usage:   %s -p -i <interface> [ -o <filename> ]  [ -s <src-ip> ] \n", arg);
}

static void
process(const uint8_t *packet,
        uint32_t       len,
        const int8_t  *rssi,
        const uint8_t *channel,
        const uint8_t *sensor_mac,
        void          *user_data)
{
    mac80211_net_t *net = NULL;
    nv2_net_t *net_nv2 = NULL;
    const uint8_t *src;


    net_nv2 = nv2_network(packet, len, &src);
    if(!net_nv2)
    {
        net = mac80211_network(packet, len, &src);
        if(!net)
            return;

        if(net->source != MAC80211_FRAME_BEACON &&
           net->source != MAC80211_FRAME_PROBE_RESPONSE)
        {
            mac80211_net_free(net);
            return;
        }
    }

    if(src)
        printf("%02X:%02X:%02X:%02X:%02X:%02X ",
               src[0], src[1], src[2], src[3], src[4], src[5]);

    if(rssi)
        printf("%d dBm ", *rssi);

    if(channel)
        printf("CH=%d ", *channel);

    if(sensor_mac)
        printf("[SENSOR=%02X:%02X:%02X:%02X:%02X:%02X] ",
               sensor_mac[0], sensor_mac[1], sensor_mac[2], sensor_mac[3], sensor_mac[4], sensor_mac[5]);

    if(net)
    {
        if(net->source == MAC80211_FRAME_BEACON)
            printf("SRC=BEACON ");

        if(net->source == MAC80211_FRAME_PROBE_RESPONSE)
            printf("SRC=PROBE_RESPONSE ");

        if(net->ssid)
            printf("SSID=%s ", net->ssid);

        if(net->radioname)
            printf("RADIONAME=%s ", net->radioname);

        if(mac80211_net_is_privacy(net))
            printf("PRIVACY ");

        if(mac80211_net_get_ext_channel(net))
            printf("EXTCHAN=%s ", mac80211_net_get_ext_channel(net));

        if(mac80211_net_get_chains(net))
            printf("CHAINS=%d ", mac80211_net_get_chains(net));

        if(net->ie_mikrotik)
        {
            printf("IE_MIKROTIK ");
            if(ie_mikrotik_get_radioname(net->ie_mikrotik))
                printf("RADIONAME=%s ", ie_mikrotik_get_radioname(net->ie_mikrotik));
            if(ie_mikrotik_get_version(net->ie_mikrotik))
                printf("ROS=%s ", ie_mikrotik_get_version(net->ie_mikrotik));
            if(ie_mikrotik_get_frequency(net->ie_mikrotik))
                printf("FREQ=%d ", ie_mikrotik_get_frequency(net->ie_mikrotik));
            if(ie_mikrotik_is_nstreme(net->ie_mikrotik))
                printf("NSTREME ");
            if(ie_mikrotik_is_wds(net->ie_mikrotik))
                printf("WDS ");
            if(ie_mikrotik_is_bridge(net->ie_mikrotik))
                printf("BRIDGE ");
        }

        if(net->ie_airmax)
        {
            printf("IE_AIRMAX ");
        }

        if(net->ie_airmax_ac)
        {
            printf("IE_AIRMAX_AC ");
            if(ie_airmax_ac_get_ssid(net->ie_airmax_ac))
                printf("SSID=%s ", ie_airmax_ac_get_ssid(net->ie_airmax_ac));
            if(ie_airmax_ac_get_radioname(net->ie_airmax_ac))
                printf("RADIONAME=%s ", ie_airmax_ac_get_radioname(net->ie_airmax_ac));
            if(ie_airmax_ac_is_ptp(net->ie_airmax_ac))
                printf("PTP ");
            if(ie_airmax_ac_is_ptmp(net->ie_airmax_ac))
                printf("PTMP ");
            if(ie_airmax_ac_is_mixed(net->ie_airmax_ac))
                printf("MIXED ");
        }

        if(mac80211_net_is_vht(net))
            printf("80211AC ");
        else if(mac80211_net_is_ht(net))
            printf("80211N ");
        else if(mac80211_net_is_ofdm(net))
        {
            /* no frequency information */
            //printf("80211G");
            printf("80211A");
        }
        else if(mac80211_net_is_dsss(net))
        {
            printf("80211B");
        }
        mac80211_net_free(net);
    }

    if(net_nv2)
    {
        printf("SRC=NV2_BEACON ");
        if(nv2_net_get_ssid(net_nv2))
            printf("SSID=%s ", nv2_net_get_ssid(net_nv2));
        if(nv2_net_get_radioname(net_nv2))
            printf("RADIONAME=%s ", nv2_net_get_radioname(net_nv2));
        if(nv2_net_get_version(net_nv2))
            printf("ROS=%s ", nv2_net_get_version(net_nv2));
        if(nv2_net_get_frequency(net_nv2))
            printf("FREQ=%d ", nv2_net_get_frequency(net_nv2));
        if(nv2_net_get_ext_channel(net_nv2))
            printf("EXTCHAN=%s ", nv2_net_get_ext_channel(net_nv2));
        if(nv2_net_is_wds(net_nv2))
            printf("WDS ");
        if(nv2_net_is_bridge(net_nv2))
            printf("BRDIGE ");
        if(nv2_net_is_sgi(net_nv2))
            printf("SGI ");
        if(nv2_net_is_privacy(net_nv2))
            printf("PRIVACY ");
        if(nv2_net_is_frameprio(net_nv2))
            printf("FRAMEPRIO ");
        if(nv2_net_get_chains(net_nv2))
            printf("CHAINS=%d ", nv2_net_get_chains(net_nv2));
        if(nv2_net_get_queue_count(net_nv2))
            printf("QUEUE=%d ", nv2_net_get_queue_count(net_nv2));

        if(nv2_net_is_vht(net_nv2))
            printf("80211AC ");
        else if(nv2_net_is_ht(net_nv2))
            printf("80211N ");
        else if(nv2_net_is_ofdm(net_nv2))
        {
            if(nv2_net_get_frequency(net_nv2) < 3000)
                printf("80211G");
            else
                printf("80211A");
        }
        else
        {
            printf("80211B");
        }
        nv2_net_free(net_nv2);
    }

    printf("\n");
}

int
main(int   argc,
     char *argv[])
{
    char *dev_if = NULL;
    char *output = NULL;
    char *ip_src = NULL;
    bool use_pcap = false;
    int c;

    while((c = getopt(argc, argv, "hi:o:s:p")) != -1)
    {
        switch(c)
        {
            case 'h':
                show_usage(stdout, argv[0]);
                exit(EXIT_SUCCESS);

            case 'i':
                dev_if = optarg;
                break;

            case 'o':
                output = optarg;
                break;

            case 's':
                ip_src = optarg;
                break;

            case 'p':
                use_pcap = true;
                break;

            case ':':
            case '?':
                show_usage(stderr, argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if(use_pcap && !dev_if)
    {
        fprintf(stderr, "No local device interface specified\n");
        show_usage(stderr, argv[0]);
        exit(EXIT_FAILURE);
    }

    if(!use_pcap && dev_if)
    {
        fprintf(stderr, "Local device interface not supported in socket mode\n");
        show_usage(stderr, argv[0]);
        exit(EXIT_FAILURE);
    }

    if(use_pcap)
    {
        tzsp_sniffer = tzsp_sniffer_new();
        switch(tzsp_sniffer_init(tzsp_sniffer, TZSP_UDP_PORT, output, ip_src, dev_if, 100))
        {
            case TZSP_SNIFFER_OK:
                break;

            case TZSP_SNIFFER_ERROR_MEMORY:
                fprintf(stderr, "Memory allocation failed\n");
                exit(EXIT_FAILURE);

            case TZSP_SNIFFER_ERROR_CAPTURE:
                fprintf(stderr, "Could not open device %s\n", tzsp_sniffer_get_error(tzsp_sniffer));
                exit(EXIT_FAILURE);

            case TZSP_SNIFFER_ERROR_DATALINK:
                fprintf(stderr, "Ethernet headers are not supported by device %s\n", dev_if);
                exit(EXIT_FAILURE);

            case TZSP_SNIFFER_ERROR_FILTER:
                fprintf(stderr, "Could not parse packet filter\n");
                exit(EXIT_FAILURE);

            case TZSP_SNIFFER_ERROR_SET_FILTER:
                fprintf(stderr, "Could not install packet filter\n");
                exit(EXIT_FAILURE);

            case TZSP_SNIFFER_ERROR_DUMP_OPEN_DEAD:
                fprintf(stderr, "Could not open virtual output device\n");
                exit(EXIT_FAILURE);

            case TZSP_SNIFFER_ERROR_DUMP_OPEN:
                fprintf(stderr, "Could not open output file %s\n", output);
                exit(EXIT_FAILURE);

            default:
                fprintf(stderr, "Unknown error\n");
                exit(EXIT_FAILURE);
        }
    }
    else
    {
        tzsp_socket = tzsp_socket_new();
        switch(tzsp_socket_init(tzsp_socket, TZSP_UDP_PORT, output, ip_src))
        {
            case TZSP_SOCKET_OK:
                break;

            case TZSP_SOCKET_ERROR_INVALID_IP:
                fprintf(stderr, "Invalid IP address\n");
                exit(EXIT_FAILURE);

            case TZSP_SOCKET_ERROR_SOCKET:
                fprintf(stderr, "Failed to create a socket\n");
                exit(EXIT_FAILURE);

            case TZSP_SOCKET_ERROR_BIND:
                fprintf(stderr, "Failed to bind a port\n");
                exit(EXIT_FAILURE);

            case TZSP_SOCKET_ERROR_DUMP_OPEN_DEAD:
                fprintf(stderr, "Could not open virtual output device\n");
                exit(EXIT_FAILURE);

            case TZSP_SOCKET_ERROR_DUMP_OPEN:
                fprintf(stderr, "Could not open output file %s\n", output);
                exit(EXIT_FAILURE);

            default:
                fprintf(stderr, "Unknown error\n");
                exit(EXIT_FAILURE);

        }
    }

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGHUP, signal_handler);
    signal(SIGUSR1, signal_handler);
    signal(SIGUSR2, signal_handler);

    if(tzsp_sniffer)
    {
        tzsp_sniffer_set_func(tzsp_sniffer, process, NULL);
        tzsp_sniffer_enable(tzsp_sniffer);
        tzsp_sniffer_loop(tzsp_sniffer);
    }

    if(tzsp_socket)
    {
        tzsp_socket_set_func(tzsp_socket, process, NULL);
        tzsp_socket_enable(tzsp_socket);
        tzsp_socket_loop(tzsp_socket);
    }

    signal(SIGINT, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
    signal(SIGHUP, SIG_DFL);
    signal(SIGUSR1, SIG_DFL);
    signal(SIGUSR2, SIG_DFL);

    if(tzsp_sniffer)
        tzsp_sniffer_free(tzsp_sniffer);

    if(tzsp_socket)
        tzsp_socket_free(tzsp_socket);

    return 0;
}
