/*
 *  MTscan - MikroTik RouterOS wireless scanner
 *  Copyright (c) 2015-2021  Konrad Kosmatka
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

#define COBJMACROS 1
#include <glib.h>
#include <sensorsapi.h>
#include <sensors.h>
#include <math.h>
#include "wsa.h"
#include "msg.h"
#include "data.h"

#define WSA_UPDATE_RATE 1
#define DEBUG 1
#define EPS 10e-6

typedef struct gnss_wsa
{
    /* Configuration */
    gint sensor_id;
    guint reconnect;

    /* Callback pointers */
    void (*cb)    (gnss_wsa_t*);
    void (*cb_msg)(const gnss_msg_t*);

    /* Thread cancelation flag */
    volatile gboolean canceled;

    /* Private data */
    ISensorManager *sensor_manager;
    ISensorCollection *sensor_list;
    ULONG sensor_count;
    gchar *name;
    ISensor *sensor;
} gnss_wsa_t;

static gpointer gnss_wsa_thread(gpointer);
static gboolean gnss_wsa_open(gnss_wsa_t*);
static gboolean gnss_wsa_read(gnss_wsa_t*);
static gboolean gnss_wsa_close(gnss_wsa_t*);

static gboolean cb_wsa(gpointer);
static gboolean cb_msg(gpointer);


gnss_wsa_t*
gnss_wsa_new(void (*cb)(gnss_wsa_t*),
             void (*cb_msg)(const gnss_msg_t*),
             gint   sensor_id,
             guint  reconnect)
{
    gnss_wsa_t *context;
    context = g_malloc(sizeof(gnss_wsa_t));

    /* Configuration */
    context->sensor_id = sensor_id;
    context->reconnect = reconnect;

    /* Callback pointers */
    context->cb = cb;
    context->cb_msg = cb_msg;

    /* Thread cancelation flag */
    context->canceled = FALSE;

    /* Private data */
    context->sensor_manager = NULL;
    context->sensor_list = NULL;
    context->sensor_count = 0;
    context->name = NULL;
    context->sensor = NULL;

    g_thread_unref(g_thread_new("gnss_wsa_thread", gnss_wsa_thread, (gpointer)context));
    return context;
}

void
gnss_wsa_free(gnss_wsa_t *context)
{
    if(context)
    {
        g_free(context->name);
        g_free(context);
    }
}

void
gnss_wsa_cancel(gnss_wsa_t *context)
{
    context->canceled = TRUE;
}

gint
gnss_wsa_get_sensor_id(const gnss_wsa_t *context)
{
    return context->sensor_id;
}

static gpointer
gnss_wsa_thread(gpointer user_data)
{
    gnss_wsa_t *context = (gnss_wsa_t*)user_data;
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

#if DEBUG
    g_print("gnss_wsa@%p: thread start\n", (gpointer)context);
#endif

    do
    {
        if(gnss_wsa_open(context))
        {
            g_idle_add(cb_msg, gnss_msg_new(context, GNSS_MSG_INFO, GINT_TO_POINTER(GNSS_INFO_READY)));

            while(gnss_wsa_read(context) && !context->canceled)
                g_usleep(WSA_UPDATE_RATE * G_USEC_PER_SEC);

            gnss_wsa_close(context);
            g_idle_add(cb_msg, gnss_msg_new(context, GNSS_MSG_INFO, GINT_TO_POINTER(GNSS_INFO_CLOSED)));
        }

        if(!context->canceled && context->reconnect)
        {
            /* Reconnect with delay */
            g_usleep(context->reconnect * G_USEC_PER_SEC);
        }
    } while(!context->canceled && context->reconnect);

#if DEBUG
    g_print("gnss_wsa@%p: thread stop\n", (gpointer)context);
#endif

    g_idle_add(cb_wsa, context);

    CoUninitialize();
    return NULL;
}

