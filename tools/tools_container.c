#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <gdk/gdkx.h>
#include "viewMode.h"

// Drag variables - commented out to disable dragging
// static int is_dragging = 0;
// static int drag_start_x, drag_start_y;

// Launch functions 
static void launch_file_manager(GtkButton *button, gpointer window) 

{
    (void)button;
    
    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        execl("./blackline-fm", "blackline-fm", NULL);
        exit(0);
    } else if (pid > 0) {
        // Parent process - close tools container
        gtk_window_close(GTK_WINDOW(window));
    }
}

static gboolean launch_file_manager_event(GtkWidget *widget, GdkEventButton *event, gpointer window)

{
    (void)widget;
    (void)event;
    
    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        execl("./blackline-fm", "blackline-fm", NULL);
        exit(0);
    } else if (pid > 0) {
        // Parent process - close tools container
        gtk_window_close(GTK_WINDOW(window));
    }
    return TRUE;
}

static void launch_text_editor(GtkButton *button, gpointer window) 

{
    (void)button;
    
    pid_t pid = fork();
    if (pid == 0) {
        // Launch custom text editor
        execl("./blackline-editor", "blackline-editor", NULL);
        exit(0);
    } else if (pid > 0) {
        // Parent process - close tools container
        gtk_window_close(GTK_WINDOW(window));
    }
}

static gboolean launch_text_editor_event(GtkWidget *widget, GdkEventButton *event, gpointer window)

{
    (void)widget;
    (void)event;
    
    pid_t pid = fork();
    if (pid == 0) {
        // Launch custom text editor
        execl("./blackline-editor", "blackline-editor", NULL);
        exit(0);
    } else if (pid > 0) {
        // Parent process - close tools container
        gtk_window_close(GTK_WINDOW(window));
    }
    return TRUE;
}

static void launch_calculator(GtkButton *button, gpointer window) 

{
    (void)button;
    
    pid_t pid = fork();
    if (pid == 0) {
        // Launch custom calculator 
        execl("./blackline-calculator", "blackline-calculator", NULL);
        exit(0);
    } else if (pid > 0) {
        // Parent process - close tools container
        gtk_window_close(GTK_WINDOW(window));
    }
}

static gboolean launch_calculator_event(GtkWidget *widget, GdkEventButton *event, gpointer window)

{
    (void)widget;
    (void)event;
    
    pid_t pid = fork();
    if (pid == 0) {
        // Launch custom calculator 
        execl("./blackline-calculator", "blackline-calculator", NULL);
        exit(0);
    } else if (pid > 0) {
        // Parent process - close tools container
        gtk_window_close(GTK_WINDOW(window));
    }
    return TRUE;
}

static void launch_system_monitor(GtkButton *button, gpointer window) 

{
    (void)button;
    
    pid_t pid = fork();
    if (pid == 0) {
        // Launch custom system monitor
        execl("./blackline-system-monitor", "blackline-system-monitor", NULL);
        exit(0);
    } else if (pid > 0) {
        // Parent process - close tools container
        gtk_window_close(GTK_WINDOW(window));
    }
}

static gboolean launch_system_monitor_event(GtkWidget *widget, GdkEventButton *event, gpointer window)

{
    (void)widget;
    (void)event;
    
    pid_t pid = fork();
    if (pid == 0) {
        // Launch custom system monitor
        execl("./blackline-system-monitor", "blackline-system-monitor", NULL);
        exit(0);
    } else if (pid > 0) {
        // Parent process - close tools container
        gtk_window_close(GTK_WINDOW(window));
    }
    return TRUE;
}

// Web browser launcher function
static void launch_web_browser(GtkButton *button, gpointer window) 

{
    (void)button;
    
    pid_t pid = fork();
    if (pid == 0) {
        // Launch VoidFox web browser
        execl("./voidfox", "voidfox", NULL);
        exit(0);
    } else if (pid > 0) {
        // Parent process - close tools container
        gtk_window_close(GTK_WINDOW(window));
    }
}

static gboolean launch_web_browser_event(GtkWidget *widget, GdkEventButton *event, gpointer window)

{
    (void)widget;
    (void)event;
    
    pid_t pid = fork();
    if (pid == 0) {
        // Launch VoidFox web browser
        execl("./voidfox", "voidfox", NULL);
        exit(0);
    } else if (pid > 0) {
        // Parent process - close tools container
        gtk_window_close(GTK_WINDOW(window));
    }
    return TRUE;
}

// Firefox wrapper launcher function
static void launch_firefox_wrapper(GtkButton *button, gpointer window) 

