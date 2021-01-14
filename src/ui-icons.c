/*
 *  MTscan - MikroTik RouterOS wireless scanner
 *  Copyright (c) 2015-2017  Konrad Kosmatka
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */

#include <math.h>
#include <string.h>
#include "ui.h"
#include "ui-icons.h"

enum
{
    ICONS_OPEN = 0,
    ICONS_PRIVACY = 1,
    ICONS_COUNT
};

typedef struct ui_icons
{
    GdkPixbuf *none;
    GdkPixbuf *marginal;
    GdkPixbuf *weak;
    GdkPixbuf *medium;
    GdkPixbuf *good;
    GdkPixbuf *strong;
    GdkPixbuf *perfect;
} ui_icons_t;

static gint icon_size = 0;
static ui_icons_t icons[ICONS_COUNT] = {0};

static const gchar svg_header[] = "<svg version=\"1.1\" baseProfile=\"full\" width=\"48\" height=\"48\" xmlns=\"http://www.w3.org/2000/svg\">";
static const gchar svg_circle[] = "<circle cx=\"24\" cy=\"24\" r=\"22\" stroke=\"black\" stroke-width=\"2\" fill=\"#%06X\" />";
static const gchar svg_priv[] =
"<path d=\"M19,17 C19,12 29,12 29,17\" stroke=\"black\" stroke-width=\"2\" fill=\"none\" />"
"<line x1=\"19\" y1=\"17\" x2=\"19\" y2=\"22\" stroke=\"black\" stroke-width=\"2\" />"
"<line x1=\"29\" y1=\"17\" x2=\"29\" y2=\"22\" stroke=\"black\" stroke-width=\"2\" />"
"<path d=\"M 18 22 C 16.338 22 15 23.338 15 25 L 15 28 C 15 28.1126 15.021235 28.218814 15.033203 28.328125 C 15.012249 28.549649 15 28.772811 15 29 C 15 32.878 18.122 36 22 36 L 26 36 C 29.878 36 33 32.878 33 29 C 33 28.772811 32.987751 28.549649 32.966797 28.328125 C 32.978765 28.218814 33 28.1126 33 28 L 33 25 C 33 23.338 31.662 22 30 22 L 26 22 L 22 22 L 18 22 z M 23.962891 25.464844 A 2.50025 2.50025 0 0 1 26.5 28 L 26.5 29 A 2.50025 2.50025 0 1 1 21.5 29 L 21.5 28 A 2.50025 2.50025 0 0 1 23.962891 25.464844 z \" />";
static const gchar svg_footer[] = "</svg>";

static GdkPixbuf* ui_icon_draw(gint, guint, gboolean);
static void ui_icon_flush(void);

void
ui_icon_size(gint new_icon_size)
{
    if(icon_size != new_icon_size)
    {
        icon_size = new_icon_size;
        ui_icon_flush();
    }
}

GdkPixbuf*
ui_icon(gint rssi,
        gint type)
{
    if(rssi == MODEL_NO_SIGNAL)
    {
        if(!icons[type].none)
            icons[type].none = ui_icon_draw(icon_size, SIGNAL_ICON_NONE, (type == ICONS_PRIVACY));
        return icons[type].none;
    }

    if(rssi <= SIGNAL_LEVEL_MARGINAL_OVER)
    {
        if(!icons[type].marginal)
            icons[type].marginal = ui_icon_draw(icon_size, SIGNAL_ICON_MARGINAL, (type == ICONS_PRIVACY));
        return icons[type].marginal;
    }

    if(rssi <= SIGNAL_LEVEL_WEAK_OVER)
    {
        if(!icons[type].weak)
            icons[type].weak = ui_icon_draw(icon_size, SIGNAL_ICON_WEAK, (type == ICONS_PRIVACY));
        return icons[type].weak;
    }

    if(rssi <= SIGNAL_LEVEL_MEDIUM_OVER)
    {
        if(!icons[type].medium)
            icons[type].medium = ui_icon_draw(icon_size, SIGNAL_ICON_MEDIUM, (type == ICONS_PRIVACY));
        return icons[type].medium;
    }

    if(rssi <= SIGNAL_LEVEL_GOOD_OVER)
    {
        if(!icons[type].good)
            icons[type].good = ui_icon_draw(icon_size, SIGNAL_ICON_GOOD, (type == ICONS_PRIVACY));
        return icons[type].good;
    }

    if(rssi <= SIGNAL_LEVEL_STRONG_OVER)
    {
        if(!icons[type].strong)
            icons[type].strong = ui_icon_draw(icon_size, SIGNAL_ICON_STRONG, (type == ICONS_PRIVACY));
        return icons[type].strong;
    }

    if(!icons[type].perfect)
        icons[type].perfect = ui_icon_draw(icon_size, SIGNAL_ICON_PERFECT, (type == ICONS_PRIVACY));
    return icons[type].perfect;
}

