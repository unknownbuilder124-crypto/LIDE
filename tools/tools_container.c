#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include "viewMode.h"
#include "animation.h" 

// Drag variables - commented out to disable dragging
// static int is_dragging = 0;
// static int drag_start_x, drag_start_y;

// Global display connection for cleanup
static Display *global_display = NULL;
static gboolean is_minimized = FALSE;  // Added for minimize state

// Forward declarations for animation callbacks
static void on_minimize_clicked(GtkButton *button, gpointer window);
static gboolean on_window_state_changed(GtkWidget *window, GdkEventWindowState *event, gpointer data);
static gboolean remove_old_container(gpointer user_data);
static gboolean reset_button_opacity(gpointer button);

// Data structure for view toggle
typedef struct {
    GtkWidget *scrolled_window;
    GtkWidget *container;      // current container
    GtkWidget *old_container;   // container being replaced
    GtkWidget *window;
    GtkWidget *view_button;
} ViewToggleData;

// Launch functions with smooth animations
static void launch_file_manager(GtkButton *button, gpointer window) 

{
    (void)button;
    
    // Animate button press
    GtkWidget *btn = GTK_WIDGET(button);
    gtk_widget_set_opacity(btn, 0.5);
    
    g_usleep(100000); // 100ms for visual feedback
    
    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        execl("./blackline-fm", "blackline-fm", NULL);
        exit(0);
    } else if (pid > 0) {
        // Parent process - animate window close
        animate_fade_out_with_scale(GTK_WIDGET(window), 200);
        g_timeout_add(200, (GSourceFunc)gtk_window_close, window);
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
        // Parent process - animate window close
        animate_fade_out_with_scale(GTK_WIDGET(window), 200);
        g_timeout_add(200, (GSourceFunc)gtk_window_close, window);
    }
    return TRUE;
}

static void launch_text_editor(GtkButton *button, gpointer window) 

{
    (void)button;
    
    // Animate button press
    GtkWidget *btn = GTK_WIDGET(button);
    gtk_widget_set_opacity(btn, 0.5);
    
    g_usleep(100000); // 100ms
    
    pid_t pid = fork();
    if (pid == 0) {
        // Launch custom text editor
        execl("./blackline-editor", "blackline-editor", NULL);
        exit(0);
    } else if (pid > 0) {
        // Parent process - animate window close
        animate_fade_out_with_scale(GTK_WIDGET(window), 200);
        g_timeout_add(200, (GSourceFunc)gtk_window_close, window);
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
        // Parent process - animate window close
        animate_fade_out_with_scale(GTK_WIDGET(window), 200);
        g_timeout_add(200, (GSourceFunc)gtk_window_close, window);
    }
    return TRUE;
}

static void launch_calculator(GtkButton *button, gpointer window) 

{
    (void)button;
    
    // Animate button press
    GtkWidget *btn = GTK_WIDGET(button);
    gtk_widget_set_opacity(btn, 0.5);
    
    g_usleep(100000); // 100ms
    
    pid_t pid = fork();
    if (pid == 0) {
        // Launch custom calculator 
        execl("./blackline-calculator", "blackline-calculator", NULL);
        exit(0);
    } else if (pid > 0) {
        // Parent process - animate window close
        animate_fade_out_with_scale(GTK_WIDGET(window), 200);
        g_timeout_add(200, (GSourceFunc)gtk_window_close, window);
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
        // Parent process - animate window close
        animate_fade_out_with_scale(GTK_WIDGET(window), 200);
        g_timeout_add(200, (GSourceFunc)gtk_window_close, window);
    }
    return TRUE;
}

static void launch_system_monitor(GtkButton *button, gpointer window) 

{
    (void)button;
    
    // Animate button press
    GtkWidget *btn = GTK_WIDGET(button);
    gtk_widget_set_opacity(btn, 0.5);
    
    g_usleep(100000); // 100ms
    
    pid_t pid = fork();
    if (pid == 0) {
        // Launch custom system monitor
        execl("./blackline-system-monitor", "blackline-system-monitor", NULL);
        exit(0);
    } else if (pid > 0) {
        // Parent process - animate window close
        animate_fade_out_with_scale(GTK_WIDGET(window), 200);
        g_timeout_add(200, (GSourceFunc)gtk_window_close, window);
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
        // Parent process - animate window close
        animate_fade_out_with_scale(GTK_WIDGET(window), 200);
        g_timeout_add(200, (GSourceFunc)gtk_window_close, window);
    }
    return TRUE;
}

