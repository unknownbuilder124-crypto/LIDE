#ifndef VOIDFOX_APP_MENU_H
#define VOIDFOX_APP_MENU_H

#include <gtk/gtk.h>
#include "voidfox.h"

/**
 * app_menu.h
 * 
 * Application menu interface definitions.
 *
 * This module is part of the LIDE desktop environment system.
 * See the main window manager (wm/) and session management (session/)
 * for system architecture overview.
 */

/**
 * Creates the application menu with all menu items.
 * Provides access to browser features including:
 * - File operations (new window, private window)
 * - Navigation history (bookmarks, history, downloads)
 * - Security (passwords)
 * - Appearance (themes)
 * - Editing (print, find in page)
 * - View (zoom controls)
 * - Settings
 * - Help (report broken site)
 *
 * @param browser BrowserWindow instance for callback context.
 * @return GtkWidget containing the complete menu hierarchy.
 *         Caller assumes ownership of the returned widget.
 */
GtkWidget* create_application_menu(BrowserWindow *browser);

/**
 * Displays the application menu anchored to a button.
 * Positions the menu relative to the specified button widget.
 *
 * @param menu   The menu to display.
 * @param button The button widget to anchor the menu to.
 *               Menu will appear below the button.
 */
void show_app_menu(GtkWidget *menu, GtkWidget *button);

/**
 * Updates the downloads menu item to show active download count.
 * Shows "Downloads (N)" where N is the number of active downloads.
 * Called whenever download state changes to provide real-time feedback.
 */
void update_downloads_menu_badge(void);

#endif /* VOIDFOX_APP_MENU_H */