#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>

static Display *xdisplay = NULL;
static GList *minimized_windows = NULL;
static GtkWidget *window = NULL;
static GtkWidget *listbox = NULL;
static Window container_xid = 0; // To Store window ID

// Structure to track minimized windows
typedef struct {
    Window xid;
    char *title;
    GtkWidget *row;
} MinimizedWindow;

// Function to get window title
static char* get_window_title(Window xid) 

{
    if (!xdisplay) return NULL;
    
    Atom prop = XInternAtom(xdisplay, "WM_NAME", False);
    Atom type;
    int format;
    unsigned long nitems, bytes_after;
    unsigned char *data = NULL;
    
    if (XGetWindowProperty(xdisplay, xid, prop, 0, 1024, False, AnyPropertyType,
                          &type, &format, &nitems, &bytes_after, &data) == Success && data) {
        char *title = g_strdup((char*)data);
        XFree(data);
        return title;
    }
    return NULL; 
}

// Check if window should be ignored 
static gboolean should_ignore_window(Window xid, const char *title)

{
    // Ignore our own container
    if (xid == container_xid) {
        fprintf(stderr, "Ignoring container itself\n");
        return TRUE;
    }
    
    // Ignore windows with NULL 
    if (!title || title[0] == '\0') {
        if (!title) return TRUE;
    }
    
    // Ignore known system windows by title
    if (title) {
        if (strstr(title, "BlackLine Panel") != NULL ||
            strstr(title, "BlackLine Tools") != NULL ||
            strstr(title, "Minimized Apps") != NULL) {
            fprintf(stderr, "Ignoring system window: %s\n", title);
            return TRUE;
        }
    }
    
    return FALSE;
}

// Function to remove window from minimized list
static void remove_minimized_window(Window xid) 

{
    GList *iter;
    for (iter = minimized_windows; iter; iter = iter->next) {
        MinimizedWindow *mw = (MinimizedWindow*)iter->data;
        if (mw->xid == xid) {
            fprintf(stderr, "Removing window 0x%lx (%s) from list\n", (long)xid, mw->title);
            gtk_widget_destroy(mw->row);
            g_free(mw->title);
            g_free(mw);
            minimized_windows = g_list_delete_link(minimized_windows, iter);
            break;
        }
    }
}

// Function to restore a minimized window and close container
static void restore_window(GtkButton *button, gpointer data) 

{
    (void)button;
    Window xid = (Window)GPOINTER_TO_INT(data);
    
    if (xdisplay) {
        fprintf(stderr, "Restoring window 0x%lx\n", (long)xid);
        XMapWindow(xdisplay, xid);
        XRaiseWindow(xdisplay, xid);
        XFlush(xdisplay);
        
        // Auto-close the minimized container after restoring
        if (window) {
            gtk_widget_hide(window);
            fprintf(stderr, "Auto-hiding container after restore\n");
        }
    }
}

// Function to add a window to minimized list
static void add_minimized_window(Window xid) 

{
    fprintf(stderr, "Attempting to add window 0x%lx\n", (long)xid);
    
    // Get title first to check if we should ignore
    char *title = get_window_title(xid);
    
    // Check if window should be ignored
    if (should_ignore_window(xid, title)) {
        g_free(title);
        return;
    }
    
    // If we couldn't get a title, maybe still add? But likely not useful.
    if (!title) {
        fprintf(stderr, "No title for 0x%lx, ignoring\n", (long)xid);
        return;
    }
    
    // Check if already in list
    GList *iter;
    for (iter = minimized_windows; iter; iter = iter->next) {
        MinimizedWindow *mw = (MinimizedWindow*)iter->data;
        if (mw->xid == xid) {
            fprintf(stderr, "Window 0x%lx already in list\n", (long)xid);
            g_free(title);
            return;
        }
    }
    
    // Create row for listbox
    GtkWidget *row = gtk_list_box_row_new();
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_container_add(GTK_CONTAINER(row), hbox);
    
    // Add icon
    GtkWidget *icon = gtk_label_new("📌");
    gtk_box_pack_start(GTK_BOX(hbox), icon, FALSE, FALSE, 5);
    
    // Add title
    GtkWidget *label = gtk_label_new(title);
    gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 5);
    
    // Add restore button
    GtkWidget *restore_btn = gtk_button_new_with_label("↗");
    gtk_widget_set_size_request(restore_btn, 30, 25);
    g_signal_connect(restore_btn, "clicked", G_CALLBACK(restore_window), GINT_TO_POINTER((int)xid));
    gtk_box_pack_end(GTK_BOX(hbox), restore_btn, FALSE, FALSE, 5);
    
    gtk_list_box_insert(GTK_LIST_BOX(listbox), row, -1);
    gtk_widget_show_all(row);
    
    // Store in list
    MinimizedWindow *mw = g_new(MinimizedWindow, 1);
    mw->xid = xid;
    mw->title = title; // Transfer ownership
    mw->row = row;
    
    minimized_windows = g_list_append(minimized_windows, mw);
    fprintf(stderr, "Added window 0x%lx (%s) to list\n", (long)xid, title);
    // if (window) {
    //     gtk_window_present(GTK_WINDOW(window));
    //     fprintf(stderr, "Forced container to show\n");
    // }
}

