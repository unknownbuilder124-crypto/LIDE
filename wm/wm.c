#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>  
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Atoms for window states
static Atom wm_state;
static Atom wm_change_state;
static Atom net_wm_state;
static Atom net_wm_state_maximized_horz;
static Atom net_wm_state_maximized_vert;
static Atom net_wm_state_fullscreen;
static Atom net_wm_state_above;
static Atom net_active_window;
static Atom net_supported;
static Atom net_wm_window_type;
static Atom net_wm_window_type_dock;
static Atom net_wm_window_type_normal;

// Window list structure 
typedef struct ClientWindow {
    Window id;
    int x, y;
    int width, height;
    int is_maximized;
    int saved_x, saved_y;
    int saved_width, saved_height;
    struct ClientWindow *next;
} ClientWindow;

static ClientWindow *window_list = NULL;

// Add window to list
static void add_window(Window id) {
    ClientWindow *new_win = malloc(sizeof(ClientWindow));
    new_win->id = id;
    new_win->x = 0;
    new_win->y = 0;
    new_win->width = 0;
    new_win->height = 0;
    new_win->is_maximized = 0;
    new_win->saved_x = 0;
    new_win->saved_y = 0;
    new_win->saved_width = 0;
    new_win->saved_height = 0;
    new_win->next = window_list;
    window_list = new_win;
}

// Find window in list
static ClientWindow* find_window(Window id) 

{
    ClientWindow *curr = window_list;
    while (curr) {
        if (curr->id == id) return curr;
        curr = curr->next;
    }
    return NULL;
}

// Remove window from list
static void remove_window(Window id) 

{
    ClientWindow **curr = &window_list;
    while (*curr) {
        if ((*curr)->id == id) {
            ClientWindow *tmp = *curr;
            *curr = (*curr)->next;
            free(tmp);
            return;
        }
        curr = &(*curr)->next;
    }
}

// Get screen dimensions
static void get_screen_size(Display *d, int screen, int *width, int *height)

{
    *width = DisplayWidth(d, screen);
    *height = DisplayHeight(d, screen);
}

// Maximize window
static void maximize_window(Display *d, Window win) 

{
    ClientWindow *w = find_window(win);
    if (!w) return;
    
    // Save current geometry
    if (!w->is_maximized) {
        XWindowAttributes attr;
        XGetWindowAttributes(d, win, &attr);
        w->saved_x = attr.x;
        w->saved_y = attr.y;
        w->saved_width = attr.width;
        w->saved_height = attr.height;
        
        // Get screen size
        int screen_width, screen_height;
        get_screen_size(d, DefaultScreen(d), &screen_width, &screen_height);
        
        // Maximize 
        w->x = 0;
        w->y = 30; // Leave space for panel
        w->width = screen_width;
        w->height = screen_height - 30;
        w->is_maximized = 1;
        
        // Move and resize
        XMoveResizeWindow(d, win, w->x, w->y, w->width, w->height);
        
        // Update WM_STATE to indicate maximized
        Atom max_states[2];
        max_states[0] = net_wm_state_maximized_horz;
        max_states[1] = net_wm_state_maximized_vert;
        XChangeProperty(d, win, net_wm_state, XA_ATOM, 32,
                       PropModeReplace, (unsigned char*)max_states, 2);
    }
}

// Unmaximize window
static void unmaximize_window(Display *d, Window win) 

{
    ClientWindow *w = find_window(win);
    if (!w || !w->is_maximized) return;
    
    // Restore saved geometry
    w->x = w->saved_x;
    w->y = w->saved_y;
    w->width = w->saved_width;
    w->height = w->saved_height;
    w->is_maximized = 0;
    
    // Move and resize
    XMoveResizeWindow(d, win, w->x, w->y, w->width, w->height);
    
    // Remove maximized state from properties
    XDeleteProperty(d, win, net_wm_state);
}

// Handle maximize/unmaximize request
static void handle_maximize_request(Display *d, Window win, long action)

{
    ClientWindow *w = find_window(win);
    if (!w) return;
    
    switch(action) {
        case 0: // _NET_WM_STATE_REMOVE
            unmaximize_window(d, win);
            break;
        case 1: // _NET_WM_STATE_ADD
            maximize_window(d, win);
            break;
        case 2: // _NET_WM_STATE_TOGGLE
            if (w->is_maximized) {
                unmaximize_window(d, win);
            } else {
                maximize_window(d, win);
            }
            break;
    }
    XRaiseWindow(d, win);
}

int main(void) 

