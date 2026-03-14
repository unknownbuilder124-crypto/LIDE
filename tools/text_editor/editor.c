#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <gdk/gdkx.h>
#include <dirent.h>
#include <sys/stat.h>
#include <libgen.h>
#include "edit.h"
#include "text_editor.h"
#include "window_resize.h"

// Text buffer
static GtkTextBuffer *buffer = NULL;
static char *current_filename = NULL;
static char *current_folder = NULL;

// Dialog mode enum
typedef enum {
    DIALOG_MODE_OPEN,
    DIALOG_MODE_SAVE
} DialogMode;

// Function prototypes
static void new_file(GtkButton *button, gpointer data);
static void open_file(GtkButton *button, gpointer data);
static void save_file(GtkButton *button, gpointer data);
static void save_file_as(GtkButton *button, gpointer data);
static void on_listbox_row_activated(GtkListBox *listbox, GtkListBoxRow *row, gpointer dialog);
static void on_open_clicked(GtkButton *button, gpointer dialog);
static void on_save_clicked(GtkButton *button, gpointer dialog);
static void on_up_clicked(GtkButton *button, gpointer dialog);
static void on_home_clicked(GtkButton *button, gpointer dialog);
static void on_dialog_response(GtkDialog *dialog, gint response_id, gpointer user_data);
static void custom_file_dialog(GtkWidget *parent, DialogMode mode);

// Dragging handlers
static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data)

{
    Editor *ed = (Editor *)data;

    if (event->button == 1)
    {
        // Check if cursor is on an edge (for resizing)
        ed->resize_edge = detect_resize_edge_absolute(GTK_WINDOW(ed->window), event->x_root, event->y_root);

        if (ed->resize_edge != RESIZE_NONE) {
            ed->is_resizing = 1;
        } else {
            ed->is_dragging = 1;
        }

        ed->drag_start_x = event->x_root;
        ed->drag_start_y = event->y_root;
        gtk_window_present(GTK_WINDOW(ed->window));
        return TRUE;
    }
    return FALSE;
}

static gboolean on_button_release(GtkWidget *widget, GdkEventButton *event, gpointer data)

{
    Editor *ed = (Editor *)data;

    if (event->button == 1) {
        ed->is_dragging = 0;
        ed->is_resizing = 0;
        ed->resize_edge = RESIZE_NONE;
        return TRUE;
    }
    return FALSE;
}

static gboolean on_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer data)

{
    Editor *ed = (Editor *)data;

    // Update cursor for resize hints
    if (!ed->is_dragging && !ed->is_resizing) {
        int resize_edge = detect_resize_edge_absolute(GTK_WINDOW(ed->window), event->x_root, event->y_root);
        update_resize_cursor(widget, resize_edge);
    }

    if (ed->is_resizing) {
        int delta_x = event->x_root - ed->drag_start_x;
        int delta_y = event->y_root - ed->drag_start_y;

        int window_width, window_height;
        gtk_window_get_size(GTK_WINDOW(ed->window), &window_width, &window_height);

        apply_window_resize(GTK_WINDOW(ed->window), ed->resize_edge,
                           delta_x, delta_y, window_width, window_height);

        ed->drag_start_x = event->x_root;
        ed->drag_start_y = event->y_root;
        return TRUE;
    } else if (ed->is_dragging) {
        int dx = event->x_root - ed->drag_start_x;
        int dy = event->y_root - ed->drag_start_y;

        int x, y;
        gtk_window_get_position(GTK_WINDOW(ed->window), &x, &y);
        gtk_window_move(GTK_WINDOW(ed->window), x + dx, y + dy);

        ed->drag_start_x = event->x_root;
        ed->drag_start_y = event->y_root;
        return TRUE;
    }
    return FALSE;
}

// Window control callbacks - FIXED maximize function
static void on_minimize_clicked(GtkButton *button, gpointer window)
{
    (void)button;
    gtk_window_iconify(GTK_WINDOW(window));
}