{
    (void)button;
    
    // Check if Firefox wrapper exists
    if (access("./tools/firefox/firefox-wrapper", X_OK) != 0 && 
        access("./firefox-wrapper", X_OK) != 0) {
        
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window),
                                                  GTK_DIALOG_MODAL,
                                                  GTK_MESSAGE_ERROR,
                                                  GTK_BUTTONS_OK,
                                                  "Firefox wrapper not found!\n\n"
                                                  "Please build it first with:\n"
                                                  "make firefox-wrapper");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    
    pid_t pid = fork();
    if (pid == 0) {
        // Child process - try different paths
        if (access("./tools/firefox/firefox-wrapper", X_OK) == 0) {
            execl("./tools/firefox/firefox-wrapper", "firefox-wrapper", NULL);
        } else {
            execl("./firefox-wrapper", "firefox-wrapper", NULL);
        }
        exit(0);
    } else if (pid > 0) {
        // Parent process - close tools container
        gtk_window_close(GTK_WINDOW(window));
    }
}

static gboolean launch_firefox_wrapper_event(GtkWidget *widget, GdkEventButton *event, gpointer window)

{
    (void)widget;
    (void)event;
    
    // Check if Firefox wrapper exists
    if (access("./tools/firefox/firefox-wrapper", X_OK) != 0 && 
        access("./firefox-wrapper", X_OK) != 0) {
        
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window),
                                                  GTK_DIALOG_MODAL,
                                                  GTK_MESSAGE_ERROR,
                                                  GTK_BUTTONS_OK,
                                                  "Firefox wrapper not found!\n\n"
                                                  "Please build it first with:\n"
                                                  "make firefox-wrapper");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return TRUE;
    }
    
    pid_t pid = fork();
    if (pid == 0) {
        // Child process - try different paths
        if (access("./tools/firefox/firefox-wrapper", X_OK) == 0) {
            execl("./tools/firefox/firefox-wrapper", "firefox-wrapper", NULL);
        } else {
            execl("./firefox-wrapper", "firefox-wrapper", NULL);
        }
        exit(0);
    } else if (pid > 0) {
        // Parent process - close tools container
        gtk_window_close(GTK_WINDOW(window));
    }
    return TRUE;
}

// Tool definitions for view mode
static ToolItem tools[] = {
    {"File Manager", "📁", launch_file_manager, launch_file_manager_event, NULL},
    {"Text Editor", "📝", launch_text_editor, launch_text_editor_event, NULL},
    {"Calculator", "🧮", launch_calculator, launch_calculator_event, NULL},
    {"System Monitor", "📊", launch_system_monitor, launch_system_monitor_event, NULL},
    {"VoidFox", "🌐", launch_web_browser, launch_web_browser_event, NULL},
    {"Firefox", "🦊", launch_firefox_wrapper, launch_firefox_wrapper_event, NULL}
};
static int num_tools = sizeof(tools) / sizeof(tools[0]);

// Dragging functions - commented out to disable
/*
static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer window) 
{
    if (event->button == 1) 
    {
        is_dragging = 1;
        drag_start_x = event->x_root;
        drag_start_y = event->y_root;
        gtk_window_present(GTK_WINDOW(window));
        return TRUE;
    }
    return FALSE;
}

static gboolean on_button_release(GtkWidget *widget, GdkEventButton *event, gpointer window) 
{
    if (event->button == 1) {
        is_dragging = 0;
        return TRUE;
    }
    return FALSE;
}

static gboolean on_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer window) 
{
    if (is_dragging) 
    {
        int dx = event->x_root - drag_start_x;
        int dy = event->y_root - drag_start_y;
        
        int x, y;
        gtk_window_get_position(GTK_WINDOW(window), &x, &y);
        gtk_window_move(GTK_WINDOW(window), x + dx, y + dy);
        
        drag_start_x = event->x_root;
        drag_start_y = event->y_root;
        return TRUE;
    }
    return FALSE;
}
*/

static void on_close_clicked(GtkButton *button, gpointer window) 

{
    gtk_window_close(GTK_WINDOW(window));
}

// View toggle callback
static void on_view_toggle_clicked(GtkButton *button, gpointer user_data) 

{
    // Retrieve stored data
    GtkWidget *scrolled_window = g_object_get_data(G_OBJECT(button), "scrolled_window");
    GtkWidget *old_container = g_object_get_data(G_OBJECT(button), "container");
    GtkWidget *window = g_object_get_data(G_OBJECT(button), "window");
    
    if (!scrolled_window || !old_container || !window) {
        g_warning("Missing data in view toggle callback");
        return;
    }
    
    // Get current mode and toggle
    ViewMode current = view_mode_get_current();
    ViewMode new_mode = (current == VIEW_MODE_LIST) ? VIEW_MODE_GRID : VIEW_MODE_LIST;
    
    // Remove old container from scrolled window
    gtk_container_remove(GTK_CONTAINER(scrolled_window), old_container);
    
    // Create new container with new mode
    GtkWidget *new_container = view_mode_create_container(tools, num_tools, new_mode, window);
    
    // Add new container to scrolled window
    gtk_container_add(GTK_CONTAINER(scrolled_window), new_container);
    
    // Update stored container
    g_object_set_data(G_OBJECT(button), "container", new_container);
    
    // Update button label and window size - same height for both modes
    if (new_mode == VIEW_MODE_LIST) {
        gtk_button_set_label(button, "📋 List");
        gtk_window_set_default_size(GTK_WINDOW(window), 300, 400);
    } else {
        gtk_button_set_label(button, "🔲 Grid");
        gtk_window_set_default_size(GTK_WINDOW(window), 350, 400);
    }
    
    // Show all widgets
    gtk_widget_show_all(window);
}