// X11 event handler 
static gboolean x11_event_watch(GIOChannel *source, GIOCondition condition, gpointer data) 

{
    (void)source;
    (void)condition;
    (void)data;
    
    if (!xdisplay) return TRUE;
    
    XEvent ev;
    while (XPending(xdisplay)) {
        XNextEvent(xdisplay, &ev);
        
        switch (ev.type) {
            case UnmapNotify: {
                Window xid = ev.xunmap.window;
                fprintf(stderr, "UnmapNotify for window 0x%lx\n", (long)xid);
                add_minimized_window(xid);
                break;
            }
            case MapNotify: {
                Window xid = ev.xmap.window;
                fprintf(stderr, "MapNotify for window 0x%lx\n", (long)xid);
                remove_minimized_window(xid);
                break;
            }
            case DestroyNotify: {
                Window xid = ev.xdestroywindow.window;
                fprintf(stderr, "DestroyNotify for window 0x%lx\n", (long)xid);
                remove_minimized_window(xid);
                break;
            }
            default:
                break;
        }
    }
    return TRUE;
}

// Timeout fallback to poll X events
static gboolean timeout_check_xevents(gpointer data) 

{
    (void)data;
    if (xdisplay) {
        while (XPending(xdisplay) > 0) {
            XEvent ev;
            XNextEvent(xdisplay, &ev);
            
            switch (ev.type) {
                case UnmapNotify: {
                    Window xid = ev.xunmap.window;
                    fprintf(stderr, "Timeout UnmapNotify for window 0x%lx\n", (long)xid);
                    add_minimized_window(xid);
                    break;
                }
                case MapNotify: {
                    Window xid = ev.xmap.window;
                    fprintf(stderr, "Timeout MapNotify for window 0x%lx\n", (long)xid);
                    remove_minimized_window(xid);
                    break;
                }
                case DestroyNotify: {
                    Window xid = ev.xdestroywindow.window;
                    fprintf(stderr, "Timeout DestroyNotify for window 0x%lx\n", (long)xid);
                    remove_minimized_window(xid);
                    break;
                }
            }
        }
    }
    return G_SOURCE_CONTINUE;
}

// Dragging variables
static int is_dragging = 0;
static int drag_start_x, drag_start_y;

static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data) 

{
    GtkWidget *win = GTK_WIDGET(data);
    
    if (event->button == 1) 
    {
        is_dragging = 1;
        drag_start_x = event->x_root;
        drag_start_y = event->y_root;
        gtk_window_present(GTK_WINDOW(win));
        return TRUE;
    }
    return FALSE;
}

static gboolean on_button_release(GtkWidget *widget, GdkEventButton *event, gpointer data) 

{
    (void)widget;
    (void)event;
    (void)data;
    is_dragging = 0;
    return FALSE;
}

static gboolean on_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer data) 

{
    GtkWidget *win = GTK_WIDGET(data);
    
    if (is_dragging) {
        int dx = event->x_root - drag_start_x;
        int dy = event->y_root - drag_start_y;
        
        int x, y;
        gtk_window_get_position(GTK_WINDOW(win), &x, &y);
        gtk_window_move(GTK_WINDOW(win), x + dx, y + dy);
        
        drag_start_x = event->x_root;
        drag_start_y = event->y_root;
        return TRUE;
    }
    return FALSE;
}

