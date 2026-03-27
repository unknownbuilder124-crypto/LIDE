#ifndef LIDE_REFRESH_RATE_H
#define LIDE_REFRESH_RATE_H

#include <gtk/gtk.h>

/**
 * Creates the refresh rate selection widget.
 * Detects available refresh rates for the current display and provides a combo box for selection.
 * When a refresh rate is selected, it applies it immediately using xrandr.
 *
 * @return GtkWidget containing refresh rate settings UI
 */
GtkWidget *refresh_rate_widget_new(void);

#endif /* LIDE_REFRESH_RATE_H */