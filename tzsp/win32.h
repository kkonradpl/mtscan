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

#ifndef MTSCAN_TZSP_WIN32_H
#define MTSCAN_TZSP_WIN32_H

#include <stdint.h>

#define ETHERTYPE_IP 0x0800
#define ETH_ALEN 6
#define ETH_HLEN 14

struct ether_header
{
    uint8_t  ether_dhost[ETH_ALEN];
    uint8_t  ether_shost[ETH_ALEN];
    uint16_t ether_type;
} __attribute__ ((__packed__));

struct ip
{
#if REG_DWORD == REG_DWORD_LITTLE_ENDIAN
       u_char  ip_hl:4,                /* header length */
               ip_v:4;                 /* version */
#else
       u_char  ip_v:4,                 /* version */
               ip_hl:4;                /* header length */
#endif
       u_char  ip_tos;                 /* type of service */
       short   ip_len;                 /* total length */
       u_short ip_id;                  /* identification */
       short   ip_off;                 /* fragment offset field */
#define        IP_DF 0x4000                    /* dont fragment flag */
#define        IP_MF 0x2000                    /* more fragments flag */
       u_char  ip_ttl;                 /* time to live */
       u_char  ip_p;                   /* protocol */
       u_short ip_sum;                 /* checksum */
       struct  in_addr ip_src,ip_dst;  /* source and dest address */
};

#endif