static void on_maximize_clicked(GtkButton *button, gpointer window)
{
    (void)button;
    GtkWindow *win = GTK_WINDOW(window);
    
    if (gtk_window_is_maximized(win)) {
        gtk_window_unmaximize(win);
    } else {
        gtk_window_maximize(win);
    }
}

// Track window state changes to update maximize button
static gboolean on_window_state_changed(GtkWidget *window, GdkEventWindowState *event, gpointer data)
{
    GtkButton *max_btn = GTK_BUTTON(data);
    
    if (event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED) {
        gtk_button_set_label(max_btn, "❐"); // Restore symbol when maximized
        gtk_widget_set_tooltip_text(GTK_WIDGET(max_btn), "Restore");
    } else {
        gtk_button_set_label(max_btn, "□"); // Maximize symbol when normal
        gtk_widget_set_tooltip_text(GTK_WIDGET(max_btn), "Maximize");
    }
    
    return FALSE;
}

static void on_close_clicked(GtkButton *button, gpointer window)
{
    (void)button;
    gtk_window_close(GTK_WINDOW(window));
}

// File operations
static void new_file(GtkButton *button, gpointer data)

{
    (void)button;
    GtkWidget *window = GTK_WIDGET(data);
    gtk_text_buffer_set_text(buffer, "", -1);
    if (current_filename) {
        g_free(current_filename);
        current_filename = NULL;
    }
    gtk_window_set_title(GTK_WINDOW(window), "BlackLine Editor - Untitled");
}

// Directory navigation functions
static void on_up_clicked(GtkButton *button, gpointer dialog)

{
    (void)button;
    char *parent = g_path_get_dirname(current_folder);
    if (g_strcmp0(parent, current_folder) != 0) {
        g_free(current_folder);
        current_folder = parent;
        
        // Get mode from dialog
        DialogMode mode = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(dialog), "dialog-mode"));
        
        // Get the parent window
        GtkWindow *parent_win = gtk_window_get_transient_for(GTK_WINDOW(dialog));
        GtkWidget *parent_widget = GTK_WIDGET(parent_win);
        
        // Destroy old dialog
        gtk_widget_destroy(GTK_WIDGET(dialog));
        
        // Create new dialog with same mode
        custom_file_dialog(parent_widget, mode);
    } else {
        g_free(parent);
    }
}

static void on_home_clicked(GtkButton *button, gpointer dialog)

{
    (void)button;
    const gchar *home = g_get_home_dir();
    g_free(current_folder);
    current_folder = g_strdup(home);
    
    // Get mode from dialog
    DialogMode mode = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(dialog), "dialog-mode"));
    
    // Get the parent window
    GtkWindow *parent_win = gtk_window_get_transient_for(GTK_WINDOW(dialog));
    GtkWidget *parent_widget = GTK_WIDGET(parent_win);
    
    // Destroy old dialog
    gtk_widget_destroy(GTK_WIDGET(dialog));
    
    // Create new dialog with same mode
    custom_file_dialog(parent_widget, mode);
}

static void on_listbox_row_activated(GtkListBox *listbox, GtkListBoxRow *row, gpointer dialog)

