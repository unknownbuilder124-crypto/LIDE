#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include "display/displaySettings.h"
#include "sound/sound.h"
#include "power/p_settings.h"
#include "network/network.h"
#include "mouse/mouse.h"
#include "privacy/privacy.h"
#include "bluetooth/bluetooth.h"

/*
 * settings.c
 * 
 * System settings application main window and tab management
Provides tabbed interface for display, sound, and system settings.
Routes configuration changes to respective handlers.
 *
 * This module is part of the LIDE desktop environment system.
 * See the main window manager (wm/) and session management (session/)
 * for system architecture overview.
 */

// static int is_dragging = 0;
// static int drag_start_x, drag_start_y;
// static int window_start_x, window_start_y;
// static Display *global_display = NULL;

// Structure to hold application state
typedef struct {
    GtkWidget *window;
    GtkWidget *header;
    GtkWidget *notebook;
    GtkWidget *statusbar;
    
    // Drag data
    gboolean is_dragging;
    int drag_start_x;
    int drag_start_y;
    int window_start_x;
    int window_start_y;
    gboolean is_resizing;      
} AppState;

// ----------------------------------------------------------------------
// Window control functions
static void on_minimize_clicked(GtkButton *button, gpointer window) 

{
    (void)button;
    gtk_window_iconify(GTK_WINDOW(window));
}

static void on_maximize_clicked(GtkButton *button, gpointer window) 

{
    (void)button;
    if (gdk_window_get_state(gtk_widget_get_window(GTK_WIDGET(window))) & GDK_WINDOW_STATE_MAXIMIZED) {
        gtk_window_unmaximize(GTK_WINDOW(window));
    } else {
        gtk_window_maximize(GTK_WINDOW(window));
    }
}

static void on_close_clicked(GtkButton *button, gpointer window) 

{
    (void)button;
    gtk_window_close(GTK_WINDOW(window));
}

// ----------------------------------------------------------------------
// Dragging handlers 
static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data)

{
    AppState *state = (AppState *)data;

    if (event->button == 1)  // Left click
    {
        state->is_dragging = TRUE;
        state->drag_start_x = event->x_root;
        state->drag_start_y = event->y_root;
        
        // Get current window position
        gtk_window_get_position(GTK_WINDOW(state->window), &state->window_start_x, &state->window_start_y);
        
        gtk_window_present(GTK_WINDOW(state->window));
        return TRUE;
    }
    return FALSE;
}

static gboolean on_button_release(GtkWidget *widget, GdkEventButton *event, gpointer data)

{
    AppState *state = (AppState *)data;

    if (event->button == 1) {
        state->is_dragging = FALSE;
        return TRUE;
    }
    return FALSE;
}

static gboolean on_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer data)

{
    AppState *state = (AppState *)data;

    if (state->is_dragging) {
        int dx = event->x_root - state->drag_start_x;
        int dy = event->y_root - state->drag_start_y;

        gtk_window_move(GTK_WINDOW(state->window), 
                       state->window_start_x + dx, 
                       state->window_start_y + dy);
        return TRUE;
    }
    return FALSE;
}

// ----------------------------------------------------------------------
// Window state change handler
static gboolean on_window_state_changed(GtkWidget *window, GdkEventWindowState *event, gpointer data) 

{
    (void)data;
    return FALSE;
}

// ----------------------------------------------------------------------
// Window destroy handler
static void on_window_destroy(GtkWidget *widget, gpointer data) 

{
    (void)widget;
    (void)data;
    
    AppState *state = (AppState *)g_object_get_data(G_OBJECT(widget), "app-state");
    if (state) {
        g_free(state);
    }
}

// ----------------------------------------------------------------------
// Create custom titlebar with buttons 
static GtkWidget *create_custom_titlebar(const char *title, AppState *state)

