/*
 *  MTscan - MikroTik RouterOS wireless scanner
 *  Copyright (c) 2015-2019  Konrad Kosmatka
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
#include <glib.h>
#include <math.h>

gdouble
geoloc_utils_distance(gdouble lat1,
                      gdouble lon1,
                      gdouble lat2,
                      gdouble lon2)
{
    gdouble d_lat, d_lon, a, c;

    lat1 *= M_PI / 180.0;
    lon1 *= M_PI / 180.0;
    lat2 *= M_PI / 180.0;
    lon2 *= M_PI / 180.0;

    d_lat = (lat2 - lat1) / 2.0;
    d_lon = (lon2 - lon1) / 2.0;

    a = sin(d_lat) * sin(d_lat) + sin(d_lon) * sin(d_lon) * cos(lat1) * cos(lat2);
    c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));

    return 6371.0 * c;
}

gdouble
geoloc_utils_azimuth(gdouble lat1,
                     gdouble lon1,
                     gdouble lat2,
                     gdouble lon2)
{
    gdouble az, a, b, cos_lat2;

    lat1 *= M_PI / 180.0;
    lon1 *= M_PI / 180.0;
    lat2 *= M_PI / 180.0;
    lon2 *= M_PI / 180.0;

    cos_lat2 = cos(lat2);
    a = asin(lon2 - lon1) * cos_lat2;
    b = cos(lat1) * sin(lat2) - sin(lat1) * cos_lat2 * cos(lon2 - lon1);

    az = atan2(a, b) / (M_PI / 180.0);
    return (az < 0) ? az + 360.0 : az;
}

gboolean
geoloc_utils_azimuth_match(gdouble az1,
                           gdouble az2,
                           gdouble error)
{
    gdouble diff;

    diff = fabs(az1 - az2);
    if(diff > 180.0)
        diff = 360.0 - diff;

    return (diff <= error);
}
