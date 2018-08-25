#ifndef MTSCAN_TZSP_RECEIVER_H_
#define MTSCAN_TZSP_RECEIVER_H_

typedef struct tzsp_receiver tzsp_receiver_t;

tzsp_receiver_t* tzsp_receiver_new(guint16,
                                   const gchar*,
                                   guint8[6],
                                   gint,
                                   gint,
                                   void (*)(tzsp_receiver_t*),
                                   void (*)(const tzsp_receiver_t*, network_t*));
void tzsp_receiver_free(tzsp_receiver_t*);
void tzsp_receiver_enable(tzsp_receiver_t*);
void tzsp_receiver_disable(tzsp_receiver_t*);
void tzsp_receiver_cancel(tzsp_receiver_t*);


#endif

