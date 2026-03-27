#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include "auto.h"

/* Forward declaration for auto.c */
void set_tools_window(GtkWidget *window);

// Global display connection for cleanup
static Display *global_display = NULL;
static GtkWidget *main_window = NULL;

// Animation data for slide‑in effect
typedef struct {
    GtkWindow *window;
    int target_x;
    int target_y;
    int current_y;
    int step;
    guint timeout_id;
} AnimationData;

// Launch functions 
/**
 * Close the tools window.
 *
 * Called from child processes after launching an application.
 */
static void close_tools_window(void)
{
    if (main_window) {
        gtk_window_close(GTK_WINDOW(main_window));
    }
}

/**
 * Launch the file manager and close the tools window.
 */
static void launch_file_manager(GtkButton *button, gpointer window) 
{
    (void)button;
    (void)window;
    
    pid_t pid = fork();
    if (pid == 0) {
        execl("./blackline-fm", "blackline-fm", NULL);
        exit(0);
    } else if (pid > 0) {
        close_tools_window();
    }
}

/**
 * Event handler for launching the file manager.
 *
 * @return TRUE to indicate event was handled.
 */
static gboolean launch_file_manager_event(GtkWidget *widget, GdkEventButton *event, gpointer window)
{
    (void)widget;
    (void)event;
    (void)window;
    
    pid_t pid = fork();
    if (pid == 0) {
        execl("./blackline-fm", "blackline-fm", NULL);
        exit(0);
    } else if (pid > 0) {
        close_tools_window();
    }
    return TRUE;
}

/**
 * Launch the text editor and close the tools window.
 */
static void launch_text_editor(GtkButton *button, gpointer window) 
{
    (void)button;
    (void)window;
    
    pid_t pid = fork();
    if (pid == 0) {
        execl("./blackline-editor", "blackline-editor", NULL);
        exit(0);
    } else if (pid > 0) {
        close_tools_window();
    }
}

/**
 * Event handler for launching the text editor.
 *
 * @return TRUE to indicate event was handled.
 */
static gboolean launch_text_editor_event(GtkWidget *widget, GdkEventButton *event, gpointer window)
{
    (void)widget;
    (void)event;
    (void)window;
    
    pid_t pid = fork();
    if (pid == 0) {
        execl("./blackline-editor", "blackline-editor", NULL);
        exit(0);
    } else if (pid > 0) {
        close_tools_window();
    }
    return TRUE;
}

/**
 * Launch the calculator and close the tools window.
 */
static void launch_calculator(GtkButton *button, gpointer window) 
{
    (void)button;
    (void)window;
    
    pid_t pid = fork();
    if (pid == 0) {
        execl("./blackline-calculator", "blackline-calculator", NULL);
        exit(0);
    } else if (pid > 0) {
        close_tools_window();
    }
}

/**
 * Event handler for launching the calculator.
 *
 * @return TRUE to indicate event was handled.
 */
static gboolean launch_calculator_event(GtkWidget *widget, GdkEventButton *event, gpointer window)
{
    (void)widget;
    (void)event;
    (void)window;
    
    pid_t pid = fork();
    if (pid == 0) {
        execl("./blackline-calculator", "blackline-calculator", NULL);
        exit(0);
    } else if (pid > 0) {
        close_tools_window();
    }
    return TRUE;
}

/**
 * Launch the system monitor and close the tools window.
 */
static void launch_system_monitor(GtkButton *button, gpointer window) 
{
    (void)button;
    (void)window;
    
    pid_t pid = fork();
    if (pid == 0) {
        execl("./blackline-system-monitor", "blackline-system-monitor", NULL);
        exit(0);
    } else if (pid > 0) {
        close_tools_window();
    }
}

/**
 * Event handler for launching the system monitor.
 *
 * @return TRUE to indicate event was handled.
 */
static gboolean launch_system_monitor_event(GtkWidget *widget, GdkEventButton *event, gpointer window)
{
    (void)widget;
    (void)event;
    (void)window;
    
    pid_t pid = fork();
    if (pid == 0) {
        execl("./blackline-system-monitor", "blackline-system-monitor", NULL);
        exit(0);
    } else if (pid > 0) {
        close_tools_window();
    }
    return TRUE;
}

// Web browser launcher function
/**
 * Launch the VoidFox web browser and close the tools window.
 */
