#ifndef MTSCAN_GPS_H_
#define MTSCAN_GPS_H_

enum
{
    GPS_OFF,
    GPS_OPENING,
    GPS_WAITING_FOR_DATA,
    GPS_INVALID,
    GPS_OK
};

typedef struct mtscan_gps
{
    gchar *host;
    gchar *port;
    volatile gboolean connected;
    volatile gboolean canceled;
} mtscan_gps_t;

typedef struct mtscan_gps_data
{
    gboolean fix;
    gdouble lat;
    gdouble lon;
    gdouble hdop;
    gint sat;
    gint64 timestamp;
    mtscan_gps_t *source;
} mtscan_gps_data_t;

void gps_start(const gchar*, gint);
void gps_stop(void);
gint gps_get_data(const mtscan_gps_data_t**);

#endif
