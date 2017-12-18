#include <string.h>
#include "export.h"
#include "model.h"
#include "mtscan.h"
#include "ui-icons.h"

typedef struct mtscan_export
{
    FILE *fp;
    gboolean icon;
    gboolean latlon;
    gboolean azimuth;
} mtscan_export_t;

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
"table.networks {\n"
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
"table.networks thead {\n"
"    background-color: #2255ff;\n"
"    text-align: center;\n"
"    font-weight: bold;\n"
"    color: white;\n"
"}\n"
"\n"
"table.networks td {\n"
"    border: 1px solid #444444;\n"
"    padding: 1px 3px 1px 3px;\n"
"}\n"
"\n"
"table.networks tbody tr:hover {\n"
"    background-color: #3a3a3a !important;\n"
"}\n"
"\n"
"table.networks a {\n"
"    text-decoration: none;\n"
"}\n"
"\n"
"table.networks td.mono {\n"
"    font-family: \"DejaVu Mono\", monospace;\n"
"}\n"
"\n"
"div.icon {\n"
"    height: 1.2em;\n"
"    width: 1.2em;\n"
"    background-size: 100%% 100%%;\n"
"}\n";

static const gchar *css_svg_priv = "\n"
"div.priv {\n"
"    height: 100%;\n"
"    width: 100%;\n"
"    background-size: 100% 100%;\n"
"    background-image: url(data:image/svg+xml;charset=UTF-8,%3Csvg%20version%3D%221.1%22%20baseProfile%3D%22full%22%20width%3D%2248%22%20height%3D%2248%22%20xmlns%3D%22http%3A%2F%2Fwww.w3.org%2F2000%2Fsvg%22%3E%3Cpath%20d%3D%22M19%2C17%20C19%2C12%2029%2C12%2029%2C17%22%20stroke%3D%22black%22%20stroke-width%3D%222%22%20fill%3D%22none%22%20%2F%3E%3Cline%20x1%3D%2219%22%20y1%3D%2217%22%20x2%3D%2219%22%20y2%3D%2222%22%20stroke%3D%22black%22%20stroke-width%3D%222%22%20%2F%3E%3Cline%20x1%3D%2229%22%20y1%3D%2217%22%20x2%3D%2229%22%20y2%3D%2222%22%20stroke%3D%22black%22%20stroke-width%3D%222%22%20%2F%3E%3Cpath%20d%3D%22M%2018%2022%20C%2016.338%2022%2015%2023.338%2015%2025%20L%2015%2028%20C%2015%2028.1126%2015.021235%2028.218814%2015.033203%2028.328125%20C%2015.012249%2028.549649%2015%2028.772811%2015%2029%20C%2015%2032.878%2018.122%2036%2022%2036%20L%2026%2036%20C%2029.878%2036%2033%2032.878%2033%2029%20C%2033%2028.772811%2032.987751%2028.549649%2032.966797%2028.328125%20C%2032.978765%2028.218814%2033%2028.1126%2033%2028%20L%2033%2025%20C%2033%2023.338%2031.662%2022%2030%2022%20L%2026%2022%20L%2022%2022%20L%2018%2022%20z%20M%2023.962891%2025.464844%20A%202.50025%202.50025%200%200%201%2026.5%2028%20L%2026.5%2029%20A%202.50025%202.50025%200%201%201%2021.5%2029%20L%2021.5%2028%20A%202.50025%202.50025%200%200%201%2023.962891%2025.464844%20z%20%22%20%2F%3E%3C%2Fsvg%3E);\n"
"}\n";

static const gchar *css_svg_icon = "\n"
"div.%s {\n"
"    background-image: url(data:image/svg+xml;charset=UTF-8,"
"%%3Csvg%%20version%%3D%%221.1%%22%%20baseProfile%%3D%%22full%%22%%20width%%3D%%2248%%22%%20height%%3D%%2248%%22%%20xmlns%%3D%%22http%%3A%%2F%%2Fwww.w3.org%%2F2000%%2Fsvg%%22%%3E%%3Ccircle%%20cx%%3D%%2224%%22%%20cy%%3D%%2224%%22%%20r%%3D%%2222%%22%%20stroke%%3D%%22black%%22%%20stroke-width%%3D%%222%%22%%20fill%%3D%%22%%23%06X%%22%%20%%2F%%3E%%3C%%2Fsvg%%3E"
");\n"
"}\n";