static void launch_web_browser(GtkButton *button, gpointer window) 
{
    (void)button;
    (void)window;
    
    pid_t pid = fork();
    if (pid == 0) {
        execl("./voidfox", "voidfox", NULL);
        exit(0);
    } else if (pid > 0) {
        close_tools_window();
    }
}

/**
 * Event handler for launching the web browser.
 *
 * @return TRUE to indicate event was handled.
 */
static gboolean launch_web_browser_event(GtkWidget *widget, GdkEventButton *event, gpointer window)
{
    (void)widget;
    (void)event;
    (void)window;
    
    pid_t pid = fork();
    if (pid == 0) {
        execl("./voidfox", "voidfox", NULL);
        exit(0);
    } else if (pid > 0) {
        close_tools_window();
    }
    return TRUE;
}

// Firefox wrapper launcher function
/**
 * Launch the Firefox wrapper after verifying its existence.
 *
 * Checks for the wrapper binary in multiple locations. If not found,
 * displays an error dialog with build instructions.
 */
static void launch_firefox_wrapper(GtkButton *button, gpointer window) 
{
    (void)button;
    (void)window;
    
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
        if (access("./tools/firefox/firefox-wrapper", X_OK) == 0) {
            execl("./tools/firefox/firefox-wrapper", "firefox-wrapper", NULL);
        } else {
            execl("./firefox-wrapper", "firefox-wrapper", NULL);
        }
        exit(0);
    } else if (pid > 0) {
        close_tools_window();
    }
}

/**
 * Event handler for launching the Firefox wrapper.
 *
 * @return TRUE to indicate event was handled.
 */
static gboolean launch_firefox_wrapper_event(GtkWidget *widget, GdkEventButton *event, gpointer window)
{
    (void)widget;
    (void)event;
    (void)window;
    
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
        if (access("./tools/firefox/firefox-wrapper", X_OK) == 0) {
            execl("./tools/firefox/firefox-wrapper", "firefox-wrapper", NULL);
        } else {
            execl("./firefox-wrapper", "firefox-wrapper", NULL);
        }
        exit(0);
    } else if (pid > 0) {
        close_tools_window();
    }
    return TRUE;
}

// Terminal launcher function
/**
 * Launch the terminal and close the tools window.
 */
static void launch_terminal(GtkButton *button, gpointer window) 
{
    (void)button;
    (void)window;
    
    pid_t pid = fork();
    if (pid == 0) {
        execl("./blackline-terminal", "blackline-terminal", NULL);
        exit(0);
    } else if (pid > 0) {
        close_tools_window();
    }
}

/**
 * Event handler for launching the terminal.
 *
 * @return TRUE to indicate event was handled.
 */
static gboolean launch_terminal_event(GtkWidget *widget, GdkEventButton *event, gpointer window)
{
    (void)widget;
    (void)event;
    (void)window;
    
    pid_t pid = fork();
    if (pid == 0) {
        execl("./blackline-terminal", "blackline-terminal", NULL);
        exit(0);
    } else if (pid > 0) {
        close_tools_window();
    }
    return TRUE;
}

/**
 * Launch the image viewer and close the tools window.
 */
static void launch_image_viewer(GtkButton *button, gpointer window) 
{
    (void)button;
    (void)window;
    
    pid_t pid = fork();
    if (pid == 0) {
        execl("./blackline-image-viewer", "blackline-image-viewer", NULL);
        exit(0);
    } else if (pid > 0) {
        close_tools_window();
    }
}

/**
 * Event handler for launching the image viewer.
 *
 * @return TRUE to indicate event was handled.
 */
static gboolean launch_image_viewer_event(GtkWidget *widget, GdkEventButton *event, gpointer window)
{
    (void)widget;
    (void)event;
    (void)window;
    
    pid_t pid = fork();
    if (pid == 0) {
        execl("./blackline-image-viewer", "blackline-image-viewer", NULL);
        exit(0);
    } else if (pid > 0) {
        close_tools_window();
    }
    return TRUE;
}

// Settings tool launcher function
/**
 * Launch the settings application and close the tools window.
 */
static void launch_settings(GtkButton *button, gpointer window) 
{
    (void)button;
    (void)window;
    
    pid_t pid = fork();
    if (pid == 0) {
        execl("./blackline-settings", "blackline-settings", NULL);
        exit(0);
    } else if (pid > 0) {
        close_tools_window();
    }
}

/**
 * Event handler for launching the settings application.
 *
 * @return TRUE to indicate event was handled.
 */
