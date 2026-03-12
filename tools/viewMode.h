#ifndef VIEW_MODE_H
#define VIEW_MODE_H

#include <gtk/gtk.h>

typedef enum {
    VIEW_MODE_LIST,
    VIEW_MODE_GRID
} ViewMode;

typedef struct {
    char *label;
    char *icon;
    void (*callback)(GtkButton *, gpointer);
    gboolean (*callback_event)(GtkWidget *, GdkEventButton *, gpointer);
    void *user_data; // not used
} ToolItem;

// Mode management
void view_mode_save(void);
void view_mode_load(void);
ViewMode view_mode_get_current(void);

// UI creation
GtkWidget* view_mode_create_container(const ToolItem *tools, int num_tools, ViewMode mode, gpointer window);
GtkWidget* view_mode_toggle(GtkWidget *old_container, const ToolItem *tools, int num_tools, GtkWidget *parent_box, gpointer window);

#endif