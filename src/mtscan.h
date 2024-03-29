/*
 *  MTscan - MikroTik RouterOS wireless scanner
 *  Copyright (c) 2015-2023  Konrad Kosmatka
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

#ifndef MTSCAN_H_
#define MTSCAN_H_
#include <glib.h>

#define APP_NAME          "MTscan"
#define APP_VERSION       "0.7-git"
#define APP_ICON          "mtscan"
#define APP_FILE_EXT      ".mtscan"
#define APP_FILE_COMPRESS ".gz"

#ifdef G_OS_WIN32
#define APP_SOUND_DIR "..\\share\\sounds\\mtscan"
#else
#define APP_SOUND_DIR "/usr/share/sounds/mtscan"
#endif

#define APP_SOUND_NETWORK_NORMAL    "network.wav"
#define APP_SOUND_NETWORK_HIGHLIGHT "network-highlight.wav"
#define APP_SOUND_NETWORK_WARNING   "network-warning.wav"
#define APP_SOUND_NETWORK_ALARM     "network-alarm.wav"
#define APP_SOUND_GNSS_LOST         "gnss-lost.wav"
#define APP_SOUND_CONNECTION_LOST   "conn-lost.wav"

#define APP_LICENCE \
"This program is free software; you can redistribute it and/or\n" \
"modify it under the terms of the GNU General Public License\n" \
"as published by the Free Software Foundation; either version 2\n" \
"of the License, or (at your option) any later version.\n" \
"\n" \
"This program is distributed in the hope that it will be useful,\n" \
"but WITHOUT ANY WARRANTY; without even the implied warranty of\n" \
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the\n" \
"GNU General Public License for more details.\n" \
"\n" \
"You should have received a copy of the GNU General Public License\n" \
"along with this program. If not, see http://www.gnu.org/licenses/\n"

#endif


