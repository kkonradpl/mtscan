#include <string.h>
#include "export.h"
#include "model.h"
#include "mtscan.h"
#include "ui-icons.h"

static const gchar *html_header =
"<!DOCTYPE html>\n"
"<head>\n"
"<!-- generated with " APP_NAME " " APP_VERSION " on %s -->\n"
"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />\n"
"<title>%s</title>\n"
"<style>\n"
"body {\n"
"    font-family: \"DejaVu Sans\", Verdana, sans-serif;\n"
"    background-color: #151515;\n"
"    color: #dceaf2;\n"
"}\n"
"\n"
"table#networks {\n"
"    background-color: #1c1c1c;\n"
"    margin-left: auto;\n"
"    margin-right: auto;\n"
"    margin-top: 10px;\n"
"    margin-bottom: 10px;\n"
"    border-collapse: collapse;\n"
"    border-style: hidden;\n"
"    font-size: small;\n"
"}\n"
"\n"
"table#networks thead {\n"
"    background-color: #2255ff;\n"
"    text-align: center;\n"
"    font-weight: bold;\n"
"    color: white;\n"
"}\n"
"\n"
"table#networks td {\n"
"    border: 1px solid #444444;\n"
"    padding: 1px 3px 1px 3px;\n"
"}\n"
"\n"
"table#networks tbody tr:hover {\n"
"    background-color: #3a3a3a !important;\n"
"}\n"
"\n"
"table#networks a {\n"
"    text-decoration: none;\n"
"}\n"
"\n"
"table#networks td.mono {\n"
"    font-family: \"DejaVu Mono\", monospace;\n"
"}\n"
"\n"
"div.icon {\n"
"    height: 1.2em;\n"
"    width: 1.2em;\n"
"    background-size: 100%% 100%%;\n"
"}\n"
"\n"
"div.priv {\n"
"    height: 100%%;\n"
"    width: 100%%;\n"
"    background-size: 100%% 100%%;\n"
"    background-image: url(data:image/svg+xml;charset=UTF-8,%%3Csvg%%20version%%3D%%221.1%%22%%20baseProfile%%3D%%22full%%22%%20width%%3D%%2248%%22%%20height%%3D%%2248%%22%%20xmlns%%3D%%22http%%3A%%2F%%2Fwww.w3.org%%2F2000%%2Fsvg%%22%%3E%%3Cpath%%20d%%3D%%22M19%%2C17%%20C19%%2C12%%2029%%2C12%%2029%%2C17%%22%%20stroke%%3D%%22black%%22%%20stroke-width%%3D%%222%%22%%20fill%%3D%%22none%%22%%20%%2F%%3E%%3Cline%%20x1%%3D%%2219%%22%%20y1%%3D%%2217%%22%%20x2%%3D%%2219%%22%%20y2%%3D%%2222%%22%%20stroke%%3D%%22black%%22%%20stroke-width%%3D%%222%%22%%20%%2F%%3E%%3Cline%%20x1%%3D%%2229%%22%%20y1%%3D%%2217%%22%%20x2%%3D%%2229%%22%%20y2%%3D%%2222%%22%%20stroke%%3D%%22black%%22%%20stroke-width%%3D%%222%%22%%20%%2F%%3E%%3Cpath%%20d%%3D%%22M%%2018%%2022%%20C%%2016.338%%2022%%2015%%2023.338%%2015%%2025%%20L%%2015%%2028%%20C%%2015%%2028.1126%%2015.021235%%2028.218814%%2015.033203%%2028.328125%%20C%%2015.012249%%2028.549649%%2015%%2028.772811%%2015%%2029%%20C%%2015%%2032.878%%2018.122%%2036%%2022%%2036%%20L%%2026%%2036%%20C%%2029.878%%2036%%2033%%2032.878%%2033%%2029%%20C%%2033%%2028.772811%%2032.987751%%2028.549649%%2032.966797%%2028.328125%%20C%%2032.978765%%2028.218814%%2033%%2028.1126%%2033%%2028%%20L%%2033%%2025%%20C%%2033%%2023.338%%2031.662%%2022%%2030%%2022%%20L%%2026%%2022%%20L%%2022%%2022%%20L%%2018%%2022%%20z%%20M%%2023.962891%%2025.464844%%20A%%202.50025%%202.50025%%200%%200%%201%%2026.5%%2028%%20L%%2026.5%%2029%%20A%%202.50025%%202.50025%%200%%201%%201%%2021.5%%2029%%20L%%2021.5%%2028%%20A%%202.50025%%202.50025%%200%%200%%201%%2023.962891%%2025.464844%%20z%%20%%22%%20%%2F%%3E%%3C%%2Fsvg%%3E);\n"
"}\n"
"\n";

