#ifndef MINIMIZED_CONTAINER_H
#define MINIMIZED_CONTAINER_H

#include <gtk/gtk.h>
#include <X11/Xlib.h>

void minimized_container_initialize(void);
void minimized_container_add_window(Window xid);
void minimized_container_remove_window(Window xid);
GtkWidget* minimized_container_get_toggle_button(void);

#endif