{
    Display *d = XOpenDisplay(NULL);
    if (!d) 
    {
        fprintf(stderr, "Cannot open display\n");
        return 1;
    }
    
    int screen = DefaultScreen(d);
    Window root = RootWindow(d, screen);
    
    // Get atoms for window states
    wm_state = XInternAtom(d, "WM_STATE", False);
    wm_change_state = XInternAtom(d, "WM_CHANGE_STATE", False);
    
    // EWMH atoms for modern window management
    net_wm_state = XInternAtom(d, "_NET_WM_STATE", False);
    net_wm_state_maximized_horz = XInternAtom(d, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
    net_wm_state_maximized_vert = XInternAtom(d, "_NET_WM_STATE_MAXIMIZED_VERT", False);
    net_wm_state_fullscreen = XInternAtom(d, "_NET_WM_STATE_FULLSCREEN", False);
    net_wm_state_above = XInternAtom(d, "_NET_WM_STATE_ABOVE", False);
    net_active_window = XInternAtom(d, "_NET_ACTIVE_WINDOW", False);
    net_supported = XInternAtom(d, "_NET_SUPPORTED", False);
    net_wm_window_type = XInternAtom(d, "_NET_WM_WINDOW_TYPE", False);
    net_wm_window_type_dock = XInternAtom(d, "_NET_WM_WINDOW_TYPE_DOCK", False);
    net_wm_window_type_normal = XInternAtom(d, "_NET_WM_WINDOW_TYPE_NORMAL", False);
    
    // Set supported EWMH atoms
    Atom supported_atoms[] = {
        net_wm_state,
        net_wm_state_maximized_horz,
        net_wm_state_maximized_vert,
        net_active_window,
        net_wm_window_type
    };
    
    XChangeProperty(d, root, net_supported, XA_ATOM, 32,
                   PropModeReplace, (unsigned char*)supported_atoms,
                   sizeof(supported_atoms) / sizeof(Atom));
    
    // Create and set a visible cursor
    Cursor cursor = XCreateFontCursor(d, XC_left_ptr);  
    XDefineCursor(d, root, cursor);
    
    // Select events
    XSelectInput(d, root, SubstructureNotifyMask | SubstructureRedirectMask | 
                        ButtonPressMask | KeyPressMask);
    
    XSync(d, False);
    
    XEvent ev;
    
    while (1)
    {
        XNextEvent(d, &ev);
        
        switch(ev.type) {
            case MapRequest: {
                // Get window attributes
                XWindowAttributes attr;
                XGetWindowAttributes(d, ev.xmaprequest.window, &attr);
                
                // Add to window list
                add_window(ev.xmaprequest.window);
                ClientWindow *w = find_window(ev.xmaprequest.window);
                if (w) {
                    w->x = attr.x;
                    w->y = attr.y;
                    w->width = attr.width;
                    w->height = attr.height;
                }
                
                // Map the window
                XMapWindow(d, ev.xmaprequest.window);
                XRaiseWindow(d, ev.xmaprequest.window);
                
                // Set active window
                XChangeProperty(d, root, net_active_window, XA_WINDOW, 32,
                               PropModeReplace, (unsigned char*)&ev.xmaprequest.window, 1);
                break;
            }
            
            case UnmapNotify: {
                if (ev.xunmap.window != root) 
                {
                    // Nothing to do, just let it be
                }
                break;
            }
            
            case ConfigureRequest: {
                XConfigureRequestEvent *e = &ev.xconfigurerequest;
                XWindowChanges changes;
                changes.x = e->x;
                changes.y = e->y;
                changes.width = e->width;
                changes.height = e->height;
                changes.border_width = e->border_width;
                changes.sibling = e->above;
                changes.stack_mode = e->detail;
                XConfigureWindow(d, e->window, e->value_mask, &changes);
                
                // Update window list
                ClientWindow *w = find_window(e->window);
                if (w && !w->is_maximized) {
                    if (e->value_mask & CWX) w->x = e->x;
                    if (e->value_mask & CWY) w->y = e->y;
                    if (e->value_mask & CWWidth) w->width = e->width;
                    if (e->value_mask & CWHeight) w->height = e->height;
                }
                break;
            }
            
            case ClientMessage: {
                // Handle client messages 
                if (ev.xclient.message_type == wm_change_state) {
                    if (ev.xclient.data.l[0] == IconicState) {
                        // Window wants to be minimized
                        XUnmapWindow(d, ev.xclient.window);
                    }
                }
                // Handle EWMH state changes 
                else if (ev.xclient.message_type == net_wm_state) {
                    long action = ev.xclient.data.l[0];
                    Atom property1 = ev.xclient.data.l[1];
                    Atom property2 = ev.xclient.data.l[2];
                    (void)property2; // Suppress unused warning
                    
                    // Check if this is a maximize request
                    if (property1 == net_wm_state_maximized_horz ||
                        property1 == net_wm_state_maximized_vert) {
                        handle_maximize_request(d, ev.xclient.window, action);
                    }
                }
                break;
            }
            
            case DestroyNotify: {
                // Window destroyed, remove from list
                remove_window(ev.xdestroywindow.window);
                break;
            }
        }
    }
    
    return 0;
}