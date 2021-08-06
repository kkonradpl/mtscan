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
#include "log.h"
#include "mtscan.h"

#ifdef G_OS_WIN32
#include "win32.h"
#endif

typedef struct mtscan_arg
{
    const gchar *config_path;
    const gchar *output_file;
    gint auto_connect;
    gint tzsp_port;
    const gchar *autosave_dir;
    gboolean batch_mode;
    gboolean skip_verification;
    gboolean skip_scanlist_warning;
    gboolean strip_samples;
    gboolean strip_gps;
    gboolean strip_azi;
} mtscan_arg_t;

static mtscan_arg_t args =
{
    .config_path = NULL,
    .output_file = NULL,
    .auto_connect = 0,
    .tzsp_port = 0,
    .autosave_dir = NULL,
    .batch_mode = FALSE,
    .skip_verification = FALSE,
    .skip_scanlist_warning = FALSE,
    .strip_samples = FALSE,
    .strip_gps = FALSE,
    .strip_azi = FALSE
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
mtscan_usage(void)
{
    printf("mtscan " APP_VERSION " - MikroTik RouterOS wireless scanner\n");
    printf("usage: mtscan [-c config] [-o log] [-a profile] [-t port] [-b] [-s] [-w] [-S] [-A] [-G] logs...\n");
    printf("options:\n");
    printf("  -c  configuration file\n");
    printf("  -o  output log file\n");
    printf("  -a  auto-connect to a given profile id\n");
    printf("  -t  override TZSP UDP port\n");
    printf("  -d  override autosave directory and enable it\n");
    printf("  -b  headless batch mode, requires -o\n");
    printf("  -s  skip SSH key verification\n");
    printf("  -w  skip scan-list warning\n");
    printf("  -S  strip signal samples (input/output log)\n");
    printf("  -G  strip GPS data (output log)\n");
    printf("  -A  strip azimuth data (output log)\n");
}

static void
parse_args(gint   argc,
           gchar *argv[])
{
    gint c;
    while((c = getopt(argc, argv, "hc:o:a:t:d:bswSGA")) != -1)
    {
        switch(c)
        {
        case 'h':
            mtscan_usage();
            exit(0);
            break;

        case 'c':
            args.config_path = optarg;
            break;
            
        case 'o':
            args.output_file = optarg;
            break;

        case 'a':
            args.auto_connect = atoi(optarg);
            break;
            
        case 't':
            args.tzsp_port = atoi(optarg);
            if(args.tzsp_port <= 0 || args.tzsp_port > 65535)
            {
                fprintf(stderr, "WARNING: Invalid TZSP UDP port given, using default.\n");
                args.tzsp_port = 0;
            }
            break;

        case 'd':
            args.autosave_dir = optarg;
            break;

        case 'b':
            args.batch_mode = 1;
            break;

        case 's':
            args.skip_verification = 1;
            break;

        case 'w':
            args.skip_scanlist_warning = 1;
            break;

        case 'S':
            args.strip_samples = 1;
            break;

        case 'G':
            args.strip_gps = 1;
            break;

        case 'A':
            args.strip_azi = 1;
            break;

        case '?':
            if(optopt == 'c')
                fprintf(stderr, "ERROR: No configuration path given, using default.\n");
            else if(optopt == 'o')
                fprintf(stderr, "ERROR: No output file specified.\n");
            else if(optopt == 'a')
                fprintf(stderr, "ERROR: No auto-connect profile index given.\n");
            else if(optopt == 't')
                fprintf(stderr, "ERROR: No TZSP UDP port given, using default.\n");

            mtscan_usage();
            break;

        default:
            break;
        }
    }
}

static void
log_read_network_cb(network_t *network,
                    gpointer   user_data)
{
    gboolean merge = GPOINTER_TO_INT(user_data);
    mtscan_model_add(ui.model, network, merge);
}

static void
log_open(gint   argc,
         gchar *argv[])
{
    GSList *filenames = NULL;
    gint count;
    gint i;

    if(args.batch_mode)
    {
        for(i = optind; i < argc; i++)
        {
            count = log_read(argv[i], log_read_network_cb, GINT_TO_POINTER(i != optind), args.strip_samples);
            if(count <= 0)
            {
                switch(count)
                {
                    case LOG_READ_ERROR_OPEN:
                        fprintf(stderr, "ERROR: Failed to open a file: %s\n", argv[i]);
                        break;

                    case LOG_READ_ERROR_READ:
                        fprintf(stderr, "ERROR: Failed to read a file: %s\n", argv[i]);
                        break;

                    case LOG_READ_ERROR_PARSE:
                    case LOG_READ_ERROR_EMPTY:
                    default:
                        fprintf(stderr, "ERROR: Failed to parse a file: %s\n", argv[i]);
                        break;
                }
            }
        }
    }
    else
    {
        for(i = optind; i < argc; i++)
            filenames = g_slist_prepend(filenames, g_strdup(argv[i]));

        if(filenames)
        {
            filenames = g_slist_reverse(filenames);
            ui_log_open(filenames, (g_slist_length(filenames) > 1), args.strip_samples);
            g_slist_free_full(filenames, g_free);
        }
    }
}

gint
main(gint   argc,
     gchar *argv[])
{
    gboolean init;
    log_save_error_t *error;
    const gchar **file;

    /* hack for the yajl bug:
       https://github.com/lloyd/yajl/issues/79 */
    gtk_disable_setlocale();
    init = gtk_init_check(&argc, &argv);
    parse_args(argc, argv);

    if(args.batch_mode && !args.output_file)
    {
        fprintf(stderr, "ERROR: Batch mode requires an output file, giving up.\n");
        mtscan_usage();
        return -1;
    }

    memset(&ui, 0, sizeof(ui));
    ui.model = mtscan_model_new();

    if(!args.batch_mode)
    {
        if(!init)
        {
            fprintf(stderr, "ERROR: GTK initialization failed (no DISPLAY?)\n");
            mtscan_usage();
            return -1;
        }

#ifdef G_OS_WIN32
        win32_init();
#endif
        curl_global_init(CURL_GLOBAL_SSL);

        /* Load the configuration file */
        conf_init(args.config_path);

        /* Override the TZSP UDP port, if given */
        if(args.tzsp_port)
            conf_set_preferences_tzsp_udp_port(args.tzsp_port);

        /* Override the autosave directory, if given */
        if(args.autosave_dir)
        {
            conf_set_interface_autosave(TRUE);
            conf_set_path_autosave(args.autosave_dir);
        }

        ui_init();
    }

    /* Load logs, if any */
    log_open(argc, argv);

    /* Save the log to a file */
    if(args.output_file)
    {
        if(args.batch_mode)
        {
            error = log_save(args.output_file, args.strip_samples, args.strip_gps, args.strip_azi, NULL);
            if(error)
            {
                fprintf(stderr, "ERROR: Failed to save the log: %s\n", args.output_file);
                g_free(error);
                return -1;
            }
            return 0;
        }
        ui_log_save_full(args.output_file, args.strip_samples, args.strip_gps, args.strip_azi, NULL, TRUE);
    }

    /* Skip SSH key verification */
    if(args.skip_verification)
        conf_set_runtime_skip_verification(TRUE);

    /* Skip scan-list warning */
    if(args.skip_scanlist_warning)
        conf_set_runtime_skip_scanlist_warning(TRUE);

    /* Perform auto-connect, if a profile number is given */
    if(args.auto_connect > 0)
        ui_toggle_connection(args.auto_connect);

    /* Load the OUI database in a separate thread */
    for(file = oui_files; *file && !oui_init(*file); file++);

    /* Main thread loop */
    gtk_main();

    oui_destroy();
    mtscan_model_free(ui.model);
#ifdef G_OS_WIN32
    win32_cleanup();
#endif
    return 0;
}