static GdkPixbuf*
ui_icon_draw(gint     size,
             guint    color,
             gboolean privacy)
{
    GdkPixbufLoader *svg_loader;
    GdkPixbuf *pixbuf;
    GError *err = NULL;
    gchar *svg_circle_colored = g_strdup_printf(svg_circle, color);

    svg_loader = gdk_pixbuf_loader_new();
    gdk_pixbuf_loader_set_size(svg_loader, size, size);

    gdk_pixbuf_loader_write(svg_loader, (guchar*)svg_header, strlen(svg_header), &err);
    gdk_pixbuf_loader_write(svg_loader, (guchar*)svg_circle_colored, strlen(svg_circle_colored), &err);
    if(privacy)
        gdk_pixbuf_loader_write(svg_loader, (guchar*)svg_priv, strlen(svg_priv), &err);
    gdk_pixbuf_loader_write(svg_loader, (guchar*)svg_footer, strlen(svg_footer), &err);
    gdk_pixbuf_loader_close(svg_loader, &err);

    pixbuf = gdk_pixbuf_loader_get_pixbuf(svg_loader);
    g_object_ref(pixbuf);
    g_free(svg_circle_colored);
    g_object_unref(svg_loader);
    return pixbuf;
}

static void
ui_icon_flush(void)
{
    int i;
    for(i=ICONS_OPEN; i<ICONS_COUNT; i++)
    {
        if(icons[i].none)
            g_object_unref(icons[i].none);
        icons[i].none = NULL;

        if(icons[i].marginal)
            g_object_unref(icons[i].marginal);
        icons[i].marginal = NULL;

        if(icons[i].weak)
            g_object_unref(icons[i].weak);
        icons[i].weak = NULL;

        if(icons[i].medium)
            g_object_unref(icons[i].medium);
        icons[i].medium = NULL;

        if(icons[i].good)
            g_object_unref(icons[i].good);
        icons[i].good = NULL;

        if(icons[i].strong)
            g_object_unref(icons[i].strong);
        icons[i].strong = NULL;

        if(icons[i].perfect)
            g_object_unref(icons[i].perfect);
        icons[i].perfect = NULL;
    }
}

const gchar*
ui_icon_string(gint rssi)
{
    if(rssi == MODEL_NO_SIGNAL)
        return "none";
    if(rssi <= SIGNAL_LEVEL_MARGINAL_OVER)
        return "marginal";
    if(rssi <= SIGNAL_LEVEL_WEAK_OVER)
        return "weak";
    if(rssi <= SIGNAL_LEVEL_MEDIUM_OVER)
        return "medium";
    if(rssi <= SIGNAL_LEVEL_GOOD_OVER)
        return "good";
    if(rssi <= SIGNAL_LEVEL_STRONG_OVER)
        return "strong";

    return "perfect";
}

#if GTK_CHECK_VERSION (3, 0, 0)
gboolean
ui_icon_draw_heartbeat(GtkWidget *widget,
                       cairo_t   *cr,
                       gpointer   data)
{
#else
#define gtk_widget_get_allocated_height(widget) ((widget)->allocation.height)
#define gtk_widget_get_allocated_width(widget) ((widget)->allocation.width)
gboolean
ui_icon_draw_heartbeat(GtkWidget      *widget,
                       GdkEventExpose *event,
                       gpointer        data)
{
    cairo_t *cr;
#endif
    gint *status = (gint*)data;
    gint height = gtk_widget_get_allocated_height(widget);
    gint width = height * 2;
    gdouble x, y, r;

    if(gtk_widget_get_allocated_width(widget) != width)
    {
        /* Always keep the 2:1 ratio */
        gtk_widget_set_size_request(widget, width, -1);
        return TRUE;
    }

#if !GTK_CHECK_VERSION (3, 0, 0)
    cr = gdk_cairo_create(gtk_widget_get_window(widget));
#endif
    x = width / 2.0;
    y = height / 2.0;
    r = height * 0.15;

    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_arc(cr, x, y, r, 0, 2*M_PI);
    cairo_fill(cr);

    if(*status == MTSCAN_MODE_SCANNER)
        cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
    else if(*status == MTSCAN_MODE_SNIFFER)
        cairo_set_source_rgb(cr, 0.0, 0.0, 1.0);
    else
        cairo_set_source_rgb(cr, 0.64, 0.64, 0.64);

    cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
    cairo_set_line_width(cr, width*0.1);

    r = width * 0.24;
    cairo_arc(cr, x, y, r, -M_PI/6.0, M_PI/6.0);
    cairo_stroke(cr);

    cairo_arc(cr, x, y, r, 5*M_PI/6.0, 7*M_PI/6.0);
    cairo_stroke(cr);

    r = width * 0.44;
    cairo_arc(cr, x, y, r, -M_PI/6.0, M_PI/6.0);
    cairo_stroke(cr);

    cairo_arc(cr, x, y, r, 5*M_PI/6.0, 7*M_PI/6.0);
    cairo_stroke(cr);
#if !GTK_CHECK_VERSION (3, 0, 0)
    cairo_destroy(cr);
#endif
    return FALSE;
}