static void activate(GtkApplication *app, gpointer user_data) 

{
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "BlackLine Tools");
    
    // Load saved view mode
    view_mode_load();
    ViewMode saved_mode = view_mode_get_current();
    
    // Set window size based on mode - same height for both
    if (saved_mode == VIEW_MODE_LIST) {
        gtk_window_set_default_size(GTK_WINDOW(window), 300, 400);
    } else {
        gtk_window_set_default_size(GTK_WINDOW(window), 350, 400);
    }
    
    // Set fixed position 
    // These coordinates are relative to the X display
    gtk_window_move(GTK_WINDOW(window), 10, 40);  // Fixed position (x=10, y=40) - below the panel
    
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_NONE);  // Don't auto-center
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);
    gtk_window_set_decorated(GTK_WINDOW(window), FALSE); 
    
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    
    // Title bar with view toggle and close
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<b>Tools</b>");
    gtk_box_pack_start(GTK_BOX(hbox), title, TRUE, TRUE, 0);
    
    // View toggle button
    GtkWidget *view_btn = gtk_button_new_with_label("");
    gtk_widget_set_size_request(view_btn, 60, 25);
    gtk_box_pack_end(GTK_BOX(hbox), view_btn, FALSE, FALSE, 0);
    
    GtkWidget *close_btn = gtk_button_new_with_label("X");
    gtk_widget_set_size_request(close_btn, 30, 25);
    g_signal_connect(close_btn, "clicked", G_CALLBACK(on_close_clicked), window);
    gtk_box_pack_end(GTK_BOX(hbox), close_btn, FALSE, FALSE, 0);
    
    // Separator
    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), sep, FALSE, FALSE, 5);
    
    // Set button label based on saved mode
    if (saved_mode == VIEW_MODE_LIST) {
        gtk_button_set_label(GTK_BUTTON(view_btn), "📋 List");
    } else {
        gtk_button_set_label(GTK_BUTTON(view_btn), "🔲 Grid");
    }
    
    // Create a scrolled window to contain the tools container (for BOTH list and grid views)
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                   GTK_POLICY_NEVER,     // Horizontal scrollbar never
                                   GTK_POLICY_AUTOMATIC); // Vertical scrollbar when needed
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scrolled_window), 300);
    
    // Create container with saved mode
    GtkWidget *tools_container = view_mode_create_container(tools, num_tools, saved_mode, window);
    
    // Add the tools container to the scrolled window
    gtk_container_add(GTK_CONTAINER(scrolled_window), tools_container);
    
    // Pack the scrolled window into the main vbox
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);
    
    // Store data on view button for later use
    g_object_set_data(G_OBJECT(view_btn), "scrolled_window", scrolled_window);
    g_object_set_data(G_OBJECT(view_btn), "container", tools_container);
    g_object_set_data(G_OBJECT(view_btn), "window", window);
    
    // Connect view toggle button
    g_signal_connect(view_btn, "clicked", G_CALLBACK(on_view_toggle_clicked), NULL);
    
    // Add some spacing at the bottom
    GtkWidget *bottom_spacer = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(vbox), bottom_spacer, TRUE, TRUE, 0);
    
    // CSS
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "window { background-color: #0b0f14; color: #ffffff; border: 1px solid #00ff88; }"
        "button { background-color: #1e2429; color: #00ff88; border: none; }"
        "button:hover { background-color: #2a323a; }"
        "label { color: #ffffff; }"
        "entry { background-color: #1e2429; color: #ffffff; border: 1px solid #00ff88; }"
        "scrolledwindow { border: none; background-color: #0b0f14; }"
        /* Style the scrollbar */
        "scrollbar { background-color: #1e2429; }"
        "scrollbar slider { background-color: #00ff88; border-radius: 4px; min-width: 8px; min-height: 8px; }"
        "scrollbar slider:hover { background-color: #33ffaa; }"
        "scrollbar slider:active { background-color: #00cc66; }"
        "scrollbar trough { background-color: #2a323a; border-radius: 4px; }",
        -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    
    // CRITICAL: Show all widgets
    gtk_widget_show_all(window);
    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv) 

{
    GtkApplication *app = gtk_application_new("org.blackline.tools", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}