{
    (void)listbox;
    GtkWidget *row_widget = GTK_WIDGET(row);
    GtkWidget *label = gtk_bin_get_child(GTK_BIN(row_widget));
    const char *filename = gtk_label_get_text(GTK_LABEL(label));
    
    // Skip [DIR] prefix if present
    if (strncmp(filename, "[DIR] ", 6) == 0) {
        filename = filename + 6;
    }
    
    gpointer is_dir_ptr = g_object_get_data(G_OBJECT(row_widget), "is_dir");
    int is_dir = GPOINTER_TO_INT(is_dir_ptr);
    
    // Get mode from dialog
    DialogMode mode = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(dialog), "dialog-mode"));
    
    if (is_dir) {
        // Enter directory
        char *new_path = g_build_filename(current_folder, filename, NULL);
        g_free(current_folder);
        current_folder = new_path;
        
        // Get the parent window
        GtkWindow *parent_win = gtk_window_get_transient_for(GTK_WINDOW(dialog));
        GtkWidget *parent_widget = GTK_WIDGET(parent_win);
        
        // Destroy old dialog
        gtk_widget_destroy(GTK_WIDGET(dialog));
        
        // Create new dialog with same mode
        custom_file_dialog(parent_widget, mode);
    } else {
        // Select file - set entry text
        GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
        GList *children = gtk_container_get_children(GTK_CONTAINER(content));
        for (GList *c = children; c; c = c->next) {
            if (GTK_IS_BOX(c->data)) {
                GList *box_children = gtk_container_get_children(GTK_CONTAINER(c->data));
                for (GList *bc = box_children; bc; bc = bc->next) {
                    if (GTK_IS_BOX(bc->data)) {
                        GList *name_box_children = gtk_container_get_children(GTK_CONTAINER(bc->data));
                        for (GList *nc = name_box_children; nc; nc = nc->next) {
                            if (GTK_IS_ENTRY(nc->data)) {
                                gtk_entry_set_text(GTK_ENTRY(nc->data), filename);
                                
                                // eneo janina
                                if (mode == DIALOG_MODE_OPEN) {
                                    //JANINA KI KORmu
                                }
                            }
                        }
                        g_list_free(name_box_children);
                    }
                }
                g_list_free(box_children);
            }
        }
        g_list_free(children);
    }
}

static void on_open_clicked(GtkButton *button, gpointer dialog)

{
    (void)button;
    
    // Get filename from entry
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    char *filename = NULL;
    
    GList *children = gtk_container_get_children(GTK_CONTAINER(content));
    for (GList *c = children; c; c = c->next) {
        if (GTK_IS_BOX(c->data)) {
            GList *box_children = gtk_container_get_children(GTK_CONTAINER(c->data));
            for (GList *bc = box_children; bc; bc = bc->next) {
                if (GTK_IS_BOX(bc->data)) {
                    GList *name_box_children = gtk_container_get_children(GTK_CONTAINER(bc->data));
                    for (GList *nc = name_box_children; nc; nc = nc->next) {
                        if (GTK_IS_ENTRY(nc->data)) {
                            filename = g_strdup(gtk_entry_get_text(GTK_ENTRY(nc->data)));
                        }
                    }
                    g_list_free(name_box_children);
                }
            }
            g_list_free(box_children);
        }
    }
    g_list_free(children);
    
    if (filename && strlen(filename) > 0) {
        char *fullpath = g_build_filename(current_folder, filename, NULL);
        
        // Read file
        FILE *f = fopen(fullpath, "r");
        if (f) {
            fseek(f, 0, SEEK_END);
            long size = ftell(f);
            fseek(f, 0, SEEK_SET);
            
            char *content = g_malloc(size + 1);
            size_t read_size = fread(content, 1, size, f);
            if (read_size == size) {
                content[size] = '\0';
                gtk_text_buffer_set_text(buffer, content, -1);
            }
            fclose(f);
            g_free(content);
            
            // Update current filename
            if (current_filename) g_free(current_filename);
            current_filename = fullpath;
            
            // Update window title
            GtkWindow *parent_win = gtk_window_get_transient_for(GTK_WINDOW(dialog));
            if (parent_win) {
                GtkWidget *window = GTK_WIDGET(parent_win);
                char *title = g_strdup_printf("BlackLine Editor - %s", filename);
                gtk_window_set_title(GTK_WINDOW(window), title);
                g_free(title);
            }
        } else {
            g_free(fullpath);
            g_free(filename);
            filename = NULL;
        }
    }
    
    gtk_widget_destroy(GTK_WIDGET(dialog));
}

static void on_save_clicked(GtkButton *button, gpointer dialog)