static void on_close_clicked(GtkButton *button, gpointer data) 

{
    (void)button;
    GtkWidget *win = GTK_WIDGET(data);
    gtk_widget_hide(win);
}

// Show the minimized container
static void show_minimized_container(GtkButton *button, gpointer data) 

{
    (void)button;
    (void)data;
    
    if (window) {
        if (gtk_widget_get_visible(window)) {
            gtk_widget_hide(window);
            fprintf(stderr, "Hiding container\n");
        } else {
            gtk_window_present(GTK_WINDOW(window));
            fprintf(stderr, "Showing container\n");
        }
    }
}

// Initialize the minimized container 
void minimized_container_initialize(void) 

{
    fprintf(stderr, "Initializing minimized container\n");
    
    // Open X display
    xdisplay = XOpenDisplay(NULL);
    if (!xdisplay) {
        fprintf(stderr, "Cannot open X display\n");
        return;
    }
    
    // Select events on root window to get all child events
    Window root = DefaultRootWindow(xdisplay);
    XSelectInput(xdisplay, root, SubstructureNotifyMask);
    fprintf(stderr, "Watching root window 0x%lx\n", (long)root);
    
    // Set up X event monitoring via GIO
    int x11_fd = ConnectionNumber(xdisplay);
    GIOChannel *channel = g_io_channel_unix_new(x11_fd);
    g_io_add_watch(channel, G_IO_IN, x11_event_watch, NULL);
    g_io_channel_unref(channel);
    
    // Add timeout fallback (every 100ms)
    g_timeout_add(100, timeout_check_xevents, NULL);
    
    // Create window
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Minimized Apps");
    gtk_window_set_default_size(GTK_WINDOW(window), 300, 400);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);
    
    // Get the XID of this window to ignore it later
    gtk_widget_realize(window); // Ensure the window is realized so we can get the XID
    GdkWindow *gdk_win = gtk_widget_get_window(window);
    if (gdk_win) {
        container_xid = GDK_WINDOW_XID(gdk_win);
        fprintf(stderr, "Container XID: 0x%lx\n", (long)container_xid);
    }
    
    // Log window position
    int x, y;
    gtk_window_get_position(GTK_WINDOW(window), &x, &y);
    fprintf(stderr, "Container window initial position: %d,%d\n", x, y);
    
    // Enable events for dragging
    gtk_widget_add_events(window, GDK_BUTTON_PRESS_MASK | 
                                   GDK_BUTTON_RELEASE_MASK | 
                                   GDK_POINTER_MOTION_MASK);
    
    // Connect drag signals
    g_signal_connect(window, "button-press-event", G_CALLBACK(on_button_press), window);
    g_signal_connect(window, "button-release-event", G_CALLBACK(on_button_release), window);
    g_signal_connect(window, "motion-notify-event", G_CALLBACK(on_motion_notify), window);
    
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    
    // Title bar
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<b>Minimized Apps</b>");
    gtk_box_pack_start(GTK_BOX(hbox), title, TRUE, TRUE, 0);
    
    GtkWidget *close_btn = gtk_button_new_with_label("X");
    gtk_widget_set_size_request(close_btn, 30, 25);
    g_signal_connect(close_btn, "clicked", G_CALLBACK(on_close_clicked), window);
    gtk_box_pack_end(GTK_BOX(hbox), close_btn, FALSE, FALSE, 0);
    
    // Separator
    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), sep, FALSE, FALSE, 5);
    
    // Scrolled window for list
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), 
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 5);
    
    // Listbox for minimized windows
    listbox = gtk_list_box_new();
    gtk_container_add(GTK_CONTAINER(scrolled), listbox);
    
    // Show window then hide it
    gtk_widget_show_all(window);
    gtk_widget_hide(window);
    fprintf(stderr, "Minimized container initialized and hidden\n");
}

// Get the toggle button for the panel
GtkWidget* minimized_container_get_toggle_button(void) 

{
    GtkWidget *btn = gtk_button_new_with_label("📌");
    gtk_widget_set_tooltip_text(btn, "Minimized Apps");
    g_signal_connect(btn, "clicked", G_CALLBACK(show_minimized_container), NULL);
    return btn;
}