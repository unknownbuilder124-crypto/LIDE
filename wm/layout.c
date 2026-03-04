#include "wm.h"

Window create_titlebar(Window client, int width) 

{
    (void)client;
    XSetWindowAttributes attr;
    attr.background_pixel = 0x0b0f14;
    attr.border_pixel = state.focused_pixel;
    attr.event_mask = ButtonPressMask | ButtonReleaseMask | ExposureMask;
    
    Window titlebar = XCreateWindow(state.display, state.root,
                                    0, 0, width, TITLEBAR_HEIGHT, 1,
                                    CopyFromParent, InputOutput, CopyFromParent,
                                    CWBackPixel | CWBorderPixel | CWEventMask, &attr);
    
    XMapWindow(state.display, titlebar);
    return titlebar;
}

void add_window(Window w) 

{
    XWindowAttributes attrs;
    XGetWindowAttributes(state.display, w, &attrs);
    
    WindowNode *node = malloc(sizeof(WindowNode));
    node->window = w;
    node->x = attrs.x;
    node->y = attrs.y;
    node->width = attrs.width;
    node->height = attrs.height;
    node->is_moving = 0;
    
    node->titlebar = create_titlebar(w, attrs.width);
    XReparentWindow(state.display, w, node->titlebar, 0, TITLEBAR_HEIGHT);
    XReparentWindow(state.display, node->titlebar, state.root, attrs.x, attrs.y);
    
    node->next = state.windows;
    state.windows = node;

    set_border(w, 0);
}

void remove_window(Window w) 

{
    WindowNode **p = &state.windows;
    while (*p) {
        if ((*p)->window == w) 
        {
            WindowNode *tmp = *p;
            *p = (*p)->next;
            XDestroyWindow(state.display, tmp->titlebar);
            free(tmp);
            break;
        }
        p = &(*p)->next;
    }

    if (state.focused == w) 
    {
        if (state.windows) {
            focus_window(state.windows->window);
        } else {
            state.focused = 0;
        }
    }
}

void focus_window(Window w) 

{
    if (state.focused == w) return;

    if (state.focused) {
        set_border(state.focused, 0);
    }

    state.focused = w;
    if (w) {
        set_border(w, 1);
        XSetInputFocus(state.display, w, RevertToPointerRoot, CurrentTime);
        XRaiseWindow(state.display, w);
        
        WindowNode *node = state.windows;
        while (node) {
            if (node->window == w) {
                XRaiseWindow(state.display, node->titlebar);
                break;
            }
            node = node->next;
        }
    }
}

void set_border(Window w, int focused)

{
    unsigned long pixel = focused ? state.focused_pixel : state.unfocused_pixel;
    XSetWindowBorder(state.display, w, pixel);
    XSetWindowBorderWidth(state.display, w, BORDER_WIDTH);
    
    WindowNode *node = state.windows;
    while (node) {
        if (node->window == w) {
            XSetWindowBorder(state.display, node->titlebar, pixel);
            break;
        }
        node = node->next;
    }
}

void handle_button_press(XButtonEvent *ev)

{
    // Find which window was clicked
    WindowNode *node = state.windows;
    while (node) {
        if (ev->window == node->titlebar) {
            // Clicked on titlebar - start moving
            node->is_moving = 1;
            node->move_start_x = ev->x_root;
            node->move_start_y = ev->y_root;
            focus_window(node->window);
            return;
        } else if (ev->window == node->window) {
            // Clicked on client window - pass through
            focus_window(node->window);
            XAllowEvents(state.display, ReplayPointer, CurrentTime);
            return;
        }
        node = node->next;
    }
    
    // If window not found, pass the event through
    XAllowEvents(state.display, ReplayPointer, CurrentTime);
}

void handle_button_release(XButtonEvent *ev)

{
    WindowNode *node = state.windows;
    while (node) {
        if (ev->window == node->titlebar) 
        {
            node->is_moving = 0;
            break;
        }
        node = node->next;
    }
}

void handle_motion_notify(XMotionEvent *ev) 

{
    WindowNode *node = state.windows;
    while (node) {
        if (node->is_moving)
         {
            int dx = ev->x_root - node->move_start_x;
            int dy = ev->y_root - node->move_start_y;
            
            node->x += dx;
            node->y += dy;
            
            XMoveWindow(state.display, node->titlebar, node->x, node->y);
            
            node->move_start_x = ev->x_root;
            node->move_start_y = ev->y_root;
            break;
        }
        node = node->next;
    }
}

void handle_configure_request(XConfigureRequestEvent *ev) 

{
    XWindowChanges changes;
    changes.x = ev->x;
    changes.y = ev->y;
    changes.width = ev->width;
    changes.height = ev->height;
    changes.border_width = ev->border_width;
    changes.sibling = ev->above;
    changes.stack_mode = ev->detail;
    
    WindowNode *node = state.windows;
    while (node) {
        if (node->window == ev->window)
        {
            node->x = ev->x;
            node->y = ev->y;
            node->width = ev->width;
            node->height = ev->height;
            XMoveResizeWindow(state.display, node->titlebar, ev->x, ev->y, 
                             ev->width, TITLEBAR_HEIGHT);
            break;
        }
        node = node->next;
    }
    
    XConfigureWindow(state.display, ev->window, ev->value_mask, &changes);
}

void update_borders(void) 

{
    WindowNode *n = state.windows;
    while (n) {
        set_border(n->window, n->window == state.focused);
        n = n->next;
    }
}