{
    GtkWidget *header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_size_request(header, -1, 30);
    gtk_widget_set_name(header, "custom-titlebar");
    
    // Title label
    GtkWidget *title_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title_label), title);
    gtk_widget_set_halign(title_label, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(title_label, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(header), title_label, TRUE, TRUE, 0);
    
    // Window control buttons
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_pack_end(GTK_BOX(header), button_box, FALSE, FALSE, 5);
    
    // Minimize button
    GtkWidget *min_btn = gtk_button_new_with_label("—");
    gtk_widget_set_size_request(min_btn, 25, 20);
    g_signal_connect(min_btn, "clicked", G_CALLBACK(on_minimize_clicked), state->window);
    gtk_box_pack_start(GTK_BOX(button_box), min_btn, FALSE, FALSE, 0);
    
    // Maximize button
    GtkWidget *max_btn = gtk_button_new_with_label("□");
    gtk_widget_set_size_request(max_btn, 25, 20);
    g_signal_connect(max_btn, "clicked", G_CALLBACK(on_maximize_clicked), state->window);
    gtk_box_pack_start(GTK_BOX(button_box), max_btn, FALSE, FALSE, 0);
    
    // Close button
    GtkWidget *close_btn = gtk_button_new_with_label("✕");
    gtk_widget_set_size_request(close_btn, 25, 20);
    g_signal_connect(close_btn, "clicked", G_CALLBACK(on_close_clicked), state->window);
    gtk_box_pack_start(GTK_BOX(button_box), close_btn, FALSE, FALSE, 0);
    
    return header;
}

// ----------------------------------------------------------------------
// Placeholder functions for other tabs 
static GtkWidget *mouse_tab_new(void)

{
    return mouse_settings_tab_new();
}

static GtkWidget *network_tab_new(void)

{
    return network_settings_tab_new();
}

static GtkWidget *power_tab_new(void)

{
    return power_settings_tab_new();
}

static GtkWidget *privacy_tab_new(void)

{
    return privacy_settings_tab_new();
}

/*
static GtkWidget *wifi_tab_new(void)

{
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(box), 20);
    
    GtkWidget *label = gtk_label_new("Wi‑Fi settings not implemented yet.");
    gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 0);
    
    return box;
}
    */

static GtkWidget *bluetooth_tab_new(void)

{
    return create_bluetooth_tab();
}

// ----------------------------------------------------------------------
// Main window creation
static void activate(GtkApplication *app, gpointer user_data)

