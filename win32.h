#ifndef MTSCAN_WIN32_H_
#define MTSCAN_WIN32_H_
#include <gtk/gtk.h>

void win32_init();
void win32_cleanup();
void win32_uri(const gchar*);
gboolean win32_uri_signal(GtkWidget*, gchar*, gpointer);
void win32_play(const gchar*);
gchar* strsep(gchar**, const gchar*);

#endif
