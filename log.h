#ifndef MTSCAN_LOG_H_
#define MTSCAN_LOG_H_
#include <gtk/gtk.h>

void log_open(GSList*, gboolean);
gboolean log_save(gchar*, gboolean, gboolean, gboolean, GList*);

#endif
