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
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#ifdef HAVE_PCAP
#include <pcap.h>
#endif
#include <string.h>
#include <sys/time.h>
#include <errno.h>
#ifdef _WIN32
#include <Winsock2.h>
#include <Ws2tcpip.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#endif
#include "tzsp-decap.h"
#include "tzsp-socket.h"

#ifdef _WIN32
typedef SOCKET socket_t;
#define inet_pton InetPtonW
#else
typedef int socket_t;
#endif

#define SOCKET_BUFF_LEN 65536

typedef struct tzsp_socket
{
    socket_t         socket;
#ifdef HAVE_PCAP
    pcap_t          *dump;
    pcap_dumper_t   *dumper;
#endif
    volatile bool    canceled;
    volatile bool    enabled;
    uint32_t         src;
    void           (*user_func)(const uint8_t*, uint32_t, const int8_t*, const uint8_t*, const uint8_t*, void*);
    void            *user_data;
} tzsp_socket_t;

static void tzsp_socket_close(tzsp_socket_t*);


tzsp_socket_t*
tzsp_socket_new()
{
    return calloc(sizeof(tzsp_socket_t), 1);
}

int
tzsp_socket_init(tzsp_socket_t *context,
                 uint16_t       tzsp_port,
                 const char    *output,
                 const char    *ip_src)
{
    struct sockaddr_in addr;
    int ret;

    if(!ip_src)
        context->src = INADDR_NONE;
    else
    {
        context->src = inet_addr(ip_src);
        if(context->src == INADDR_NONE)
            return TZSP_SOCKET_ERROR_INVALID_IP;
    }

    context->socket = socket(AF_INET, SOCK_DGRAM, 0);
    if(context->socket < 0)
        return TZSP_SOCKET_ERROR_SOCKET;

    memset((char*)&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(tzsp_port);

    if(bind(context->socket, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        ret = TZSP_SOCKET_ERROR_BIND;
        goto free_socket;
    }

    if(output)
    {
#ifdef HAVE_PCAP
        if((context->dump = pcap_open_dead(DLT_IEEE802_11, BUFSIZ)) == NULL)
        {
            ret = TZSP_SOCKET_ERROR_DUMP_OPEN_DEAD;
            goto free_socket;
        }

        if((context->dumper = pcap_dump_open(context->dump, output)) == NULL)
        {
            ret = TZSP_SOCKET_ERROR_DUMP_OPEN;
            goto free_dump;
        }
#endif
    }

    return TZSP_SOCKET_OK;

#ifdef HAVE_PCAP
free_dump:
    pcap_close(context->dump);
    context->dump = NULL;
#endif
free_socket:
    tzsp_socket_close(context);
    return ret;
}

void
tzsp_socket_set_func(tzsp_socket_t  *context,
                     void          (*user_func)(const uint8_t*, uint32_t, const int8_t*, const uint8_t*, const uint8_t*, void*),
                     void          (*user_data))
{
    context->user_func = user_func;
    context->user_data = user_data;
}

void
tzsp_socket_loop(tzsp_socket_t *context)
{
    uint8_t packet[SOCKET_BUFF_LEN];
    const uint8_t *ptr;
    struct sockaddr_in addr;
    socklen_t len;
    struct timeval timeout;
#ifdef HAVE_PCAP
    struct pcap_pkthdr header;
#endif
    fd_set input;
    const int8_t *rssi;
    const uint8_t *channel;
    const uint8_t *sensor_mac;
    ssize_t ret;
    uint32_t data_len;

    while(!context->canceled)
    {
        FD_ZERO(&input);
        FD_SET(context->socket, &input);
        timeout.tv_sec  = 0;
        timeout.tv_usec = 100 * 100;

        ret = select(context->socket+1, &input, NULL, NULL, &timeout);
        if(ret < 0)
        {
            if(ret == -1 && errno == EINTR)
                continue;
            break;
        }
        if(ret == 0)
            continue;

        len = sizeof(addr);
        ret = recvfrom(context->socket, (char*)packet, SOCKET_BUFF_LEN, 0, (struct sockaddr*)&addr, &len);
        if(ret <= 0)
        {
            if(ret == -1 && errno == EINTR)
                continue;
            break;
        }

        if(!context->canceled &&
            context->enabled)
        {
            if(context->src != INADDR_NONE && context->src != addr.sin_addr.s_addr)
                continue;

#ifdef HAVE_PCAP
            gettimeofday(&header.ts, NULL);
            header.caplen = (uint32_t)ret;
#endif

            rssi = NULL;
            data_len = (uint32_t)ret;
            if((ptr = decap_tzsp(packet, &data_len, &rssi, &channel, &sensor_mac)) == NULL)
                continue;

#ifdef HAVE_PCAP
            if(context->dumper)
            {
                header.len = header.caplen;
                pcap_dump((u_char*)context->dumper, &header, ptr);
            }
#endif

            if(context->user_func)
                context->user_func(ptr, data_len, rssi, channel, sensor_mac, context->user_data);
        }
    }
}

void
tzsp_socket_enable(tzsp_socket_t *context)
{
    context->enabled = true;
}

void
tzsp_socket_disable(tzsp_socket_t *context)
{
    context->enabled = false;
}

void
tzsp_socket_cancel(tzsp_socket_t *context)
{
    context->canceled = true;
}

void
tzsp_socket_free(tzsp_socket_t *context)
{
    if(context)
    {
        tzsp_socket_close(context);
#ifdef HAVE_PCAP
        if(context->dumper)
            pcap_dump_close(context->dumper);
        if(context->dump)
            pcap_close(context->dump);
#endif
        free(context);
    }
}

static void
tzsp_socket_close(tzsp_socket_t *context)
{
#ifdef _WIN32
    if(context->socket == INVALID_SOCKET)
        return;
    shutdown(context->socket, SD_BOTH);
    closesocket(context->socket);
    context->socket = INVALID_SOCKET;
#else
    if(context->socket < 0)
        return;
    shutdown(context->socket, SHUT_RDWR);
    close(context->socket);
    context->socket = -1;
#endif
}