static gboolean launch_settings_event(GtkWidget *widget, GdkEventButton *event, gpointer window)
{
    (void)widget;
    (void)event;
    (void)window;
    
    pid_t pid = fork();
    if (pid == 0) {
        execl("./blackline-settings", "blackline-settings", NULL);
        exit(0);
    } else if (pid > 0) {
        close_tools_window();
    }
    return TRUE;
}

/**
 * Window state change handler for restore events.
 *
 * @return FALSE to allow further signal propagation.
 */
static gboolean on_window_state_changed(GtkWidget *window, GdkEventWindowState *event, gpointer data) 
{
    (void)data;
    return FALSE;
}


static ToolItem static_tools[] = 
{
    {"File Manager", "📁", launch_file_manager, launch_file_manager_event, NULL},
    {"Text Editor", "📝", launch_text_editor, launch_text_editor_event, NULL},
    {"Terminal", "$", launch_terminal, launch_terminal_event, NULL},
    {"Calculator", "🔢", launch_calculator, launch_calculator_event, NULL},
    {"System Monitor", "📊", launch_system_monitor, launch_system_monitor_event, NULL},
    {"Settings", "⚙️", launch_settings, launch_settings_event, NULL},
    {"Image", "🖼️", launch_image_viewer, launch_image_viewer_event, NULL},
    {"VoidFox", "🌐", launch_web_browser, launch_web_browser_event, NULL},
    {"Firefox", "🦊", launch_firefox_wrapper, launch_firefox_wrapper_event, NULL}
};

static int num_static_tools = sizeof(static_tools) / sizeof(static_tools[0]);

/**
 * Creates a grid container for tools (vertical icon + label).
 *
 * @param tools Array of ToolItem structures.
 * @param num_tools Number of tools.
 * @param window Parent window reference.
 * @return GtkWidget container with all tool buttons.
 */
static GtkWidget* create_tools_container(ToolItem *tools, int num_tools, gpointer window)
{
    GtkWidget *flowbox = gtk_flow_box_new();
    gtk_flow_box_set_homogeneous(GTK_FLOW_BOX(flowbox), FALSE);
    gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(flowbox), 6);
    gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(flowbox), GTK_SELECTION_NONE);
    gtk_flow_box_set_column_spacing(GTK_FLOW_BOX(flowbox), 4);
    gtk_flow_box_set_row_spacing(GTK_FLOW_BOX(flowbox), 2);
    
    for (int i = 0; i < num_tools; i++) {
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
        gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
        gtk_widget_set_margin_start(box, 6);
        gtk_widget_set_margin_end(box, 6);
        gtk_widget_set_margin_top(box, 4);
        gtk_widget_set_margin_bottom(box, 4);
        
        GtkWidget *icon_label = gtk_label_new(tools[i].icon);
        gtk_label_set_xalign(GTK_LABEL(icon_label), 0.5);
        PangoFontDescription *font_desc = pango_font_description_from_string("Sans 20");
        gtk_widget_override_font(icon_label, font_desc);
        pango_font_description_free(font_desc);
        
        GtkWidget *name_label = gtk_label_new(tools[i].name);
        gtk_label_set_xalign(GTK_LABEL(name_label), 0.5);
        
        gtk_box_pack_start(GTK_BOX(box), icon_label, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), name_label, FALSE, FALSE, 0);
        
        GtkWidget *event_box = gtk_event_box_new();
        gtk_container_add(GTK_CONTAINER(event_box), box);
        gtk_widget_set_name(event_box, "tool-item");
        
        if (tools[i].button_callback) {
            g_signal_connect(event_box, "button-release-event", G_CALLBACK(tools[i].event_callback), tools[i].user_data);
        }
        
        gtk_container_add(GTK_CONTAINER(flowbox), event_box);
    }
    
    return flowbox;
}

/**
 * Close button handler - destroys the window.
 */
static void on_close_clicked(GtkButton *button, gpointer window) 
{
    (void)button;
    gtk_window_close(GTK_WINDOW(window));
}

/**
 * Window destroy handler to clean up the X11 atom.
 *
 * Removes the _BLACKLINE_TOOLS_WINDOW property from the root window,
 * allowing new instances to launch after this one exits.
 */
static void on_window_destroy(GtkWidget *widget, gpointer data) 
{
    (void)widget;
    (void)data;
    
    if (global_display) {
        Atom tools_atom = XInternAtom(global_display, "_BLACKLINE_TOOLS_WINDOW", False);
        if (tools_atom != None) {
            XDeleteProperty(global_display, DefaultRootWindow(global_display), tools_atom);
            XFlush(global_display);
        }
    }
}