static gboolean
gnss_wsa_open(gnss_wsa_t *context)
{
    HRESULT hr;
    BSTR name = NULL;

#if DEBUG
    g_print("gnss_wsa@%p: opening\n", (gpointer)context);
#endif
    g_idle_add(cb_msg, gnss_msg_new(context, GNSS_MSG_INFO, GINT_TO_POINTER(GNSS_INFO_OPENING)));

    hr = CoCreateInstance(&CLSID_SensorManager, 0, CLSCTX_ALL, &IID_ISensorManager, (void**)&context->sensor_manager);
    if(hr != 0 || !context->sensor_manager)
    {
#if DEBUG
        g_print("gnss_wsa@%p: failed to create sensor manager\n", (gpointer)context);
#endif
        g_idle_add(cb_msg, gnss_msg_new(context, GNSS_MSG_INFO, GINT_TO_POINTER(GNSS_INFO_ERR_UNAVAILABLE)));
        gnss_wsa_close(context);
        return FALSE;
    }

    hr = ISensorManager_GetSensorsByCategory(context->sensor_manager, &SENSOR_CATEGORY_LOCATION, &context->sensor_list);
    if(hr != 0 || !context->sensor_list)
    {
#if DEBUG
        g_print("gnss_wsa@%p: failed to obtain sensor list\n", (gpointer)context);
#endif
        g_idle_add(cb_msg, gnss_msg_new(context, GNSS_MSG_INFO, GINT_TO_POINTER(GNSS_INFO_ERR_UNAVAILABLE)));
        gnss_wsa_close(context);
        return FALSE;
    }

    hr = ISensorManager_RequestPermissions(context->sensor_manager, 0, context->sensor_list, TRUE);
    if(hr != 0)
    {
#if DEBUG
        g_print("gnss_wsa@%p: access denied\n", (gpointer)context);
#endif
        g_idle_add(cb_msg, gnss_msg_new(context, GNSS_MSG_INFO, GINT_TO_POINTER(GNSS_INFO_ERR_ACCESS)));
        gnss_wsa_close(context);
        return FALSE;
    }

    hr = ISensorCollection_GetCount(context->sensor_list, &context->sensor_count);
    if(hr != 0 || !context->sensor_count)
    {
#if DEBUG
        g_print("gnss_wsa@%p: no sensors available\n", (gpointer)context);
#endif
        g_idle_add(cb_msg, gnss_msg_new(context, GNSS_MSG_INFO, GINT_TO_POINTER(GNSS_INFO_ERR_UNAVAILABLE)));
        gnss_wsa_close(context);
        return FALSE;
    }

    hr = ISensorCollection_GetAt(context->sensor_list, context->sensor_id, &context->sensor);
    if(hr != 0 || !context->sensor)
    {
#if DEBUG
        g_print("gnss_wsa@%p: sensor unavailable\n", (gpointer)context);
#endif
        g_idle_add(cb_msg, gnss_msg_new(context, GNSS_MSG_INFO, GINT_TO_POINTER(GNSS_INFO_ERR_UNAVAILABLE)));
        gnss_wsa_close(context);
        return FALSE;
    }

    hr = ISensor_GetFriendlyName(context->sensor, &name);
    if(hr != 0 || !name)
    {
#if DEBUG
        g_print("gnss_wsa@%p: unable to get sensor name\n", (gpointer)context);
#endif
        g_idle_add(cb_msg, gnss_msg_new(context, GNSS_MSG_INFO, GINT_TO_POINTER(GNSS_INFO_ERR_UNAVAILABLE)));
        gnss_wsa_close(context);
        return FALSE;
    }

    context->name = g_strdup_printf("%ls", name);
    SysFreeString(name);

#if DEBUG
    g_print("gnss_wsa@%p: connected, sensor: %s\n", (gpointer)context, context->name);
#endif

    return TRUE;
}