static const gchar *css_svg_icon =
"div.%s {\n"
"    background-image: url(data:image/svg+xml;charset=UTF-8,"
"%%3Csvg%%20version%%3D%%221.1%%22%%20baseProfile%%3D%%22full%%22%%20width%%3D%%2248%%22%%20height%%3D%%2248%%22%%20xmlns%%3D%%22http%%3A%%2F%%2Fwww.w3.org%%2F2000%%2Fsvg%%22%%3E%%3Ccircle%%20cx%%3D%%2224%%22%%20cy%%3D%%2224%%22%%20r%%3D%%2222%%22%%20stroke%%3D%%22black%%22%%20stroke-width%%3D%%222%%22%%20fill%%3D%%22%%23%06X%%22%%20%%2F%%3E%%3C%%2Fsvg%%3E"
");\n"
"}\n"
"\n";

static const gchar *html_header2 =
"</style>\n"
"</head>\n"
"<body>\n"
"<table id=\"networks\">\n"
"<thead>"
"<tr>"
"<th></th>"
"<th>Address</th>"
"<th>Freq</th>"
"<th>M</th>"
"<th>Channel</th>"
"<th>SSID</th>"
"<th>Radio name</th>"
"<th>S↑</th>"
"<th>R</th>"
"<th>N</th>"
"<th>T</th>"
"<th>W</th>"
"<th>B</th>"
"<th>First seen</th>"
"<th>Last seen</th>"
"</tr>"
"</thead>\n"
"<tbody>";

static const gchar *html_footer =
"</tbody>\n"
"</table>\n"
"</body>\n"
"</html>\n";

static void export_write_css(FILE*, gchar*, guint);
static gboolean export_foreach(GtkTreeModel*, GtkTreePath*, GtkTreeIter*, gpointer);


gboolean
export_html(const gchar    *filename,
            const gchar    *name,
            mtscan_model_t *model)
{
    FILE *fp;
    gchar *string;
    gchar *date;
    gchar *title;
    GDateTime *now;

    if(!(fp = fopen(filename, "w")))
        return FALSE;

    now = g_date_time_new_now_local();

    date = g_date_time_format(now, "%Y-%m-%d %H:%M:%S");
    title = (name ? g_strdup_printf(APP_NAME " - %s", name) : g_strdup(APP_NAME));
    string = g_markup_printf_escaped(html_header, date, title);
    fwrite(string, sizeof(char), strlen(string), fp);
    g_free(date);
    g_free(title);
    g_free(string);
    g_date_time_unref(now);

    export_write_css(fp, "marginal", SIGNAL_ICON_MARGINAL);
    export_write_css(fp, "weak", SIGNAL_ICON_WEAK);
    export_write_css(fp, "medium", SIGNAL_ICON_MEDIUM);
    export_write_css(fp, "good", SIGNAL_ICON_GOOD);
    export_write_css(fp, "strong", SIGNAL_ICON_STRONG);
    export_write_css(fp, "perfect", SIGNAL_ICON_PERFECT);

    fwrite(html_header2, sizeof(char), strlen(html_header2), fp);
    gtk_tree_model_foreach(GTK_TREE_MODEL(model->store), export_foreach, fp);
    fwrite(html_footer, sizeof(char), strlen(html_footer), fp);
    fclose(fp);

    return TRUE;
}