{
    (void)user_data;
    
    AppState *state = g_new0(AppState, 1);
    state->window = gtk_application_window_new(app);
    state->is_dragging = FALSE;
    state->window_start_x = 0;
    state->window_start_y = 0;
    
    gtk_window_set_title(GTK_WINDOW(state->window), "BlackLine Settings");
    gtk_window_set_default_size(GTK_WINDOW(state->window), 750, 550);
    gtk_window_set_resizable(GTK_WINDOW(state->window), TRUE);
    
    // Get monitor geometry for positioning below panel
    GdkDisplay *display = gdk_display_get_default();
    GdkMonitor *monitor = gdk_display_get_primary_monitor(display);
    if (!monitor) monitor = gdk_display_get_monitor_at_point(display, 0, 0);
    GdkRectangle monitor_geom;
    gdk_monitor_get_geometry(monitor, &monitor_geom);
    
    // Panel height (adjust based on your panel)
    int panel_height = 40;
    
    // Position window below the panel
    int window_width, window_height;
    gtk_window_get_default_size(GTK_WINDOW(state->window), &window_width, &window_height);
    
    // Calculate position: centered horizontally, below panel vertically
    int pos_x = (monitor_geom.width - window_width) / 2;
    int pos_y = panel_height + 10;  // Panel height + small gap
    
    // Ensure window stays within screen bounds
    if (pos_x < 0) pos_x = 10;
    if (pos_y + window_height > monitor_geom.height) {
        pos_y = monitor_geom.height - window_height - 10;
    }
    
    gtk_window_move(GTK_WINDOW(state->window), pos_x, pos_y);
    
    // Remove default titlebar
    gtk_window_set_decorated(GTK_WINDOW(state->window), FALSE);
    
    // Enable events on the window for dragging 
    gtk_widget_add_events(state->window, GDK_BUTTON_PRESS_MASK |
                                          GDK_BUTTON_RELEASE_MASK |
                                          GDK_POINTER_MOTION_MASK);
    
    // Connect drag handlers to the WINDOW
    g_signal_connect(state->window, "button-press-event", G_CALLBACK(on_button_press), state);
    g_signal_connect(state->window, "button-release-event", G_CALLBACK(on_button_release), state);
    g_signal_connect(state->window, "motion-notify-event", G_CALLBACK(on_motion_notify), state);
    
    // Store state in window for callbacks
    g_object_set_data_full(G_OBJECT(state->window), "app-state", state, NULL);
    
    // Main vertical box
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(state->window), vbox);
    
    // Custom titlebar with buttons only 
    GtkWidget *titlebar = create_custom_titlebar("<b>Settings</b>", state);
    gtk_box_pack_start(GTK_BOX(vbox), titlebar, FALSE, FALSE, 0);
    
    // Separator
    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), sep, FALSE, FALSE, 0);
    
    // Create a scrolled window for the notebook content
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                   GTK_POLICY_NEVER,      // No horizontal scrollbar
                                   GTK_POLICY_AUTOMATIC); // Vertical scrollbar when needed
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scrolled_window), 400);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 5);
    
    // Notebook for tabs
    state->notebook = gtk_notebook_new();
    gtk_container_add(GTK_CONTAINER(scrolled_window), state->notebook);
    
    // Add tabs
    gtk_notebook_append_page(GTK_NOTEBOOK(state->notebook),
                              display_settings_tab_new(),
                              gtk_label_new("Display"));
    
    gtk_notebook_append_page(GTK_NOTEBOOK(state->notebook),
                              mouse_tab_new(),
                              gtk_label_new("Mouse"));
    
    gtk_notebook_append_page(GTK_NOTEBOOK(state->notebook),
                              network_tab_new(),
                              gtk_label_new("Network"));
    
    gtk_notebook_append_page(GTK_NOTEBOOK(state->notebook),
                              sound_tab_new(),
                              gtk_label_new("Sound"));
    
    gtk_notebook_append_page(GTK_NOTEBOOK(state->notebook),
                              power_tab_new(),
                              gtk_label_new("Power"));
    
    gtk_notebook_append_page(GTK_NOTEBOOK(state->notebook),
                              privacy_tab_new(),
                              gtk_label_new("Privacy"));
    
    gtk_notebook_append_page(GTK_NOTEBOOK(state->notebook),
                              bluetooth_tab_new(),
                              gtk_label_new("Bluetooth"));
    
    // Status bar
    state->statusbar = gtk_statusbar_new();
    gtk_box_pack_start(GTK_BOX(vbox), state->statusbar, FALSE, FALSE, 0);
    
    guint context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(state->statusbar), "settings");
    gtk_statusbar_push(GTK_STATUSBAR(state->statusbar), context_id, "Settings ready");
    
    // Connect window state signal
    g_signal_connect(state->window, "window-state-event", G_CALLBACK(on_window_state_changed), NULL);
    
    // Connect destroy signal
    g_signal_connect(state->window, "destroy", G_CALLBACK(on_window_destroy), NULL);
    
    // CSS
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "window { background-color: #0b0f14; color: #ffffff; }"
        "#custom-titlebar { background-color: #1a1e24; color: #ffffff; padding: 2px; }"
        "#custom-titlebar button { background-color: #2a323a; color: #00ff88; border: none; min-height: 20px; min-width: 25px; }"
        "#custom-titlebar button:hover { background-color: #3a424a; }"
        "#custom-titlebar button:active { background-color: #00ff88; color: #0b0f14; }"
        "button { background-color: #1e2429; color: #00ff88; border: 1px solid #00ff88; }"
        "button:hover { background-color: #2a323a; }"
        "label { color: #ffffff; }"
        "entry { background-color: #1e2429; color: #ffffff; border: 1px solid #00ff88; }"
        "entry:focus { border-color: #44ffaa; }"
        "notebook { background-color: #0b0f14; }"
        "notebook tab { background-color: #1a1e24; color: #ffffff; }"
        "notebook tab:checked { background-color: #00ff88; color: #0b0f14; }"
        "statusbar { background-color: #0b0f14; color: #00ff88; }"
        "frame { border-color: #2a323a; }"
        "scrolledwindow { border: none; background-color: #0b0f14; }"
        "scrollbar { background-color: #1e2429; }"
        "scrollbar slider { background-color: #62316b; border-radius: 4px; min-width: 8px; min-height: 8px; }"
        "scrollbar slider:hover { background-color: #7a3b8b; }"
        "scrollbar slider:active { background-color: #9a4bab; }"
        "scrollbar trough { background-color: #2a323a; border-radius: 4px; }",
        -1, NULL);
        
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
    
    gtk_widget_show_all(state->window);
}

// ----------------------------------------------------------------------
int main(int argc, char **argv)

{
    GtkApplication *app = gtk_application_new("org.blackline.settings",
                                               G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}