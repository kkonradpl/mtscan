#ifndef MTSCAN_MISC_H_
#define MTSCAN_MISC_H_

gint gintcmp(gconstpointer, gconstpointer);
void remove_char(gchar*, gchar);
gchar* str_scanlist_compress(const gchar*);

GtkListStore* create_liststore_from_tree(GTree*);
void fill_tree_from_liststore(GTree*, GtkListStore*);

#endif
