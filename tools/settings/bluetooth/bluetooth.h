#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include <gtk/gtk.h>

/*
 * Function: create_bluetooth_tab
 * -------------------------------
 * Creates and returns a GTK widget representing the Bluetooth settings tab.
 * 
 * Parameters: none
 * 
 * Returns: GtkWidget* - A vertical box container holding the complete
 *          Bluetooth user interface (scrolled window with command reference).
 * 
 * Side effects: Allocates GTK widgets, spawns a process to read blt.txt,
 *               updates UI with file contents.
 * 
 * Workflow:
 *   1. Creates main vertical container (GtkBox)
 *   2. Optionally adds a title label (commented out)
 *   3. Creates scrolled window for scrollable content area
 *   4. Creates text view widget to display command reference
 *   5. Reads blt.txt from current directory via popen()
 *   6. Inserts file content into text buffer (read-only)
 *   7. Packs all widgets into container
 */
GtkWidget* create_bluetooth_tab(void);

#endif