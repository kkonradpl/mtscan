/*
 *  MTscan
 *  Copyright (C) 2015-2017  Konrad Kosmatka

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
#include "conf.h"
#include "ui.h"
#include "log.h"
#include "model.h"
#ifdef G_OS_WIN32
#include "win32.h"
#endif

static const gchar*
get_config_path(gint   argc,
                gchar *argv[])
{
    gchar *path = NULL;
    gint c;
    while((c = getopt(argc, argv, "c:")) != -1)
    {
        switch(c)
        {
        case 'c':
            path = optarg;
            break;
        case '?':
            if(optopt == 'c')
                fprintf(stderr, "No configuration path argument found, using default.\n");
            break;
        default:
            printf("%c\n", c);
            break;
        }
    }
    return path;
}

gint
main(gint   argc,
     gchar *argv[])
{
    GSList *filenames = NULL;
    gint i;

    /* hack for the yajl bug:
       https://github.com/lloyd/yajl/issues/79 */
    gtk_disable_setlocale();

    gtk_init(&argc, &argv);

#ifdef G_OS_WIN32
    win32_init();
#endif

    conf_init(get_config_path(argc, argv));

    memset(&ui, 0, sizeof(ui));
    ui.model = mtscan_model_new();
    ui_init();

    for(i=optind; i<argc; i++)
        filenames = g_slist_prepend(filenames, g_strdup(argv[i]));

    if(filenames)
    {
        filenames = g_slist_reverse(filenames);
        log_open(filenames, (g_slist_length(filenames) > 1));
        g_slist_free_full(filenames, g_free);
    }

    gtk_main();

    mtscan_model_free(ui.model);
#ifdef G_OS_WIN32
    win32_cleanup();
#endif
    return 0;
}
