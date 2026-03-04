#include "wm.h"
#include <X11/XKBlib.h>

void grab_keys(void) 

{
    KeyCode mod4 = XKeysymToKeycode(state.display, XK_Super_L);
    if (!mod4) {
        fprintf(stderr, "Warning: Could not get Super key keycode\n");
        return;
    }
    
    KeyCode return_key = XKeysymToKeycode(state.display, XK_Return);
    
    if (return_key) 
    {
        XGrabKey(state.display, return_key, Mod4Mask, state.root, 
                True, GrabModeAsync, GrabModeAsync);
    }

    KeyCode q_key = XKeysymToKeycode(state.display, XK_q);
    
    if (q_key) {
        XGrabKey(state.display, q_key, Mod4Mask, state.root, 
                True, GrabModeAsync, GrabModeAsync);
    }

    KeyCode space_key = XKeysymToKeycode(state.display, XK_space);
    
    if (space_key) {
        XGrabKey(state.display, space_key, Mod4Mask, state.root, 
                True, GrabModeAsync, GrabModeAsync);
    }
}

static void spawn(char *const argv[]) 

{
    if (fork() == 0) {
        setsid();
        execvp(argv[0], argv);
        perror("execvp failed");
        exit(1);
    }
}

static void close_window(Window w)

{
    XEvent ev;
    ev.xclient.type = ClientMessage;
    ev.xclient.window = w;
    ev.xclient.message_type = state.wm_protocols;
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = state.wm_delete_window;
    ev.xclient.data.l[1] = CurrentTime;
    XSendEvent(state.display, w, False, NoEventMask, &ev);
}

void handle_keypress(XKeyEvent *ev) 

{
    KeySym keysym = XkbKeycodeToKeysym(state.display, ev->keycode, 0, 0);
    unsigned int mods = ev->state;

    if (!(mods & Mod4Mask)) return;

    if (keysym == XK_Return) {
        char *const argv[] = { "xterm", NULL };
        spawn(argv);
    } else if (keysym == XK_q) {
        if (state.focused) {
            close_window(state.focused);
        }
    } else if (keysym == XK_space) {
        char *const argv[] = { "./blackline-launcher", NULL };
        spawn(argv);
    }
}