/**
 * Realize callback - sets X11 properties after window is realized.
 *
 * Configures window type as DOCK to prevent window manager from killing it,
 * sets _NET_WM_PID for identification, and stores the XID on the root window
 * for single-instance detection.
 */
static void on_window_realized(GtkWidget *window, gpointer data) 
{
    Display *xdisplay = GDK_DISPLAY_XDISPLAY(gtk_widget_get_display(window));
    global_display = xdisplay;
    
    GdkWindow *gdk_window = gtk_widget_get_window(window);
    if (gdk_window) {
        Window xwindow = GDK_WINDOW_XID(gdk_window);
        
        Atom net_wm_window_type = XInternAtom(xdisplay, "_NET_WM_WINDOW_TYPE", False);
        Atom net_wm_window_type_dock = XInternAtom(xdisplay, "_NET_WM_WINDOW_TYPE_DOCK", False);
        XChangeProperty(xdisplay, xwindow, net_wm_window_type, XA_ATOM, 32,
                       PropModeReplace, (unsigned char*)&net_wm_window_type_dock, 1);
        
        Atom net_wm_pid = XInternAtom(xdisplay, "_NET_WM_PID", False);
        pid_t pid = getpid();
        XChangeProperty(xdisplay, xwindow, net_wm_pid, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char*)&pid, 1);
        
        Atom tools_atom = XInternAtom(xdisplay, "_BLACKLINE_TOOLS_WINDOW", False);
        XChangeProperty(xdisplay, DefaultRootWindow(xdisplay), tools_atom, XA_WINDOW, 32,
                        PropModeReplace, (unsigned char*)&xwindow, 1);
        XFlush(xdisplay);
    }
}

/**
 * Slide animation step.
 *
 * Moves the window by a fixed amount each tick until it reaches the target Y.
 */
static gboolean animate_slide(gpointer data)
{
    AnimationData *anim = (AnimationData*)data;
    int new_y = anim->current_y + anim->step;
    
    if ((anim->step > 0 && new_y >= anim->target_y) ||
        (anim->step < 0 && new_y <= anim->target_y)) {
        new_y = anim->target_y;
        gtk_window_move(anim->window, anim->target_x, new_y);
        g_source_remove(anim->timeout_id);
        g_free(anim);
        return G_SOURCE_REMOVE;
    }
    
    gtk_window_move(anim->window, anim->target_x, new_y);
    anim->current_y = new_y;
    return G_SOURCE_CONTINUE;
}

/**
 * Start slide‑in animation from the top.
 *
 * @param window     The window to animate.
 * @param target_x   Final X coordinate.
 * @param target_y   Final Y coordinate.
 * @param start_y    Starting Y coordinate (usually -height).
 */
static void start_slide_animation(GtkWindow *window, int target_x, int target_y, int start_y)
{
    AnimationData *anim = g_new0(AnimationData, 1);
    anim->window = window;
    anim->target_x = target_x;
    anim->target_y = target_y;
    anim->current_y = start_y;
    anim->step = 32;
    
    anim->timeout_id = g_timeout_add(10, animate_slide, anim);
}

/**
 * Application activation callback.
 *
 * Creates the main window, sets up UI components, applies CSS styling,
 * and initializes the tools container.
 */
