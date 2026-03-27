#ifndef LIDE_AUTO_H
#define LIDE_AUTO_H

#include <gtk/gtk.h>

/**
 * Structure representing a tool item in the tools container.
 *
 * Contains all necessary information to display and launch an application.
 */
typedef struct {
    char *name;                                      ///< Display name of the tool
    char *icon;                                      ///< Icon (emoji or text) for the tool
    void (*button_callback)(GtkButton*, gpointer);   ///< Callback for button click
    gboolean (*event_callback)(GtkWidget*, GdkEventButton*, gpointer); ///< Callback for button events
    gpointer user_data;                              ///< User data passed to callbacks
} ToolItem;

/**
 * Structure representing a detected application from .desktop files.
 */
typedef struct {
    char *name;          ///< Application name
    char *icon;          ///< Icon name or path
    char *exec;          ///< Executable command
} DetectedApp;

/**
 * Scans system directories for installed applications.
 *
 * @param count Output parameter for number of detected apps.
 * @return Array of DetectedApp structures. Caller must free with free_detected_apps().
 */
DetectedApp* detect_installed_apps(int *count);

/**
 * Frees memory allocated for detected applications.
 *
 * @param apps Array of DetectedApp structures.
 * @param count Number of apps in the array.
 */
void free_detected_apps(DetectedApp *apps, int count);

/**
 * Creates a ToolItem from a detected application.
 *
 * @param app Detected application data.
 * @param window Parent window reference.
 * @return ToolItem structure with callbacks set.
 */
ToolItem create_tool_item_from_detected(DetectedApp *app, gpointer window);

/**
 * Merges static tools with detected applications into a single array.
 *
 * @param static_tools Static tool array.
 * @param static_count Number of static tools.
 * @param detected_apps Detected apps array.
 * @param detected_count Number of detected apps.
 * @param window Parent window reference.
 * @param merged_count Output parameter for merged array size.
 * @return Dynamically allocated merged ToolItem array.
 */
ToolItem* merge_tool_items(ToolItem *static_tools, int static_count,
                           DetectedApp *detected_apps, int detected_count,
                           gpointer window, int *merged_count);

/**
 * Sets the global tools window reference for closing from child processes.
 *
 * @param window The main tools window.
 */
void set_tools_window(GtkWidget *window);

#endif /* LIDE_AUTO_H */