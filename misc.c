#include <gtk/gtk.h>
#include <string.h>

static gboolean create_liststore_from_tree_foreach(gpointer, gpointer, gpointer);
static gboolean fill_tree_from_liststore_foreach(GtkTreeModel*, GtkTreePath*, GtkTreeIter*, gpointer);

gint
gintcmp(gconstpointer a,
        gconstpointer b)
{
    return GPOINTER_TO_INT(a) - GPOINTER_TO_INT(b);
}

void
remove_char(gchar *str,
            gchar  c)
{
    gchar *pr = str;
    gchar *pw = str;
    while(*pr)
    {
        *pw = *pr++;
        pw += (*pw != c);
    }
    *pw = 0;
}

gchar*
str_scanlist_compress(const gchar *ptr)
{
    GString *output;
    gint prev, curr, last_written;

    if(!ptr)
        return NULL;

    output = g_string_new(NULL);

    prev = 0;
    last_written = 0;
    while(ptr)
    {
        curr = 0;
        sscanf(ptr, "%d", &curr);
        if(curr)
        {
            if(!prev)
            {
                g_string_append_printf(output, "%d", curr);
                prev = curr;
                last_written = curr;
            }
            else
            {
                if(prev != curr-5)
                {
                    if(last_written != prev)
                        g_string_append_printf(output, "-%d", prev);
                    g_string_append_printf(output, ",%d", curr);
                    last_written = curr;
                }
                prev = curr;
            }
        }

        ptr = strchr(ptr, ',');
        ptr = (ptr ? ptr + 1 : NULL);
    }

    if(last_written != prev)
        g_string_append_printf(output, "-%d", prev);

    return g_string_free(output, FALSE);
}

GtkListStore*
create_liststore_from_tree(GTree *tree)
{
    GtkListStore *model = gtk_list_store_new(1, G_TYPE_STRING);
    g_tree_foreach(tree, create_liststore_from_tree_foreach, model);
    return model;
}

static gboolean
create_liststore_from_tree_foreach(gpointer key,
                                   gpointer value,
                                   gpointer data)
{
    gchar *string = (gchar*)key;
    GtkListStore *model = (GtkListStore*)data;
    gtk_list_store_insert_with_values(model, NULL, -1, 0, string, -1);
    return FALSE;
}

void
fill_tree_from_liststore(GTree        *tree,
                         GtkListStore *model)
{
    g_tree_ref(tree);
    g_tree_destroy(tree);
    gtk_tree_model_foreach(GTK_TREE_MODEL(model), fill_tree_from_liststore_foreach, tree);
}

static gboolean
fill_tree_from_liststore_foreach(GtkTreeModel *model,
                                 GtkTreePath  *path,
                                 GtkTreeIter  *iter,
                                 gpointer      data)
{
    GTree *tree = (GTree*)data;
    gchar *string;

    gtk_tree_model_get(model, iter, 0, &string, -1);
    g_tree_insert(tree, string, GINT_TO_POINTER(TRUE));
    return FALSE;
}
