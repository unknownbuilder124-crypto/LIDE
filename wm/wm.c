#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>  // Added for IconicState
#include <X11/cursorfont.h>  
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Atoms for window states
static Atom wm_state;
static Atom wm_change_state;

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
                // Map the window
                XMapWindow(d, ev.xmaprequest.window);
                XRaiseWindow(d, ev.xmaprequest.window);
                break;
            }
            
            case UnmapNotify: {
                // Window was unmapped (minimized)
                // Nothing to do, just let it be unmapped
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
                break;
            }
            
            case ClientMessage: {
                // Handle client messages (including minimize requests)
                if (ev.xclient.message_type == wm_change_state) {
                    if (ev.xclient.data.l[0] == IconicState) {
                        // Window wants to be minimized
                        XUnmapWindow(d, ev.xclient.window);
                    }
                }
                break;
            }
            
            case DestroyNotify:
                // Window destroyed, nothing to do
                break;
        }
    }
    
    return 0;
}