// Web browser launcher function
static void launch_web_browser(GtkButton *button, gpointer window) 

{
    (void)button;
    
    // Animate button press
    GtkWidget *btn = GTK_WIDGET(button);
    gtk_widget_set_opacity(btn, 0.5);
    
    g_usleep(100000); // 100ms
    
    pid_t pid = fork();
    if (pid == 0) {
        // Launch VoidFox web browser
        execl("./voidfox", "voidfox", NULL);
        exit(0);
    } else if (pid > 0) {
        // animate window close
        animate_fade_out_with_scale(GTK_WIDGET(window), 200);
        g_timeout_add(200, (GSourceFunc)gtk_window_close, window);
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
        // Parent process - animate window close
        animate_fade_out_with_scale(GTK_WIDGET(window), 200);
        g_timeout_add(200, (GSourceFunc)gtk_window_close, window);
    }
    return TRUE;
}

// Firefox wrapper launcher function
static void launch_firefox_wrapper(GtkButton *button, gpointer window) 

{
    (void)button;
    
    // Animate button press
    GtkWidget *btn = GTK_WIDGET(button);
    gtk_widget_set_opacity(btn, 0.5);
    
    g_usleep(100000); // 100ms
    
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
        
        // Reset button opacity
        gtk_widget_set_opacity(btn, 1.0);
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
        // Parent process - animate window close
        animate_fade_out_with_scale(GTK_WIDGET(window), 200);
        g_timeout_add(200, (GSourceFunc)gtk_window_close, window);
    }
}

static gboolean launch_firefox_wrapper_event(GtkWidget *widget, GdkEventButton *event, gpointer window)

{
    (void)widget;
    (void)event;
    
    // Animate button press
    gtk_widget_set_opacity(widget, 0.5);
    
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
        
        // Reset button opacity
        gtk_widget_set_opacity(widget, 1.0);
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
        // Parent process - animate window close
        animate_fade_out_with_scale(GTK_WIDGET(window), 200);
        g_timeout_add(200, (GSourceFunc)gtk_window_close, window);
    }
    return TRUE;
}

// Terminal launcher function
static void launch_terminal(GtkButton *button, gpointer window) 

{
    (void)button;
    
    // Animate button press
    GtkWidget *btn = GTK_WIDGET(button);
    gtk_widget_set_opacity(btn, 0.5);
    
    g_usleep(100000); // 100ms
    
    pid_t pid = fork();
    if (pid == 0) {
        // Launch terminal
        execl("./blackline-terminal", "blackline-terminal", NULL);
        exit(0);
    } else if (pid > 0) {
        // Parent process - animate window close
        animate_fade_out_with_scale(GTK_WIDGET(window), 200);
        g_timeout_add(200, (GSourceFunc)gtk_window_close, window);
    }
}

static gboolean launch_terminal_event(GtkWidget *widget, GdkEventButton *event, gpointer window)

{
    (void)widget;
    (void)event;
    
    pid_t pid = fork();
    if (pid == 0) {
        // Launch terminal
        execl("./blackline-terminal", "blackline-terminal", NULL);
        exit(0);
    } else if (pid > 0) {
        // Parent process - animate window close
        animate_fade_out_with_scale(GTK_WIDGET(window), 200);
        g_timeout_add(200, (GSourceFunc)gtk_window_close, window);
    }
    return TRUE;
}