static const gchar *html_header2 =
"</style>\n"
"</head>\n"
"<body>\n";

static const gchar *html_footer =
"</body>\n"
"</html>\n";


static gboolean export_write(mtscan_export_t*, const gchar*);
static gboolean export_write_css(mtscan_export_t*, const gchar*, guint);
static gboolean export_foreach(GtkTreeModel*, GtkTreePath*, GtkTreeIter*, gpointer);


gboolean
export_html(const gchar    *filename,
            const gchar    *name,
            mtscan_model_t *model,
            gboolean        icon,
            gboolean        latlon,
            gboolean        azimuth)
{
    mtscan_export_t e;
    gchar *string;
    gchar *date;
    gchar *title;
    GDateTime *now;

    if(!(e.fp = fopen(filename, "w")))
        return FALSE;

    e.icon = icon;
    e.latlon = latlon;
    e.azimuth = azimuth;
    now = g_date_time_new_now_local();
    date = g_date_time_format(now, "%Y-%m-%d %H:%M:%S");

    title = (name ? g_strdup_printf(APP_NAME " - %s", name) : g_strdup(APP_NAME));
    string = g_markup_printf_escaped(html_header, date, title);
    export_write(&e, string);
    g_free(date);
    g_free(title);
    g_free(string);
    g_date_time_unref(now);

    if(e.icon)
    {
        export_write(&e, css_svg_priv);
        export_write_css(&e, "marginal", SIGNAL_ICON_MARGINAL);
        export_write_css(&e, "weak", SIGNAL_ICON_WEAK);
        export_write_css(&e, "medium", SIGNAL_ICON_MEDIUM);
        export_write_css(&e, "good", SIGNAL_ICON_GOOD);
        export_write_css(&e, "strong", SIGNAL_ICON_STRONG);
        export_write_css(&e, "perfect", SIGNAL_ICON_PERFECT);
    }

    export_write(&e, html_header2);
    export_write(&e, "<table class=\"networks\">\n");
    export_write(&e, "<thead><tr>");

    if(e.icon)
        export_write(&e, "<th>" MODEL_TEXT_ICON "</th>");

    export_write(&e, "<th>" MODEL_TEXT_ADDRESS "</th>");
    export_write(&e, "<th>" MODEL_TEXT_FREQUENCY "</th>");
    export_write(&e, "<th>" MODEL_TEXT_MODE "</th>");
    export_write(&e, "<th>" MODEL_TEXT_CHANNEL "</th>");
    export_write(&e, "<th>" MODEL_TEXT_SSID "</th>");
    export_write(&e, "<th>" MODEL_TEXT_RADIONAME "</th>");
    export_write(&e, "<th>" MODEL_TEXT_MAXRSSI "</th>");
    export_write(&e, "<th>" MODEL_TEXT_ROUTEROS "</th>");
    export_write(&e, "<th>" MODEL_TEXT_NSTREME "</th>");
    export_write(&e, "<th>" MODEL_TEXT_TDMA "</th>");
    export_write(&e, "<th>" MODEL_TEXT_WDS "</th>");
    export_write(&e, "<th>" MODEL_TEXT_BRIDGE "</th>");
    export_write(&e, "<th>" MODEL_TEXT_ROUTEROS_VER "</th>");
    export_write(&e, "<th>" MODEL_TEXT_FIRSTSEEN "</th>");
    export_write(&e, "<th>" MODEL_TEXT_LASTSEEN "</th>");

    if(e.latlon)
    {
        export_write(&e, "<th>" MODEL_TEXT_LATITUDE "</th>");
        export_write(&e, "<th>" MODEL_TEXT_LONGITUDE "</th>");
    }

    if(e.azimuth)
        export_write(&e, "<th>" MODEL_TEXT_AZIMUTH "</th>");

    export_write(&e, "</tr></thead>\n");
    export_write(&e, "<tbody>\n");
    gtk_tree_model_foreach(GTK_TREE_MODEL(model->store), export_foreach, &e);
    export_write(&e, "</tbody>\n");
    export_write(&e, "</table>\n");
    export_write(&e, html_footer);

    fclose(e.fp);
    return TRUE;
}

