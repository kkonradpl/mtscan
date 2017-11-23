#include <gtk/gtk.h>
#include <string.h>

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