// Minimize button handler with animation - FIXED VERSION
static void on_minimize_clicked(GtkButton *button, gpointer window) 
{
    (void)button;
    
    GtkWidget *win = GTK_WIDGET(window);
    
    // Get panel position using GdkMonitor
    GdkDisplay *display = gtk_widget_get_display(win);
    GdkMonitor *monitor = gdk_display_get_primary_monitor(display);
    GdkRectangle geometry;
    gdk_monitor_get_geometry(monitor, &geometry);
    
    int panel_x = geometry.x;
    int panel_y = geometry.y;
    int panel_width = geometry.width;
    int panel_height = 30;
    
    // Get current window position and size
    int x, y, width, height;
    gtk_window_get_position(GTK_WINDOW(win), &x, &y);
    gtk_window_get_size(GTK_WINDOW(win), &width, &height);
    
    // Animate minimize to panel
    Animation *anim = g_new0(Animation, 1);
    anim->widget = win;
    anim->start_x = x;
    anim->start_y = y;
    anim->target_x = panel_x + panel_width/2 - 10;
    anim->target_y = panel_y + panel_height/2 - 5;
    anim->start_width = width;
    anim->start_height = height;
    anim->target_width = 20;
    anim->target_height = 20;
    anim->start_opacity = 1.0;
    anim->target_opacity = 0.0;
    anim->duration = 300;
    anim->start_time = g_get_monotonic_time() / 1000;
    anim->easing = ANIM_EASE_IN_OUT_QUAD;
    anim->active = TRUE;
    
    // Instead of iconifying at the end, just hide the window
    anim->on_complete = (void (*)(gpointer))gtk_widget_hide;
    anim->complete_data = win;
    
    is_minimized = TRUE;
    gtk_widget_add_tick_callback(win, on_animation_tick, anim, g_free);
}

// Window state change handler for restore animation
static gboolean on_window_state_changed(GtkWidget *window, GdkEventWindowState *event, gpointer data) 
{
    (void)data;
    
    if (is_minimized && !(event->new_window_state & GDK_WINDOW_STATE_ICONIFIED)) {
        is_minimized = FALSE;
        
        // Get panel position
        GdkDisplay *display = gtk_widget_get_display(window);
        GdkMonitor *monitor = gdk_display_get_primary_monitor(display);
        GdkRectangle geometry;
        gdk_monitor_get_geometry(monitor, &geometry);
        
        int panel_x = geometry.x;
        int panel_y = geometry.y;
        int panel_width = geometry.width;
        int panel_height = 30;
        
        // Get target position and size
        int x, y, width, height;
        gtk_window_get_position(GTK_WINDOW(window), &x, &y);
        gtk_window_get_size(GTK_WINDOW(window), &width, &height);
        
        // Start from panel
        gtk_window_move(GTK_WINDOW(window), panel_x + panel_width/2 - 10, panel_y + panel_height/2 - 5);
        gtk_window_resize(GTK_WINDOW(window), 20, 20);
        gtk_widget_set_opacity(window, 0.0);
        gtk_widget_show(window);
        
        // Animate restore
        Animation *anim = g_new0(Animation, 1);
        anim->widget = window;
        anim->start_x = panel_x + panel_width/2 - 10;
        anim->start_y = panel_y + panel_height/2 - 5;
        anim->target_x = x;
        anim->target_y = y;
        anim->start_width = 20;
        anim->start_height = 20;
        anim->target_width = width;
        anim->target_height = height;
        anim->start_opacity = 0.0;
        anim->target_opacity = 1.0;
        anim->duration = 400;
        anim->start_time = g_get_monotonic_time() / 1000;
        anim->easing = ANIM_EASE_OUT_ELASTIC;
        anim->active = TRUE;
        
        gtk_widget_add_tick_callback(window, on_animation_tick, anim, g_free);
    }
    
    return FALSE;
}

// Tool definitions for view mode
static ToolItem tools[] = {
    {"File Manager", "📁", launch_file_manager, launch_file_manager_event, NULL},
    {"Text Editor", "📝", launch_text_editor, launch_text_editor_event, NULL},
    {"Terminal", ">_", launch_terminal, launch_terminal_event, NULL},
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
    // Animate window close
    animate_fade_out_with_scale(GTK_WIDGET(window), 250);
    g_timeout_add(250, (GSourceFunc)gtk_window_close, window);
}

// Window destroy handler to clean up the atom
static void on_window_destroy(GtkWidget *widget, gpointer data) 