{
    (void)button;
    
    // Get filename from entry
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    char *filename = NULL;
    
    GList *children = gtk_container_get_children(GTK_CONTAINER(content));
    for (GList *c = children; c; c = c->next) {
        if (GTK_IS_BOX(c->data)) {
            GList *box_children = gtk_container_get_children(GTK_CONTAINER(c->data));
            for (GList *bc = box_children; bc; bc = bc->next) {
                if (GTK_IS_BOX(bc->data)) {
                    GList *name_box_children = gtk_container_get_children(GTK_CONTAINER(bc->data));
                    for (GList *nc = name_box_children; nc; nc = nc->next) {
                        if (GTK_IS_ENTRY(nc->data)) {
                            filename = g_strdup(gtk_entry_get_text(GTK_ENTRY(nc->data)));
                        }
                    }
                    g_list_free(name_box_children);
                }
            }
            g_list_free(box_children);
        }
    }
    g_list_free(children);
    
    if (filename && strlen(filename) > 0) {
        char *fullpath = g_build_filename(current_folder, filename, NULL);
        
        // Get text from buffer
        GtkTextIter start, end;
        gtk_text_buffer_get_start_iter(buffer, &start);
        gtk_text_buffer_get_end_iter(buffer, &end);
        char *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
        
        // Write file
        FILE *f = fopen(fullpath, "w");
        if (f) {
            fprintf(f, "%s", text);
            fclose(f);
            
            // Update current filename
            if (current_filename) g_free(current_filename);
            current_filename = fullpath;
            
            // Update window title
            GtkWindow *parent_win = gtk_window_get_transient_for(GTK_WINDOW(dialog));
            if (parent_win) {
                GtkWidget *window = GTK_WIDGET(parent_win);
                char *title = g_strdup_printf("BlackLine Editor - %s", filename);
                gtk_window_set_title(GTK_WINDOW(window), title);
                g_free(title);
            }
        } else {
            g_free(fullpath);
        }
        g_free(text);
        g_free(filename);
    }
    
    gtk_widget_destroy(GTK_WIDGET(dialog));
}

static void on_dialog_response(GtkDialog *dialog, gint response_id, gpointer user_data)

{
    (void)user_data;
    if (response_id == GTK_RESPONSE_DELETE_EVENT) {
        gtk_widget_destroy(GTK_WIDGET(dialog));
    }
}

// Unified file dialog function
static void custom_file_dialog(GtkWidget *parent, DialogMode mode)

