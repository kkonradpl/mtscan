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

/* Based on reverse engineering, may be not fully accurate */

#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <openssl/aes.h>
#include <string.h>
#include "ie-airmax-ac.h"
#include "utils.h"

#define IE_AIRMAX_AC_HEADER_LEN   10
#define IE_AIRMAX_AC_DATA_LEN_IDX 9

#define IE_AIRMAX_AC_DATA_HEADER_LEN 22
#define IE_AIRMAX_AC_ADDR1 2
#define IE_AIRMAX_AC_ADDR2 8
#define IE_AIRMAX_AC_MODE  17

#define IE_AIRMAX_AC_MODE_PTP    (1 << 0)
#define IE_AIRMAX_AC_MODE_PTMP   (1 << 1)
#define IE_AIRMAX_AC_MODE_MIXED1 (1 << 2)
#define IE_AIRMAX_AC_MODE_MIXED2 (1 << 3)
#define IE_AIRMAX_AC_MODE_MIXED3 (1 << 4)

#define IE_AIRMAX_AC_TAG_HEADER_LEN 2
#define IE_AIRMAX_AC_TAG_PADDING    0x00
#define IE_AIRMAX_AC_TAG_RADIONAME  0x01
#define IE_AIRMAX_AC_TAG_SSID       0x02

typedef struct ie_airmax_ac
{
    uint8_t mode;
    char *radioname;
    char *ssid;
} ie_airmax_ac_t;

static ie_airmax_ac_t* ie_airmax_ac_process_data(const uint8_t*, uint8_t);
static void ie_airmax_ac_process_tag(ie_airmax_ac_t*, uint8_t, uint8_t, const uint8_t*);

ie_airmax_ac_t*
ie_airmax_ac_parse(const uint8_t *ie,
                   uint8_t        ie_len,
                   const uint8_t  addr[6])
{
    static const uint8_t magic[] = { 0x00, 0x27, 0x22, 0xff, 0xff, 0xff, 0x02, 0x01, 0x00 };
    static const uint8_t hmac_key[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
    uint8_t data[UINT8_MAX];
    uint8_t data_len;
    uint8_t hmac[SHA_DIGEST_LENGTH];
    AES_KEY aes_key;
    int i;

    if(ie_len < IE_AIRMAX_AC_HEADER_LEN + IE_AIRMAX_AC_DATA_HEADER_LEN)
        return NULL;

    /* The IE must start with a magic byte sequence */
    if(memcmp(magic, ie, sizeof(magic)) != 0)
        return NULL;

    data_len = ie[IE_AIRMAX_AC_DATA_LEN_IDX];

    /* Data must be aligned to 128-bit blocks */
    if(data_len % 16)
        return NULL;

    /* The IE length must match the data length */
    if(IE_AIRMAX_AC_HEADER_LEN + data_len < ie_len)
        return NULL;

    /* Generate the AES key */
    if(HMAC(EVP_sha1(), hmac_key, 6, addr, 6, hmac, NULL))
    {
        /* The AES uses only 128 bits (16B) of the HMAC-SHA1 hash */
        AES_set_decrypt_key(hmac, 128, &aes_key);

        /* Decrypt each 128-bit data block */
        for(i=0; i<data_len; i+=16)
            AES_decrypt(ie + IE_AIRMAX_AC_HEADER_LEN + i, data + i, &aes_key);

        /* Verify the data */
        if(memcmp(data+IE_AIRMAX_AC_ADDR1, addr, 6) != 0 ||
           memcmp(data+IE_AIRMAX_AC_ADDR2, addr, 6) != 0)
        {
            return NULL;
        }

        return ie_airmax_ac_process_data(data, data_len);
    }
    return NULL;
}

static ie_airmax_ac_t*
ie_airmax_ac_process_data(const uint8_t *data,
                          uint8_t        data_len)
{
    ie_airmax_ac_t *context;
    uint8_t tag_len;
    int i;

    context = calloc(sizeof(ie_airmax_ac_t), 1);
    if(context == NULL)
        return NULL;

    context->mode = data[IE_AIRMAX_AC_MODE];

    for(i=IE_AIRMAX_AC_DATA_HEADER_LEN;
        i+IE_AIRMAX_AC_TAG_HEADER_LEN <= data_len;
        i+=IE_AIRMAX_AC_TAG_HEADER_LEN + tag_len)
    {
        if(data[i] == IE_AIRMAX_AC_TAG_PADDING)
            break;

        tag_len = data[i+1];
        if((i + IE_AIRMAX_AC_TAG_HEADER_LEN + tag_len) > data_len)
            break;

        ie_airmax_ac_process_tag(context, data[i], tag_len, data+i+IE_AIRMAX_AC_TAG_HEADER_LEN);
    }

    return context;
}

static void
ie_airmax_ac_process_tag(ie_airmax_ac_t *context,
                         uint8_t         type,
                         uint8_t         len,
                         const uint8_t  *value)
{
    if(type == IE_AIRMAX_AC_TAG_RADIONAME &&
       !context->radioname &&
       len)
    {
        context->radioname = tzsp_utils_string(value, len);
    }
    else if(type == IE_AIRMAX_AC_TAG_SSID &&
            !context->ssid &&
            len)
    {
        context->ssid = tzsp_utils_string(value, len);
    }
}

bool
ie_airmax_ac_is_ptp(ie_airmax_ac_t *context)
{
    return (context->mode & IE_AIRMAX_AC_MODE_PTP) != 0;
}

bool
ie_airmax_ac_is_ptmp(ie_airmax_ac_t *context)
{
    return (context->mode & IE_AIRMAX_AC_MODE_PTMP) != 0;
}

bool
ie_airmax_ac_is_mixed(ie_airmax_ac_t *context)
{
    return (context->mode & IE_AIRMAX_AC_MODE_MIXED1) &&
           (context->mode & IE_AIRMAX_AC_MODE_MIXED2) &&
           (context->mode & IE_AIRMAX_AC_MODE_MIXED3);
}

const char*
ie_airmax_ac_get_radioname(ie_airmax_ac_t *context)
{
    return context->radioname;
}

const char*
ie_airmax_ac_get_ssid(ie_airmax_ac_t *context)
{
    return context->ssid;
}

void
ie_airmax_ac_free(ie_airmax_ac_t *context)
{
    if(context)
    {
        free(context->radioname);
        free(context->ssid);
        free(context);
    }
}
