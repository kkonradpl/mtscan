#define _WIN32_WINNT 0x0500
#include <gtk/gtk.h>
#include <gdk/gdkwin32.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <mmsystem.h>
#include "ui-dialogs.h"
#include "win32.h"

#define WIN32_FONT_FILE "DejaVuSansMono.ttf"
static gint win32_font;

void
win32_init(void)
{
    WSADATA wsaData;
    if(WSAStartup(MAKEWORD(2,2), &wsaData))
        ui_dialog(NULL, GTK_MESSAGE_ERROR, "Error", "Unable to initialize Winsock");

    win32_font = AddFontResourceEx(WIN32_FONT_FILE, FR_PRIVATE, NULL);
}

void
win32_cleanup(void)
{
    WSACleanup();
    if(win32_font)
        RemoveFontResourceEx(WIN32_FONT_FILE, FR_PRIVATE, NULL);
}

void
win32_uri(const gchar *uri)
{
    ShellExecute(0, "open", uri, NULL, NULL, 1);
}

gboolean
win32_uri_signal(GtkWidget *label,
                 gchar     *uri,
                 gpointer   data)
{
    win32_uri(uri);
    return TRUE;
}

void
win32_play(const gchar *path)
{
    PlaySound(path, NULL, SND_FILENAME | SND_ASYNC);
}

gchar*
strsep(gchar       **string,
       const gchar  *del)
{
    gchar *start = *string;
    gchar *p = (start ? strpbrk(start, del) : NULL);

    if(!p)
    {
        *string = NULL;
    }
    else
    {
        *p = '\0';
        *string = p + 1;
    }
    return start;
}
