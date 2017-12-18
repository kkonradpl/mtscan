#ifndef MTSCAN_MISC_H_
#define MTSCAN_MISC_H_

gint gptrcmp(gconstpointer, gconstpointer);
gint gint64cmp(const gint64*, const gint64*);
gint64* gint64dup(const gint64*);

void remove_char(gchar*, gchar);
gchar* str_scanlist_compress(const gchar*);

GtkListStore* create_liststore_from_tree(GTree*);
void fill_tree_from_liststore(GTree*, GtkListStore*);

gint64 str_addr_to_gint64(const gchar*, gint);

#endif
