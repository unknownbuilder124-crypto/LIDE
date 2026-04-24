#ifndef LIDE_MOUSE_STYLE_H
#define LIDE_MOUSE_STYLE_H

#include <gtk/gtk.h>

/**
 * Creates a widget that allows the user to select a system‑wide mouse cursor style.
 *
 * @return GtkWidget containing the cursor style selector.
 */
GtkWidget *mouse_style_widget_new(void);

#endif /* LIDE_MOUSE_STYLE_H */