{
    (void)widget;
    (void)data;
    
    if (global_display) {
        // Remove the atom property
        Atom tools_atom = XInternAtom(global_display, "_BLACKLINE_TOOLS_WINDOW", False);
        if (tools_atom != None) {
            XDeleteProperty(global_display, DefaultRootWindow(global_display), tools_atom);
            XFlush(global_display);
        }
    }
}

// Realize callback - called after window is realized and has an XID
static void on_window_realized(GtkWidget *window, gpointer data) 

{
    // Get display for atom operations
    Display *xdisplay = GDK_DISPLAY_XDISPLAY(gtk_widget_get_display(window));
    global_display = xdisplay;  // Store for cleanup
    
    // Get the X window ID
    GdkWindow *gdk_window = gtk_widget_get_window(window);
    if (gdk_window) {
        Window xwindow = GDK_WINDOW_XID(gdk_window);
        
        // Set window type to DOCK so window manager doesn't kill it
        Atom net_wm_window_type = XInternAtom(xdisplay, "_NET_WM_WINDOW_TYPE", False);
        Atom net_wm_window_type_dock = XInternAtom(xdisplay, "_NET_WM_WINDOW_TYPE_DOCK", False);
        XChangeProperty(xdisplay, xwindow, net_wm_window_type, XA_ATOM, 32,
                       PropModeReplace, (unsigned char*)&net_wm_window_type_dock, 1);
        
        // Set _NET_WM_PID property for easier window detection
        Atom net_wm_pid = XInternAtom(xdisplay, "_NET_WM_PID", False);
        pid_t pid = getpid();
        XChangeProperty(xdisplay, xwindow, net_wm_pid, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char*)&pid, 1);
        
        // Set a unique atom on the root window to identify this instance
        Atom tools_atom = XInternAtom(xdisplay, "_BLACKLINE_TOOLS_WINDOW", False);
        XChangeProperty(xdisplay, DefaultRootWindow(xdisplay), tools_atom, XA_WINDOW, 32,
                        PropModeReplace, (unsigned char*)&xwindow, 1);
        XFlush(xdisplay);
    }
}

// Helper functions for view toggle
static gboolean remove_old_container(gpointer user_data)
{
    ViewToggleData *data = (ViewToggleData *)user_data;
    
    if (data->old_container && GTK_IS_WIDGET(data->old_container)) {
        // Hide first, then destroy after a short delay
        gtk_widget_hide(data->old_container);
        
        // Use idle add to ensure we're not in the middle of an animation
        g_idle_add((GSourceFunc)gtk_widget_destroy, data->old_container);
        data->old_container = NULL;
    }
    
    return FALSE;
}

static gboolean reset_button_opacity(gpointer button)
{
    if (button && GTK_IS_WIDGET(button)) {
        gtk_widget_set_opacity(GTK_WIDGET(button), 1.0);
    }
    return FALSE;
}

