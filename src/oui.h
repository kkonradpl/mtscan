#ifndef MTSCAN_OUI_H_
#define MTSCAN_OUI_H_

gboolean oui_init(const gchar*);
const gchar* oui_lookup(gint64);
void oui_destroy();

#endif