static void
export_write_css(FILE  *fp,
                 gchar *name,
                 guint  color)
{
    gchar *string = g_strdup_printf(css_svg_icon, name, color);
    fwrite(string, sizeof(char), strlen(string), fp);
    g_free(string);
}

static gboolean
export_foreach(GtkTreeModel *store,
               GtkTreePath  *path,
               GtkTreeIter  *iter,
               gpointer      data)
{
    FILE *fp = (FILE*)data;
    network_t net;
    GDateTime *date;
    gchar *string, *firstseen, *lastseen;

    network_init(&net);
    gtk_tree_model_get(store, iter,
                       COL_ADDRESS, &net.address,
                       COL_FREQUENCY, &net.frequency,
                       COL_CHANNEL, &net.channel,
                       COL_MODE, &net.mode,
                       COL_SSID, &net.ssid,
                       COL_RADIONAME, &net.radioname,
                       COL_MAXRSSI, &net.rssi,
                       COL_PRIVACY, &net.flags.privacy,
                       COL_ROUTEROS, &net.flags.routeros,
                       COL_NSTREME, &net.flags.nstreme,
                       COL_TDMA, &net.flags.tdma,
                       COL_WDS, &net.flags.wds,
                       COL_BRIDGE, &net.flags.bridge,
                       COL_FIRSTSEEN, &net.firstseen,
                       COL_LASTSEEN, &net.lastseen,
                       -1);

    date = g_date_time_new_from_unix_local(net.firstseen);
    firstseen = g_date_time_format(date, "%Y-%m-%d %H:%M:%S");
    g_date_time_unref(date);

    date = g_date_time_new_from_unix_local(net.lastseen);
    lastseen = g_date_time_format(date, "%Y-%m-%d %H:%M:%S");
    g_date_time_unref(date);

    string = g_strdup_printf("<tr><td><div class=\"icon %s\">%s</div></td>",
                             ui_icon_string(net.rssi),
                             (net.flags.privacy ? "<div class=\"priv\"></div>" : ""));

    fwrite(string, sizeof(char), strlen(string), fp);
    g_free(string);

    string = g_markup_printf_escaped("<td class=\"mono\">%s</td>" /* COL_ADDRESS */
                                     "<td>%s</td>" /* COL_FREQ (formatted) */
                                     "<td>%s</td>" /* COL_MODE */
                                     "<td>%s</td>" /* COL_CHANNEL */
                                     "<td>%s</td>" /* COL_SSID */
                                     "<td>%s</td>" /* COL_RADIONAME */
                                     "<td>%d</td>" /* COL_MAXSIG */
                                     "<td>%s</td>" /* COL_ROUTEROS */
                                     "<td>%s</td>" /* COL_NSTREME */
                                     "<td>%s</td>" /* COL_TDMA */
                                     "<td>%s</td>" /* COL_WDS */
                                     "<td>%s</td>" /* COL_BRIDGE */
                                     "<td>%s</td>" /* COL_FIRSTSEEN */
                                     "<td>%s</td>" /* COL_LASTSEEN */
                                     "</tr>\n",
                                     net.address,
                                     model_format_frequency(net.frequency),
                                     net.mode,
                                     net.channel,
                                     net.ssid,
                                     net.radioname,
                                     net.rssi,
                                     (net.flags.routeros ? "R" : ""),
                                     (net.flags.nstreme ? "N" : ""),
                                     (net.flags.tdma ? "T" : ""),
                                     (net.flags.wds ? "W" : ""),
                                     (net.flags.bridge ? "B" : ""),
                                     firstseen,
                                     lastseen);

    g_free(firstseen);
    g_free(lastseen);

    fwrite(string, sizeof(char), strlen(string), fp);
    g_free(string);

    network_free(&net);
    return FALSE;
}