// View toggle callback with animation - FIXED VERSION
static void on_view_toggle_clicked(GtkButton *button, gpointer user_data) 
{
    ViewToggleData *data = (ViewToggleData *)user_data;
    
    if (!data->scrolled_window || !data->container || !data->window || !data->view_button) {
        g_warning("Missing data in view toggle callback");
        return;
    }
    
    // Animate button press
    gtk_widget_set_opacity(GTK_WIDGET(button), 0.5);
    
    // Get current mode and toggle
    ViewMode current = view_mode_get_current();
    ViewMode new_mode = (current == VIEW_MODE_LIST) ? VIEW_MODE_GRID : VIEW_MODE_LIST;
    
    // Store the current container to be removed later
    GtkWidget *old_container = data->container;
    
    // Hide the old container immediately to prevent visual glitches
    gtk_widget_hide(old_container);
    
    // Create new container with new mode
    GtkWidget *new_container = view_mode_create_container(tools, num_tools, new_mode, data->window);
    gtk_widget_set_opacity(new_container, 0.0);
    
    // Remove old container from scrolled window
    GtkWidget *current_child = gtk_bin_get_child(GTK_BIN(data->scrolled_window));
    if (current_child) {
        gtk_container_remove(GTK_CONTAINER(data->scrolled_window), current_child);
    }
    
    // Add new container to scrolled window
    gtk_container_add(GTK_CONTAINER(data->scrolled_window), new_container);
    
    // Update data
    data->container = new_container;
    
    // Set new mode
    view_mode_set_current(new_mode);
    
    // Update button label and window size
    if (new_mode == VIEW_MODE_LIST) {
        gtk_button_set_label(button, "📋 List");
        gtk_window_set_default_size(GTK_WINDOW(data->window), 300, 400);
    } else {
        gtk_button_set_label(button, "🔲 Grid");
        gtk_window_set_default_size(GTK_WINDOW(data->window), 350, 400);
    }
    
    // Show all widgets
    gtk_widget_show_all(data->window);
    
    // Fade in new container
    Animation *fade_in = g_new0(Animation, 1);
    fade_in->widget = new_container;
    fade_in->start_opacity = 0.0;
    fade_in->target_opacity = 1.0;
    fade_in->duration = 300;
    fade_in->start_time = g_get_monotonic_time() / 1000;
    fade_in->easing = ANIM_EASE_OUT_QUAD;
    fade_in->active = TRUE;
    gtk_widget_add_tick_callback(new_container, on_animation_tick, fade_in, g_free);
    
    // Destroy the old container after a short delay
    g_timeout_add(50, (GSourceFunc)gtk_widget_destroy, old_container);
    
    // Restore button opacity after fade in
    g_timeout_add(300, reset_button_opacity, button);
    
    // Save the new mode to config file
    view_mode_save();
    
    g_print("Switched to %s mode\n", new_mode == VIEW_MODE_LIST ? "LIST" : "GRID");
}

static void activate(GtkApplication *app, gpointer user_data) 

{
    // Initialize animation system
    animation_init();
    
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "BlackLine Tools");
    
    // Load saved view mode - this loads from file
    view_mode_load();
    
    // Get the saved mode (now properly loaded)
    ViewMode saved_mode = view_mode_get_current();
    
    // Debug print to verify mode is loaded correctly
    if (saved_mode == VIEW_MODE_LIST) {
        g_print("Tools container starting in LIST mode\n");
    } else {
        g_print("Tools container starting in GRID mode\n");
    }
    
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
    
    // Connect window state signal for minimize/restore animations
    g_signal_connect(window, "window-state-event", G_CALLBACK(on_window_state_changed), NULL);
    
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    
    // Title bar with view toggle, minimize, and close
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<b>Tools</b>");
    gtk_box_pack_start(GTK_BOX(hbox), title, TRUE, TRUE, 0);
    
    // View toggle button
    GtkWidget *view_btn = gtk_button_new_with_label("");
    gtk_widget_set_size_request(view_btn, 60, 25);
    gtk_box_pack_end(GTK_BOX(hbox), view_btn, FALSE, FALSE, 0);
    
    // Minimize button
    GtkWidget *min_btn = gtk_button_new_with_label("─");
    gtk_widget_set_size_request(min_btn, 30, 25);
    g_signal_connect(min_btn, "clicked", G_CALLBACK(on_minimize_clicked), window);
    gtk_box_pack_end(GTK_BOX(hbox), min_btn, FALSE, FALSE, 0);
    
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
    
    // Create container with saved mode - this will use the loaded mode
    GtkWidget *tools_container = view_mode_create_container(tools, num_tools, saved_mode, window);
    
    // Add the tools container to the scrolled window
    gtk_container_add(GTK_CONTAINER(scrolled_window), tools_container);
    
    // Pack the scrolled window into the main vbox
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);
    
    // Create and store toggle data
    ViewToggleData *toggle_data = g_new0(ViewToggleData, 1);
    toggle_data->scrolled_window = scrolled_window;
    toggle_data->container = tools_container;
    toggle_data->old_container = NULL;
    toggle_data->window = window;
    toggle_data->view_button = view_btn;
    
    // Store data on view button for later use
    g_object_set_data_full(G_OBJECT(view_btn), "toggle-data", toggle_data, g_free);
    
    // Connect view toggle button
    g_signal_connect(view_btn, "clicked", G_CALLBACK(on_view_toggle_clicked), toggle_data);
    
    // Connect destroy signal to clean up atom
    g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), NULL);
    
    // Connect realize signal to set atoms after window is ready
    g_signal_connect(window, "realize", G_CALLBACK(on_window_realized), NULL);
    
    // Add some spacing at the bottom
    GtkWidget *bottom_spacer = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(vbox), bottom_spacer, TRUE, TRUE, 0);
    
    // CSS with animation support - REMOVED TRANSFORM PROPERTIES TO FIX WARNINGS
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "window { background-color: #0b0f14; color: #ffffff; border: 1px solid #00ff88; "
        "         transition: all 200ms ease-out; }"
        "button { background-color: #1e2429; color: #00ff88; border: none; "
        "         transition: all 150ms ease-out; }"
        "button:hover { background-color: #2a323a; }"
        "button:active { background-color: #2a323a; }"
        "label { color: #ffffff; }"
        "entry { background-color: #1e2429; color: #ffffff; border: 1px solid #00ff88; "
        "        transition: all 200ms ease; }"
        "entry:focus { border-color: #44ffaa; box-shadow: 0 0 10px rgba(0, 255, 136, 0.5); }"
        "scrolledwindow { border: none; background-color: #0b0f14; }"
        /* Style the scrollbar */
        "scrollbar { background-color: #1e2429; }"
        "scrollbar slider { background-color: #00ff88; border-radius: 4px; min-width: 8px; min-height: 8px; "
        "                  transition: background-color 200ms ease; }"
        "scrollbar slider:hover { background-color: #33ffaa; }"
        "scrollbar slider:active { background-color: #00cc66; }"
        "scrollbar trough { background-color: #2a323a; border-radius: 4px; }",
        -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    
    // Show all widgets
    gtk_widget_show_all(window);
    
    // Animate window opening with simple fade
    gtk_widget_set_opacity(window, 0.0);
    Animation *open_anim = g_new0(Animation, 1);
    open_anim->widget = window;
    open_anim->start_opacity = 0.0;
    open_anim->target_opacity = 1.0;
    open_anim->duration = 400;
    open_anim->start_time = g_get_monotonic_time() / 1000;
    open_anim->easing = ANIM_EASE_OUT_QUAD;  // Simple fade
    open_anim->active = TRUE;
    gtk_widget_add_tick_callback(window, on_animation_tick, open_anim, g_free);
}

