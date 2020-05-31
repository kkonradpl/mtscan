/*
 *  MTscan - MikroTik RouterOS wireless scanner
 *  Copyright (c) 2015-2020  Konrad Kosmatka
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

#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <curl/curl.h>
#include "conf.h"
#include "ui.h"
#include "ui-log.h"
#include "model.h"
#include "oui.h"

#ifdef G_OS_WIN32
#include "win32.h"
#endif

typedef struct mtscan_arg
{
    const gchar *config_path;
    gint auto_connect;
} mtscan_arg_t;

static mtscan_arg_t args =
{
    .config_path = NULL,
    .auto_connect = 0
};

static const gchar *oui_files[] =
{
#ifdef G_OS_WIN32
    "..\\etc\\manuf",
    "etc\\manuf",
#else
    "/etc/manuf",
    "/usr/share/wireshark/manuf",
    "/usr/share/wireshark/wireshark/manuf",
#endif
    NULL
};


static void
parse_args(gint   argc,
           gchar *argv[])
{
    gint c;
    while((c = getopt(argc, argv, "c:a:")) != -1)
    {
        switch(c)
        {
        case 'c':
            args.config_path = optarg;
            break;

        case 'a':
            args.auto_connect = atoi(optarg);
            break;

        case '?':
            if(optopt == 'c')
                fprintf(stderr, "No configuration path given, using default.\n");
            else if(optopt == 'a')
                fprintf(stderr, "No auto-connect profile index given.\n");
            break;

        default:
            break;
        }
    }
}

gint
main(gint   argc,
     gchar *argv[])
{
    GSList *filenames = NULL;
    const gchar **file;
    gint i;

    /* hack for the yajl bug:
       https://github.com/lloyd/yajl/issues/79 */
    gtk_disable_setlocale();

    gtk_init(&argc, &argv);

#ifdef G_OS_WIN32
    win32_init();
#endif

    curl_global_init(CURL_GLOBAL_SSL);

    parse_args(argc, argv);
    conf_init(args.config_path);

    memset(&ui, 0, sizeof(ui));
    ui.model = mtscan_model_new();
    ui_init();

    for(i=optind; i<argc; i++)
        filenames = g_slist_prepend(filenames, g_strdup(argv[i]));

    if(filenames)
    {
        filenames = g_slist_reverse(filenames);
        ui_log_open(filenames, (g_slist_length(filenames) > 1), FALSE);
        g_slist_free_full(filenames, g_free);
    }

    if(args.auto_connect > 0)
        ui_toggle_connection(args.auto_connect);

    for(file = oui_files; *file && !oui_init(*file); file++);

    gtk_main();

    oui_destroy();
    mtscan_model_free(ui.model);

#ifdef G_OS_WIN32
    win32_cleanup();
#endif
    return 0;
}