{
    GtkWidget *dialog;
    GtkWidget *content;
    GtkWidget *vbox;
    GtkWidget *scrolled;
    GtkWidget *listbox;
    GtkWidget *button_box;
    GtkWidget *cancel_btn;
    GtkWidget *action_btn;
    GtkWidget *entry;
    GtkWidget *path_box;
    GtkWidget *up_btn;
    GtkWidget *home_btn;
    GtkWidget *path_label;
    
    const gchar *home = g_get_home_dir();
    if (!current_folder) {
        current_folder = g_strdup(home);
    }
    
    // Create dialog with appropriate title
    dialog = gtk_dialog_new();
    if (mode == DIALOG_MODE_OPEN) {
        gtk_window_set_title(GTK_WINDOW(dialog), "Open File");
    } else {
        gtk_window_set_title(GTK_WINDOW(dialog), "Save File");
    }
    
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(parent));
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 450, 400);
    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);
    
    // Store mode in dialog for later use
    g_object_set_data(G_OBJECT(dialog), "dialog-mode", GINT_TO_POINTER(mode));
    
    content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_box_pack_start(GTK_BOX(content), vbox, TRUE, TRUE, 0);
    
    // Path bar
    path_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_pack_start(GTK_BOX(vbox), path_box, FALSE, FALSE, 0);
    
    up_btn = gtk_button_new_with_label("↑");
    gtk_widget_set_size_request(up_btn, 30, 25);
    g_signal_connect(up_btn, "clicked", G_CALLBACK(on_up_clicked), dialog);
    gtk_box_pack_start(GTK_BOX(path_box), up_btn, FALSE, FALSE, 0);
    
    home_btn = gtk_button_new_with_label("🏠");
    gtk_widget_set_size_request(home_btn, 30, 25);
    g_signal_connect(home_btn, "clicked", G_CALLBACK(on_home_clicked), dialog);
    gtk_box_pack_start(GTK_BOX(path_box), home_btn, FALSE, FALSE, 0);
    
    path_label = gtk_label_new(current_folder);
    gtk_label_set_ellipsize(GTK_LABEL(path_label), PANGO_ELLIPSIZE_START);
    gtk_box_pack_start(GTK_BOX(path_box), path_label, TRUE, TRUE, 5);
    
    // File list
    scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scrolled, -1, 250);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);
    
    listbox = gtk_list_box_new();
    gtk_container_add(GTK_CONTAINER(scrolled), listbox);
    
    // Populate file list
    DIR *dir = opendir(current_folder);
    if (dir) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] == '.') continue; // Skip hidden files
            
            // Get file/directory info
            char *fullpath = g_build_filename(current_folder, entry->d_name, NULL);
            struct stat st;
            if (stat(fullpath, &st) == 0) {
                GtkWidget *row = gtk_list_box_row_new();
                
                char *display_name;
                if (S_ISDIR(st.st_mode)) {
                    display_name = g_strdup_printf("[DIR] %s", entry->d_name);
                    g_object_set_data(G_OBJECT(row), "is_dir", GINT_TO_POINTER(1));
                } else {
                    display_name = g_strdup(entry->d_name);
                    g_object_set_data(G_OBJECT(row), "is_dir", GINT_TO_POINTER(0));
                }
                
                GtkWidget *label = gtk_label_new(display_name);
                gtk_label_set_xalign(GTK_LABEL(label), 0.0);
                gtk_container_add(GTK_CONTAINER(row), label);
                g_free(display_name);
                
                gtk_list_box_insert(GTK_LIST_BOX(listbox), row, -1);
            }
            g_free(fullpath);
        }
        closedir(dir);
    }
    
    // File name entry
    GtkWidget *name_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), name_box, FALSE, FALSE, 0);
    
    GtkWidget *name_label = gtk_label_new("Name:");
    gtk_box_pack_start(GTK_BOX(name_box), name_label, FALSE, FALSE, 0);
    
    entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(name_box), entry, TRUE, TRUE, 0);
    
    // Set default filename for save dialog
    if (mode == DIALOG_MODE_SAVE && current_filename) {
        char *basename = g_path_get_basename(current_filename);
        gtk_entry_set_text(GTK_ENTRY(entry), basename);
        g_free(basename);
    } else if (mode == DIALOG_MODE_SAVE) {
        gtk_entry_set_text(GTK_ENTRY(entry), "untitled.txt");
    }
    
    // Buttons
    button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), button_box, FALSE, FALSE, 0);
    
    cancel_btn = gtk_button_new_with_label("Cancel");
    g_signal_connect_swapped(cancel_btn, "clicked", G_CALLBACK(gtk_widget_destroy), dialog);
    gtk_box_pack_start(GTK_BOX(button_box), cancel_btn, TRUE, TRUE, 0);
    
    if (mode == DIALOG_MODE_OPEN) {
        action_btn = gtk_button_new_with_label("Open");
        g_signal_connect(action_btn, "clicked", G_CALLBACK(on_open_clicked), dialog);
    } else {
        action_btn = gtk_button_new_with_label("Save");
        g_signal_connect(action_btn, "clicked", G_CALLBACK(on_save_clicked), dialog);
    }
    gtk_box_pack_start(GTK_BOX(button_box), action_btn, TRUE, TRUE, 0);
    
    // Handle row activation
    g_signal_connect(listbox, "row-activated", G_CALLBACK(on_listbox_row_activated), dialog);
    
    // Handle dialog close
    g_signal_connect(dialog, "response", G_CALLBACK(on_dialog_response), NULL);
    
    gtk_widget_show_all(dialog);
}

static void open_file(GtkButton *button, gpointer data)

{
    (void)button;
    GtkWidget *window = GTK_WIDGET(data);
    custom_file_dialog(window, DIALOG_MODE_OPEN);
}

static void save_file(GtkButton *button, gpointer data)

{
    (void)button;
    GtkWidget *window = GTK_WIDGET(data);
    
    if (current_filename) {
        GtkTextIter start, end;
        gtk_text_buffer_get_start_iter(buffer, &start);
        gtk_text_buffer_get_end_iter(buffer, &end);
        char *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
        
        FILE *f = fopen(current_filename, "w");
        if (f) {
            fprintf(f, "%s", text);
            fclose(f);
        }
        g_free(text);
    } else {
        custom_file_dialog(window, DIALOG_MODE_SAVE);
    }
}