// Single-instance check: if another instance exists, raise it and exit.
static Window find_existing_instance(Display *dpy) 
{
    Atom atom = XInternAtom(dpy, "_BLACKLINE_TOOLS_WINDOW", False);
    if (atom == None) return None;
    
    Window root = DefaultRootWindow(dpy);
    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char *data = NULL;
    
    int status = XGetWindowProperty(dpy, root, atom, 0, 1, False, XA_WINDOW,
                                    &actual_type, &actual_format, &nitems, &bytes_after,
                                    &data);
    
    if (status == Success && actual_type == XA_WINDOW && nitems == 1) {
        Window win = *(Window*)data;
        XFree(data);
        
        // Verify the window still exists and is mapped
        XWindowAttributes attrs;
        if (XGetWindowAttributes(dpy, win, &attrs) != 0) {
            if (attrs.map_state != IsUnmapped) {
                return win;
            }
        }
        
        // Window exists but is unmapped - delete the stale atom
        XDeleteProperty(dpy, root, atom);
        XFlush(dpy);
    } else {
        if (data) XFree(data);
    }
    
    return None;
}

static void raise_window(Display *dpy, Window win) 
{
    XRaiseWindow(dpy, win);
    XMapRaised(dpy, win);
    XFlush(dpy);
}

int main(int argc, char **argv) 

{
    // Check for existing instance
    Display *dpy = XOpenDisplay(NULL);
    if (dpy) {
        Window existing = find_existing_instance(dpy);
        if (existing != None) {
            raise_window(dpy, existing);
            XCloseDisplay(dpy);
            return 0;  // Exit
        }
        XCloseDisplay(dpy);
    }
    
    GtkApplication *app = gtk_application_new("org.blackline.tools", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}