static gboolean
export_write(mtscan_export_t *e,
             const gchar     *str)
{
    gint length = strlen(str);
    return (fwrite(str, sizeof(char), length, e->fp) == length);
}

static gboolean
export_write_css(mtscan_export_t *e,
                 const gchar     *name,
                 guint            color)
{
    gchar *string = g_strdup_printf(css_svg_icon, name, color);
    gboolean ret = export_write(e, string);
    g_free(string);
    return ret;
}

static gboolean
export_foreach(GtkTreeModel *store,
               GtkTreePath  *path,
               GtkTreeIter  *iter,
               gpointer      data)
{
    mtscan_export_t *e = (mtscan_export_t*)data;
    network_t net;
    GDateTime *date;
    gchar *firstseen, *lastseen, *cstr;
    GString *str;

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
                       COL_ROUTEROS_VER, &net.routeros_ver,
                       COL_FIRSTSEEN, &net.firstseen,
                       COL_LASTSEEN, &net.lastseen,
                       COL_LATITUDE, &net.latitude,
                       COL_LONGITUDE, &net.longitude,
                       COL_AZIMUTH, &net.azimuth,
                       -1);

    date = g_date_time_new_from_unix_local(net.firstseen);
    firstseen = g_date_time_format(date, "%Y-%m-%d %H:%M:%S");
    g_date_time_unref(date);

    date = g_date_time_new_from_unix_local(net.lastseen);
    lastseen = g_date_time_format(date, "%Y-%m-%d %H:%M:%S");
    g_date_time_unref(date);

    str = g_string_new("<tr>");

    if(e->icon)
    {
        g_string_append_printf(str, "<td><div class=\"icon %s\">%s</div></td>",
                                    ui_icon_string(net.rssi),
                                    (net.flags.privacy ? "<div class=\"priv\"></div>" : ""));

    }

    cstr = g_markup_printf_escaped("<td class=\"mono\">%s</td>" /* COL_ADDRESS (formatted) */
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
                                   "<td>%s</td>" /* COL_ROUTEROS_VER */
                                   "<td>%s</td>" /* COL_FIRSTSEEN */
                                   "<td>%s</td>", /* COL_LASTSEEN */
                                   model_format_address(net.address, FALSE),
                                   model_format_frequency(net.frequency),
                                   net.mode,
                                   net.channel,
                                   net.ssid,
                                   net.radioname,
                                   net.rssi,
                                   (net.flags.routeros ? MODEL_TEXT_ROUTEROS : ""),
                                   (net.flags.nstreme ? MODEL_TEXT_NSTREME : ""),
                                   (net.flags.tdma ? MODEL_TEXT_TDMA : ""),
                                   (net.flags.wds ? MODEL_TEXT_WDS : ""),
                                   (net.flags.bridge ? MODEL_TEXT_BRIDGE : ""),
                                   net.routeros_ver,
                                   firstseen,
                                   lastseen);
    g_free(firstseen);
    g_free(lastseen);
    g_string_append_printf(str, cstr);
    g_free(cstr);

    if(e->latlon)
    {
        g_string_append_printf(str, "<td>%s</td>", model_format_gps(net.latitude, FALSE));
        g_string_append_printf(str, "<td>%s</td>", model_format_gps(net.longitude, FALSE));
    }

    if(e->azimuth)
        g_string_append_printf(str, "<td>%s</td>", model_format_azimuth(net.azimuth, TRUE));

    g_string_append_printf(str, "</tr>\n");

    cstr = g_string_free(str, FALSE);
    export_write(e, cstr);
    g_free(cstr);

    network_free(&net);
    return FALSE;
}
