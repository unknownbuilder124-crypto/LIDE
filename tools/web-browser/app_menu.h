#ifndef VOIDFOX_APP_MENU_H
#define VOIDFOX_APP_MENU_H

#include <gtk/gtk.h>
#include "voidfox.h"

// Function prototypes
GtkWidget* create_application_menu(BrowserWindow *browser);
void show_app_menu(GtkWidget *menu, GtkWidget *button);

#endif