#ifndef LIDE_OUTPUT_H
#define LIDE_OUTPUT_H

#include <gtk/gtk.h>

GtkWidget *output_volume_widget_new(void);
GtkWidget *output_device_selector_widget_new(void);
GtkWidget *balance_widget_new(void);
void balance_cleanup(void);
void output_volume_refresh(void);

#endif /* LIDE_OUTPUT_H */