static void save_file_as(GtkButton *button, gpointer data)

{
    (void)button;
    GtkWidget *window = GTK_WIDGET(data);
    custom_file_dialog(window, DIALOG_MODE_SAVE);
}

static void activate(GtkApplication *app, gpointer user_data)

{
    (void)user_data;

    // Create Editor struct
    Editor *ed = g_new(Editor, 1);
    ed->current_file = NULL;
    ed->modified = FALSE;
    ed->find_dialog = NULL;
    ed->find_entry = NULL;
    ed->replace_entry = NULL;
    ed->is_dragging = 0;
    ed->is_resizing = 0;
    ed->resize_edge = RESIZE_NONE;

    GtkWidget *window;
    GtkWidget *vbox;
    GtkWidget *toolbar;
    GtkWidget *scrolled;
    GtkWidget *text_view;
    GtkWidget *edit_menu_btn;
    GtkWidget *edit_menu;
    GtkWidget *menu_item;
    GtkWidget *max_btn;  // Store maximize button for state tracking

    window = gtk_application_window_new(app);
    ed->window = window;
    gtk_window_set_title(GTK_WINDOW(window), "BlackLine Editor - Untitled");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

    // Enable events for dragging
    gtk_widget_add_events(window, GDK_BUTTON_PRESS_MASK |
                                   GDK_BUTTON_RELEASE_MASK |
                                   GDK_POINTER_MOTION_MASK);
    g_signal_connect(window, "button-press-event", G_CALLBACK(on_button_press), ed);
    g_signal_connect(window, "button-release-event", G_CALLBACK(on_button_release), ed);
    g_signal_connect(window, "motion-notify-event", G_CALLBACK(on_motion_notify), ed);

    // Main vertical box
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // Custom title bar with controls
    GtkWidget *title_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_name(title_bar, "title-bar");
    gtk_widget_set_size_request(title_bar, -1, 30);
    gtk_box_pack_start(GTK_BOX(vbox), title_bar, FALSE, FALSE, 0);

    GtkWidget *title_label = gtk_label_new("BlackLine Editor");
    gtk_label_set_xalign(GTK_LABEL(title_label), 0.0);
    gtk_box_pack_start(GTK_BOX(title_bar), title_label, TRUE, TRUE, 10);

    GtkWidget *window_buttons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_pack_end(GTK_BOX(title_bar), window_buttons, FALSE, FALSE, 5);

    // Minimize button
    GtkWidget *min_btn = gtk_button_new_with_label("─");
    gtk_widget_set_size_request(min_btn, 30, 25);
    g_signal_connect(min_btn, "clicked", G_CALLBACK(on_minimize_clicked), window);
    gtk_box_pack_start(GTK_BOX(window_buttons), min_btn, FALSE, FALSE, 0);

    // Maximize button - store reference for state tracking
    max_btn = gtk_button_new_with_label("□");
    gtk_widget_set_size_request(max_btn, 30, 25);
    g_signal_connect(max_btn, "clicked", G_CALLBACK(on_maximize_clicked), window);
    // Connect window state changes to update button appearance
    g_signal_connect(window, "window-state-event", G_CALLBACK(on_window_state_changed), max_btn);
    gtk_box_pack_start(GTK_BOX(window_buttons), max_btn, FALSE, FALSE, 0);

    // Close button
    GtkWidget *close_btn = gtk_button_new_with_label("✕");
    gtk_widget_set_size_request(close_btn, 30, 25);
    g_signal_connect(close_btn, "clicked", G_CALLBACK(on_close_clicked), window);
    gtk_box_pack_start(GTK_BOX(window_buttons), close_btn, FALSE, FALSE, 0);

    // Separator
    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), sep, FALSE, FALSE, 0);

    // Toolbar
    toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 5);

    // File operations buttons
    GtkWidget *new_btn = gtk_button_new_with_label("New");
    g_signal_connect(new_btn, "clicked", G_CALLBACK(new_file), window);
    gtk_box_pack_start(GTK_BOX(toolbar), new_btn, FALSE, FALSE, 0);

    GtkWidget *open_btn = gtk_button_new_with_label("Open");
    g_signal_connect(open_btn, "clicked", G_CALLBACK(open_file), window);
    gtk_box_pack_start(GTK_BOX(toolbar), open_btn, FALSE, FALSE, 0);

    GtkWidget *save_btn = gtk_button_new_with_label("Save");
    g_signal_connect(save_btn, "clicked", G_CALLBACK(save_file), window);
    gtk_box_pack_start(GTK_BOX(toolbar), save_btn, FALSE, FALSE, 0);

    GtkWidget *save_as_btn = gtk_button_new_with_label("Save As");
    g_signal_connect(save_as_btn, "clicked", G_CALLBACK(save_file_as), window);
    gtk_box_pack_start(GTK_BOX(toolbar), save_as_btn, FALSE, FALSE, 0);

    // Separator
    GtkWidget *toolbar_sep = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    gtk_box_pack_start(GTK_BOX(toolbar), toolbar_sep, FALSE, FALSE, 5);

    // Scrolled window for text view
    scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 5);

    // Text view
    text_view = gtk_text_view_new();
    ed->text_view = text_view;
    ed->buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    buffer = ed->buffer;
    gtk_container_add(GTK_CONTAINER(scrolled), text_view);

    // Initialize edit features
    edit_init(buffer);

    // Edit Menu Button
    edit_menu_btn = gtk_menu_button_new();
    gtk_button_set_label(GTK_BUTTON(edit_menu_btn), "Edit");
    gtk_box_pack_start(GTK_BOX(toolbar), edit_menu_btn, FALSE, FALSE, 0);

    // Create edit menu
    edit_menu = gtk_menu_new();

    // Undo/Redo section
    menu_item = gtk_menu_item_new_with_label("Undo");
    g_signal_connect(menu_item, "activate", G_CALLBACK(edit_undo), text_view);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), menu_item);

    menu_item = gtk_menu_item_new_with_label("Redo");
    g_signal_connect(menu_item, "activate", G_CALLBACK(edit_redo), text_view);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), menu_item);

    menu_item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), menu_item);

    // Cut/Copy/Paste section
    menu_item = gtk_menu_item_new_with_label("Cut");
    g_signal_connect(menu_item, "activate", G_CALLBACK(edit_cut), text_view);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), menu_item);

    menu_item = gtk_menu_item_new_with_label("Copy");
    g_signal_connect(menu_item, "activate", G_CALLBACK(edit_copy), text_view);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), menu_item);

    menu_item = gtk_menu_item_new_with_label("Paste");
    g_signal_connect(menu_item, "activate", G_CALLBACK(edit_paste), text_view);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), menu_item);

    menu_item = gtk_menu_item_new_with_label("Delete");
    g_signal_connect(menu_item, "activate", G_CALLBACK(edit_delete), text_view);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), menu_item);

    menu_item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), menu_item);

    // Select All
    menu_item = gtk_menu_item_new_with_label("Select All");
    g_signal_connect(menu_item, "activate", G_CALLBACK(edit_select_all), text_view);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), menu_item);

    menu_item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), menu_item);

    // Find/Replace/Goto section
    menu_item = gtk_menu_item_new_with_label("Find...");
    g_signal_connect(menu_item, "activate", G_CALLBACK(edit_find), text_view);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), menu_item);

    menu_item = gtk_menu_item_new_with_label("Replace...");
    g_signal_connect(menu_item, "activate", G_CALLBACK(edit_replace), text_view);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), menu_item);

    menu_item = gtk_menu_item_new_with_label("Go to Line...");
    g_signal_connect(menu_item, "activate", G_CALLBACK(edit_goto_line), text_view);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), menu_item);

    menu_item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), menu_item);

    // Text Transformation section
    menu_item = gtk_menu_item_new_with_label("To Uppercase");
    g_signal_connect(menu_item, "activate", G_CALLBACK(edit_to_uppercase), text_view);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), menu_item);

    menu_item = gtk_menu_item_new_with_label("To Lowercase");
    g_signal_connect(menu_item, "activate", G_CALLBACK(edit_to_lowercase), text_view);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), menu_item);

    menu_item = gtk_menu_item_new_with_label("Capitalize Words");
    g_signal_connect(menu_item, "activate", G_CALLBACK(edit_capitalize), text_view);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), menu_item);

    menu_item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), menu_item);

    // Line Operations section
    menu_item = gtk_menu_item_new_with_label("Toggle Comment");
    g_signal_connect(menu_item, "activate", G_CALLBACK(edit_toggle_comment), text_view);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), menu_item);

    menu_item = gtk_menu_item_new_with_label("Indent");
    g_signal_connect(menu_item, "activate", G_CALLBACK(edit_indent), text_view);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), menu_item);

    menu_item = gtk_menu_item_new_with_label("Unindent");
    g_signal_connect(menu_item, "activate", G_CALLBACK(edit_unindent), text_view);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), menu_item);

    menu_item = gtk_menu_item_new_with_label("Duplicate Line");
    g_signal_connect(menu_item, "activate", G_CALLBACK(edit_duplicate_line), text_view);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), menu_item);

    menu_item = gtk_menu_item_new_with_label("Delete Line");
    g_signal_connect(menu_item, "activate", G_CALLBACK(edit_delete_line), text_view);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), menu_item);

    menu_item = gtk_menu_item_new_with_label("Join Lines");
    g_signal_connect(menu_item, "activate", G_CALLBACK(edit_join_lines), text_view);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), menu_item);

    menu_item = gtk_menu_item_new_with_label("Sort Lines");
    g_signal_connect(menu_item, "activate", G_CALLBACK(edit_sort_lines), text_view);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), menu_item);

    menu_item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), menu_item);

    // Print section
    menu_item = gtk_menu_item_new_with_label("Print...");
    g_signal_connect(menu_item, "activate", G_CALLBACK(edit_print), text_view);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), menu_item);

    gtk_widget_show_all(edit_menu);
    gtk_menu_button_set_popup(GTK_MENU_BUTTON(edit_menu_btn), GTK_WIDGET(edit_menu));

    // Print button to toolbar for quick access
    GtkWidget *print_btn = gtk_button_new_with_label("Print");
    g_signal_connect(print_btn, "clicked", G_CALLBACK(edit_print), text_view);
    gtk_box_pack_start(GTK_BOX(toolbar), print_btn, FALSE, FALSE, 0);

    // CSS
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "window { background-color: #000000; color: #ffffff; }\n"
        "textview { background-color: #1a1a1a; color: #ffffff; }\n"
        "textview text { background-color: #1a1a1a; color: #ffffff; }\n"
        "button { background-color: #1a1a1a; color: #ff3333; border: 1px solid #ff3333; }\n"
        "button:hover { background-color: #333333; }\n"
        "#title-bar { background-color: #000000; border-bottom: 2px solid #ff3333; }\n"
        "dialog { background-color: #1a1a1a; color: #ffffff; }\n"
        "list { background-color: #1a1a1a; color: #ffffff; }\n"
        "listboxrow { background-color: #1a1a1a; color: #ffffff; }\n"
        "listboxrow:hover { background-color: #333333; }\n"
        "entry { background-color: #000000; color: #ffffff; border: 1px solid #ff3333; }\n"
        "label { color: #ffffff; }\n"
        "menu { background-color: #1a1a1a; color: #ffffff; }\n"
        "menuitem { background-color: #1a1a1a; color: #ffffff; }\n"
        "menuitem:hover { background-color: #333333; }\n",
        -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);

    gtk_window_set_decorated(GTK_WINDOW(window), TRUE);
    gtk_window_set_resizable(GTK_WINDOW(window), TRUE);
    gtk_widget_show_all(window);
    g_object_set_data_full(G_OBJECT(window), "editor", ed, g_free);
}

int main(int argc, char **argv)

{
    GtkApplication *app = gtk_application_new("org.blackline.editor", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}