static gboolean
gnss_wsa_read(gnss_wsa_t *context)
{
    HRESULT hr;
    SensorState state;
    ISensorDataReport *report = NULL;
    IPortableDeviceKeyCollection *keys = NULL;
    ULONG count = 0;
    guint i;
    PROPERTYKEY key;
    PROPVARIANT value;
    gnss_data_t *data;
    gboolean fix = FALSE;

    hr = ISensor_GetState(context->sensor, &state);
    if(hr != 0)
    {
#if DEBUG
        g_print("gnss_wsa@%p: failed to get sensor state\n", (gpointer)context);
#endif
        return FALSE;
    }

    switch(state)
    {
        case SENSOR_STATE_NOT_AVAILABLE:
#if DEBUG
            g_print("gnss_wsa@%p: sensor state not available\n", (gpointer)context);
#endif
            g_idle_add(cb_msg, gnss_msg_new(context, GNSS_MSG_INFO, GINT_TO_POINTER(GNSS_INFO_ERR_UNAVAILABLE)));
            return FALSE;

        case SENSOR_STATE_ERROR:
#if DEBUG
            g_print("gnss_wsa@%p: sensor state error\n", (gpointer)context);
#endif
            g_idle_add(cb_msg, gnss_msg_new(context, GNSS_MSG_INFO, GINT_TO_POINTER(GNSS_INFO_ERR_UNAVAILABLE)));
            return FALSE;

        case SENSOR_STATE_ACCESS_DENIED:
#if DEBUG
            g_print("gnss_wsa@%p: sensor state access denied\n", (gpointer)context);
#endif
            g_idle_add(cb_msg, gnss_msg_new(context, GNSS_MSG_INFO, GINT_TO_POINTER(GNSS_INFO_ERR_ACCESS)));
            return FALSE;

        case SENSOR_STATE_NO_DATA:
            return TRUE;

        case SENSOR_STATE_INITIALIZING:
        case SENSOR_STATE_READY:
            break;
    }
    
    hr = ISensor_GetData(context->sensor, &report);
    if(hr != 0 || !report)
    {
#if DEBUG
        g_print("gnss_wsa@%p: unable to get sensor data\n", (gpointer)context);
#endif
        return FALSE;
    }

    hr = ISensor_GetSupportedDataFields(context->sensor, &keys);
    if(hr != 0 || !keys)
    {
#if DEBUG
        g_print("gnss_wsa@%p: unable to get supported data fields\n", (gpointer)context);
#endif
        ISensorDataReport_Release(report);
        return FALSE;
    }

    IPortableDeviceKeyCollection_GetCount(keys, &count);
    if(hr != 0)
    {
#if DEBUG
        g_print("gnss_wsa@%p: unable to get count of data keys\n", (gpointer)context);
#endif
        IPortableDeviceKeyCollection_Release(keys);
        ISensorDataReport_Release(report);
        return FALSE;
    }

    data = gnss_data_new();
    gnss_data_set_device(data, context->name, strlen(context->name));

    for(i=0; i<count; i++)
    {
        if(IPortableDeviceKeyCollection_GetAt(keys, i, &key) == S_OK)
        {
            if(!IsEqualGUID(&key.fmtid, &SENSOR_DATA_TYPE_LOCATION_GUID))
               continue;

            if(ISensorDataReport_GetSensorValue(report, &key, &value) == S_OK)
            {
                if(IsEqualPropertyKey(key, SENSOR_DATA_TYPE_LATITUDE_DEGREES))
                    gnss_data_set_lat(data, value.dblVal);
                else if(IsEqualPropertyKey(key, SENSOR_DATA_TYPE_LONGITUDE_DEGREES))
                    gnss_data_set_lon(data, value.dblVal);
                else if(IsEqualPropertyKey(key, SENSOR_DATA_TYPE_ALTITUDE_SEALEVEL_METERS))
                    gnss_data_set_alt(data, value.dblVal);
                else if(IsEqualPropertyKey(key, SENSOR_DATA_TYPE_ERROR_RADIUS_METERS))
                {
                    if(fabs(value.dblVal) > EPS)
                    {
                        gnss_data_set_epx(data, value.dblVal);
                        gnss_data_set_epy(data, value.dblVal);
                    }
                }
                else if(IsEqualPropertyKey(key, SENSOR_DATA_TYPE_ALTITUDE_SEALEVEL_ERROR_METERS))
                {
                    if(fabs(value.dblVal) > EPS)
                        gnss_data_set_epv(data, value.dblVal);
                }
                else if(IsEqualPropertyKey(key, SENSOR_DATA_TYPE_FIX_QUALITY))
                    fix = value.lVal;

                PropVariantClear(&value);
            }
        }
    }

    IPortableDeviceKeyCollection_Release(keys);
    ISensorDataReport_Release(report);

    if(state == SENSOR_STATE_READY &&
       fix &&
       fabs(gnss_data_get_lat(data)) > EPS &&
       fabs(gnss_data_get_lon(data)) > EPS &&
       fabs(gnss_data_get_alt(data)) > EPS)
    {
        gnss_data_set_mode(data, GNSS_MODE_3D);
    }
    else
    {
        gnss_data_set_mode(data, GNSS_MODE_NONE);
    }

#if DEBUG
        g_print("gnss_wsa@%p: device=%s, mode=%s: lat=%f lon=%f alt=%f epx/y=%f epv=%f\n",
                (gpointer)context,
                gnss_data_get_device(data),
                (gnss_data_get_mode(data) == GNSS_MODE_NONE ? "none" :
                 (gnss_data_get_mode(data) == GNSS_MODE_2D ? "2D" :
                 (gnss_data_get_mode(data) == GNSS_MODE_3D ? "3D" : "unknown"))),
                gnss_data_get_lat(data),
                gnss_data_get_lon(data),
                gnss_data_get_alt(data),
                gnss_data_get_epx(data),
                gnss_data_get_epv(data));
#endif

    g_idle_add(cb_msg, gnss_msg_new(context, GNSS_MSG_DATA, data));
    return TRUE;
}

static gboolean
gnss_wsa_close(gnss_wsa_t *context)
{
#if DEBUG
    g_print("gnss_wsa@%p: closed\n", (gpointer)context);
#endif

    g_free(context->name);
    context->name = NULL;

    if(context->sensor)
    {
        ISensor_Release(context->sensor);
        context->sensor = NULL;
    }

    if(context->sensor_list)
    {
        ISensorCollection_Release(context->sensor_list);
        context->sensor_list = NULL;
    }

    if(context->sensor_manager)
    {
        ISensorManager_Release(context->sensor_manager);
        context->sensor_manager = NULL;        
    }

    return TRUE;
}

static gboolean
cb_wsa(gpointer user_data)
{
    gnss_wsa_t *context = (gnss_wsa_t*)user_data;
    context->cb(context);
    return FALSE;
}

static gboolean
cb_msg(gpointer user_data)
{
    gnss_msg_t *msg = (gnss_msg_t*)user_data;
    const gnss_wsa_t *src = gnss_msg_get_src(msg);

    if(!src->canceled)
        src->cb_msg(msg);

    gnss_msg_free(msg);
    return FALSE;
}
