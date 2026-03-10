#include "viewMode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static ViewMode current_mode = VIEW_MODE_LIST;
static char *config_path = NULL;

// Get config directory path
static const char* get_config_dir(void)

{
    static char *config_dir = NULL;
    
    if (config_dir == NULL) {
        const char *home = getenv("HOME");
        if (home) {
            config_dir = g_build_filename(home, ".config", "blackline", NULL);
            
            // Create directory if it doesn't exist
            if (access(config_dir, F_OK) != 0) {
                g_mkdir_with_parents(config_dir, 0755);
            }
        } else {
            config_dir = g_strdup("/tmp");
        }
    }
    
    return config_dir;
}

// Get config file path
static const char* get_config_path(void)

{
    if (config_path == NULL) {
        config_path = g_build_filename(get_config_dir(), "tools_view_mode.conf", NULL);
    }
    
    return config_path;
}

// Save current view mode to config file
void view_mode_save(void)

{
    const char *path = get_config_path();
    FILE *fp = fopen(path, "w");
    
    if (fp) {
        fprintf(fp, "%d\n", current_mode);
        fclose(fp);
    }
}

// Load view mode from config file
void view_mode_load(void)

{
    const char *path = get_config_path();
    FILE *fp = fopen(path, "r");
    
    if (fp) {
        int mode;
        if (fscanf(fp, "%d", &mode) == 1) {
            if (mode == VIEW_MODE_LIST || mode == VIEW_MODE_GRID) {
                current_mode = (ViewMode)mode;
            }
        }
        fclose(fp);
    }
    // If file doesn't exist, keep default (VIEW_MODE_LIST)
}

ViewMode view_mode_get_current(void)

{
    return current_mode;
}

static void populate_list_view(GtkWidget *container, const ToolItem *tools, int num_tools, gpointer window)

{
    for (int i = 0; i < num_tools; i++) {
        char *label_text = g_strdup_printf("%s %s", tools[i].icon, tools[i].label);
        GtkWidget *button = gtk_button_new_with_label(label_text);
        g_free(label_text);
        
        // Set consistent height for list view buttons
        gtk_widget_set_size_request(button, -1, 35);
        
        // Store window pointer in button for callback
        g_object_set_data(G_OBJECT(button), "window", window);
        
        g_signal_connect(button, "clicked", G_CALLBACK(tools[i].callback), window);
        gtk_box_pack_start(GTK_BOX(container), button, FALSE, FALSE, 2);
    }
}

static void populate_grid_view(GtkWidget *container, const ToolItem *tools, int num_tools, gpointer window)

{
    // Use a grid with 2 columns
    int cols = 2;
    
    for (int i = 0; i < num_tools; i++) {
        // Create a vertical box for each grid item to hold icon and label
        GtkWidget *grid_item = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        gtk_widget_set_size_request(grid_item, 120, 100);
        
        // Icon as a label with larger font
        char *icon_markup = g_strdup_printf("<span font='32' foreground='#00ff88'>%s</span>", tools[i].icon);
        GtkWidget *icon_label = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(icon_label), icon_markup);
        g_free(icon_markup);
        
        // Tool name
        GtkWidget *name_label = gtk_label_new(tools[i].label);
        
        // Pack icon and name
        gtk_box_pack_start(GTK_BOX(grid_item), icon_label, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(grid_item), name_label, FALSE, FALSE, 0);
        
        // Make the whole box clickable
        GtkWidget *event_box = gtk_event_box_new();
        gtk_container_add(GTK_CONTAINER(event_box), grid_item);
        
        // Store window pointer in event box for callback
        g_object_set_data(G_OBJECT(event_box), "window", window);
        
        // Connect click event
        g_signal_connect(event_box, "button-press-event", G_CALLBACK(tools[i].callback_event), window);
        
        int row = i / cols;
        int col = i % cols;
        gtk_grid_attach(GTK_GRID(container), event_box, col, row, 1, 1);
    }
    
    // Make grid homogeneous
    gtk_grid_set_column_homogeneous(GTK_GRID(container), TRUE);
    gtk_grid_set_row_homogeneous(GTK_GRID(container), TRUE);
}

GtkWidget* view_mode_create_container(const ToolItem *tools, int num_tools, ViewMode mode, gpointer window)

{
    GtkWidget *container;
    
    if (mode == VIEW_MODE_LIST) {
        container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
        populate_list_view(container, tools, num_tools, window);
    } else {
        container = gtk_grid_new();
        gtk_grid_set_row_spacing(GTK_GRID(container), 10);
        gtk_grid_set_column_spacing(GTK_GRID(container), 10);
        populate_grid_view(container, tools, num_tools, window);
    }
    
    current_mode = mode;
    gtk_widget_show_all(container);
    
    return container;
}

GtkWidget* view_mode_toggle(GtkWidget *old_container, const ToolItem *tools, int num_tools, GtkWidget *parent_box, gpointer window)

{
    ViewMode new_mode = (current_mode == VIEW_MODE_LIST) ? VIEW_MODE_GRID : VIEW_MODE_LIST;
    
    // Remove old container
    if (old_container) {
        gtk_widget_destroy(old_container);
    }
    
    // Create new container with new mode
    GtkWidget *new_container = view_mode_create_container(tools, num_tools, new_mode, window);
    
    // Add it back to the parent box
    gtk_box_pack_start(GTK_BOX(parent_box), new_container, TRUE, TRUE, 0);
    gtk_box_reorder_child(GTK_BOX(parent_box), new_container, 2); // After separator
    
    // Save the selection
    view_mode_save();
    
    return new_container;
}