static void activate(GtkApplication *app, gpointer user_data) 
{
    GtkWidget *window = gtk_application_window_new(app);
    main_window = window;
    set_tools_window(window);
    
    gtk_window_set_title(GTK_WINDOW(window), "BlackLine Tools");
    
    // Use only static tools
    int total_tools = num_static_tools;
    ToolItem *all_tools = g_malloc(sizeof(ToolItem) * total_tools);
    for (int i = 0; i < total_tools; i++) {
        all_tools[i].name = g_strdup(static_tools[i].name);
        all_tools[i].icon = g_strdup(static_tools[i].icon);
        all_tools[i].button_callback = static_tools[i].button_callback;
        all_tools[i].event_callback = static_tools[i].event_callback;
        all_tools[i].user_data = static_tools[i].user_data;
    }
    
    // Compute full‑screen margins and target size
    GdkDisplay *display = gdk_display_get_default();
    GdkMonitor *monitor = gdk_display_get_primary_monitor(display);
    if (!monitor) monitor = gdk_display_get_monitor_at_point(display, 0, 0);
    GdkRectangle monitor_geom;
    gdk_monitor_get_geometry(monitor, &monitor_geom);
    
    int margin_top = 50;
    int margin_bottom = 50;
    int margin_left = 50;
    int margin_right = 50;
    
    int target_width = monitor_geom.width - margin_left - margin_right;
    int target_height = monitor_geom.height - margin_top - margin_bottom;
    if (target_width < 400) target_width = 400;
    if (target_height < 300) target_height = 300;
    
    int target_x = margin_left;
    int target_y = margin_top;
    
    gtk_window_set_default_size(GTK_WINDOW(window), target_width, target_height);
    gtk_window_move(GTK_WINDOW(window), target_x, -target_height);
    
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_NONE);
    gtk_window_set_resizable(GTK_WINDOW(window), TRUE);
    gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);
    gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
    
    g_signal_connect(window, "window-state-event", G_CALLBACK(on_window_state_changed), NULL);
    
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    
    // Title bar with only close button
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<b>Tools</b>");
    gtk_box_pack_start(GTK_BOX(hbox), title, TRUE, TRUE, 0);
    
    GtkWidget *close_btn = gtk_button_new_with_label("X");
    gtk_widget_set_size_request(close_btn, 30, 25);
    g_signal_connect(close_btn, "clicked", G_CALLBACK(on_close_clicked), window);
    gtk_box_pack_end(GTK_BOX(hbox), close_btn, FALSE, FALSE, 0);
    
    // Separator
    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), sep, FALSE, FALSE, 5);
    
    // Scrolled window with scrollbar (AUTOMATIC for both directions)
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scrolled_window), 400);
    
    // Create container with tools
    GtkWidget *tools_container = create_tools_container(all_tools, total_tools, window);
    gtk_container_add(GTK_CONTAINER(scrolled_window), tools_container);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);
    
    g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), NULL);
    g_signal_connect(window, "realize", G_CALLBACK(on_window_realized), NULL);
    
    // Add some spacing at the bottom
    GtkWidget *bottom_spacer = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(vbox), bottom_spacer, TRUE, TRUE, 0);
    
    // CSS for styling
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "window { background-color: #0b0f14; color: #ffffff; border: 1px solid #62316b; }"
        "button { background-color: #1e2429; color: #62316b; border: none; }"
        "button:hover { background-color: #2a323a; }"
        "button:active { background-color: #2a323a; }"
        "#tool-item { background-color: transparent; border: none; }"
        "#tool-item:hover { background-color: rgba(30, 36, 41, 0.6); border-radius: 8px; }"
        "label { color: #ffffff; }"
        "scrolledwindow { border: none; background-color: #0b0f14; }"
        "scrollbar { background-color: #1e2429; }"
        "scrollbar slider { background-color: #62316b; border-radius: 4px; min-width: 8px; min-height: 8px; }"
        "scrollbar slider:hover { background-color: #7a3b8b; }"
        "scrollbar slider:active { background-color: #9a4bab; }"
        "scrollbar trough { background-color: #2a323a; border-radius: 4px; }"
        "flowbox { background-color: #0b0f14; }"
        "flowboxchild { background-color: transparent; }",
        -1, NULL);
        
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
    
    gtk_widget_show_all(window);
    
    // Start slide‑in animation after window is visible
    start_slide_animation(GTK_WINDOW(window), target_x, target_y, -target_height);
    
    // Clean up merged tools
    for (int i = 0; i < total_tools; i++) {
        g_free(all_tools[i].name);
        g_free(all_tools[i].icon);
    }
    g_free(all_tools);
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
        
        XWindowAttributes attrs;
        if (XGetWindowAttributes(dpy, win, &attrs) != 0) {
            if (attrs.map_state != IsUnmapped) {
                return win;
            }
        }
        
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

/**
 * Application entry point.
 *
 * Performs single-instance check: if another instance is already running,
 * raises it and exits. Otherwise, initializes GTK application and runs
 * the main loop.
 */
int main(int argc, char **argv) 
{
    Display *dpy = XOpenDisplay(NULL);
    if (dpy) {
        Window existing = find_existing_instance(dpy);
        if (existing != None) {
            raise_window(dpy, existing);
            XCloseDisplay(dpy);
            return 0;
        }
        XCloseDisplay(dpy);
    }
    
    GtkApplication *app = gtk_application_new("org.blackline.tools", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}