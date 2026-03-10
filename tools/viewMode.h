#ifndef VIEW_MODE_H
#define VIEW_MODE_H

#include <gtk/gtk.h>

// View mode enumeration
typedef enum {
    VIEW_MODE_LIST,
    VIEW_MODE_GRID
} ViewMode;

// Tool item structure
typedef struct {
    const char *label;
    const char *icon;
    void (*callback)(GtkButton *, gpointer);
    gboolean (*callback_event)(GtkWidget *, GdkEventButton *, gpointer);
    gpointer callback_data;
} ToolItem;

// Function declarations
ViewMode view_mode_get_current(void);
GtkWidget* view_mode_toggle(GtkWidget *old_container, const ToolItem *tools, int num_tools, GtkWidget *parent_box, gpointer window);
GtkWidget* view_mode_create_container(const ToolItem *tools, int num_tools, ViewMode mode, gpointer window);
void view_mode_save(void);
void view_mode_load(void);

#endif