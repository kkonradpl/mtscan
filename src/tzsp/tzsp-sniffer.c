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

#define _GNU_SOURCE
#include <pcap.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#ifdef _WIN32
#include <Winsock2.h>
#include <Ws2tcpip.h>
#define inet_pton InetPtonW
#else
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#endif
#include "tzsp-sniffer.h"
#include "tzsp-decap.h"

typedef struct tzsp_sniffer
{
    pcap_t         *capture;
    pcap_t         *dump;
    pcap_dumper_t  *dumper;
    volatile bool   canceled;
    volatile bool   enabled;
    char            errbuf[PCAP_ERRBUF_SIZE];
    void          (*user_func)(const uint8_t*, uint32_t, const int8_t*, const uint8_t*, const uint8_t*, void*);
    void           *user_data;
} tzsp_sniffer_t;


tzsp_sniffer_t*
tzsp_sniffer_new()
{
    return calloc(sizeof(tzsp_sniffer_t), 1);
}

int
tzsp_sniffer_init(tzsp_sniffer_t *context,
                  uint16_t        udp_port,
                  const char     *output,
                  const char     *ip_src,
                  const char     *dev_if,
                  int             latency)
{
    char *filter_exp;
    struct bpf_program fp;
    bpf_u_int32 net;
    bpf_u_int32 mask;
    int ret;

    /* Clear the string error buffer */
    *context->errbuf = '\0';

    if(ip_src &&
       inet_addr(ip_src) == INADDR_NONE)
    {
        snprintf(context->errbuf, PCAP_ERRBUF_SIZE, "Invalid IP address");
        return TZSP_SNIFFER_ERROR_INVALID_IP;
    }

    /* Prepare filter expression */
    if(asprintf(&filter_exp,
                "udp dst port %d%s%s",
                udp_port,
                (ip_src ? " and src host " : ""), (ip_src ? ip_src : "")) < 0)
    {
        snprintf(context->errbuf, PCAP_ERRBUF_SIZE, "Failed to allocate memory");
        return TZSP_SNIFFER_ERROR_MEMORY;
    }

    if((context->capture = pcap_open_live(dev_if, BUFSIZ, 1, latency, context->errbuf)) == NULL)
    {
        ret = TZSP_SNIFFER_ERROR_CAPTURE;
        goto free_filter_exp;
    }

    if(pcap_datalink(context->capture) != DLT_EN10MB)
    {
        snprintf(context->errbuf, PCAP_ERRBUF_SIZE, "Selected interface has wrong datalink");
        ret = TZSP_SNIFFER_ERROR_DATALINK;
        goto free_capture;
    }

    if(pcap_lookupnet(dev_if, &net, &mask, context->errbuf) == -1)
    {
        net = 0;
        mask = 0;
    }

    if(pcap_compile(context->capture, &fp, filter_exp, 0, net) == -1)
    {
        snprintf(context->errbuf, PCAP_ERRBUF_SIZE, "pcap_compile failed");
        ret = TZSP_SNIFFER_ERROR_FILTER;
        goto free_capture;
    }

    if(pcap_setfilter(context->capture, &fp) == -1)
    {
        snprintf(context->errbuf, PCAP_ERRBUF_SIZE, "pcap_setfilter failed");
        ret = TZSP_SNIFFER_ERROR_SET_FILTER;
        goto free_filter;
    }

    if(output)
    {
        if ((context->dump = pcap_open_dead(DLT_IEEE802_11, BUFSIZ)) == NULL)
        {
            snprintf(context->errbuf, PCAP_ERRBUF_SIZE, "pcap_open_dead failed");
            ret = TZSP_SNIFFER_ERROR_DUMP_OPEN_DEAD;
            goto free_filter;
        }

        if ((context->dumper = pcap_dump_open(context->dump, output)) == NULL)
        {
            snprintf(context->errbuf, PCAP_ERRBUF_SIZE, "pcap_open_dead failed");
            ret = TZSP_SNIFFER_ERROR_DUMP_OPEN;
            goto free_dump;
        }
    }

    pcap_freecode(&fp);
    free(filter_exp);
    return TZSP_SNIFFER_OK;

free_dump:
    pcap_close(context->dump);
    context->dump = NULL;
free_filter:
    pcap_freecode(&fp);
free_capture:
    pcap_close(context->capture);
    context->capture = NULL;
free_filter_exp:
    free(filter_exp);
    return ret;
}

const char*
tzsp_sniffer_get_error(const tzsp_sniffer_t *context)
{
    return context->errbuf;
}

void
tzsp_sniffer_set_func(tzsp_sniffer_t  *context,
                      void           (*user_func)(const uint8_t*, uint32_t, const int8_t*, const uint8_t*, const uint8_t*, void*),
                      void           (*user_data))
{
    context->user_func = user_func;
    context->user_data = user_data;
}

void
tzsp_sniffer_loop(tzsp_sniffer_t *context)
{
    struct pcap_pkthdr *header;
    const u_char *packet;
    const u_char *ptr;
    const int8_t *rssi;
    const uint8_t *channel;
    const uint8_t *sensor_mac;
    int ret;

    printf("tzsp_sniffer_loop\n");

    while(!context->canceled &&
          (ret = pcap_next_ex(context->capture, &header, &packet)) >= 0)
    {
        if(ret != 0 && !context->canceled && context->enabled)
        {
            if((ptr = decap_ethernet(packet, &header->caplen)) == NULL)
                continue;
            if((ptr = decap_ip(ptr, &header->caplen)) == NULL)
                continue;
            if((ptr = decap_udp(ptr, &header->caplen)) == NULL)
                continue;

            rssi = NULL;
            if((ptr = decap_tzsp(ptr, &header->caplen, &rssi, &channel, &sensor_mac)) == NULL)
                continue;

            if(context->dumper)
            {
                header->len = header->caplen;
                pcap_dump((u_char*)context->dumper, header, ptr);
            }

            if(context->user_func)
                context->user_func(ptr, header->caplen, rssi, channel, sensor_mac, context->user_data);
        }
    }

    printf("tzsp_sniffer_loop END\n");
}

void
tzsp_sniffer_enable(tzsp_sniffer_t *context)
{
    context->enabled = true;
}

void
tzsp_sniffer_disable(tzsp_sniffer_t *context)
{
    context->enabled = false;
}

void
tzsp_sniffer_cancel(tzsp_sniffer_t *context)
{
    context->canceled = true;
    pcap_breakloop(context->capture);
}

void
tzsp_sniffer_free(tzsp_sniffer_t *context)
{
    if(context)
    {
        if(context->dumper)
            pcap_dump_close(context->dumper);
        if(context->dump)
            pcap_close(context->dump);
        if(context->capture)
            pcap_close(context->capture);
        free(context);
    }
}

