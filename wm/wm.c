#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>  
#include <Imlib2.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <pwd.h>
#include <sys/select.h>
#include <sys/time.h>
#include "controls/optionals/Otab.h"


/*
 * wm.c
 * 
 * Window manager implementation
 * Core X11 event loop, client window management (add/remove), focus handling,
 * wallpaper loading via Imlib2, desktop icon management.
 *
 * This module is part of the LIDE desktop environment system.
 * See the main window manager (wm/) and session management (session/)
 * for system architecture overview.
 */

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
static Atom net_wm_name;
static Atom utf8_string;

/**
 * Desktop icon structure representing a file or folder on the desktop.
 *
 * @id            X11 window ID for the icon (None if not a separate window).
 * @name          Display name of the icon.
 * @path          Full filesystem path.
 * @x,y           Screen coordinates of the icon.
 * @width,height  Icon dimensions in pixels.
 * @is_selected   Selection state flag (1 if selected, 0 otherwise).
 * @mod_time      Last modification timestamp from stat().
 * @size          File size in bytes.
 * @next          Pointer to next icon in linked list.
 */
typedef struct DesktopIcon {
    Window id;
    char *name;
    char *path;
    int x, y;
    int width, height;
    int is_selected;
    time_t mod_time;
    off_t size;
    struct DesktopIcon *next;
} DesktopIcon;

/**
 * Client window structure for managed application windows.
 *
 * @id                X11 window ID.
 * @x,y               Current window position.
 * @width,height      Current window dimensions.
 * @is_maximized      Flag indicating if window is maximized.
 * @saved_x,saved_y   Saved position before maximize.
 * @saved_width,saved_height Saved dimensions before maximize.
 * @is_dock           Flag for dock-type windows (panels, toolbars) that
 *                    should not be focused or managed like regular windows.
 * @next              Pointer to next client in linked list.
 */
typedef struct ClientWindow {
    Window id;
    int x, y;
    int width, height;
    int is_maximized;
    int saved_x, saved_y;
    int saved_width, saved_height;
    int is_dock;  // Flag for dock windows (panel, tools container)
    struct ClientWindow *next;
} ClientWindow;

// Function prototypes
static void add_window(Window id);
static ClientWindow* find_window(Window id);
static void remove_window(Window id);
static void get_screen_size(Display *d, int screen, int *width, int *height);
static int is_dock_window(Display *d, Window win);
static void maximize_window(Display *d, Window win);
static void unmaximize_window(Display *d, Window win);
static void handle_maximize_request(Display *d, Window win, long action);
static void set_focus(Display *d, Window win);

// Wallpaper functions
static Pixmap load_wallpaper_imlib2(Display *d, int screen, Window root, const char *filename);
static Pixmap create_solid_wallpaper(Display *d, int screen, Window root);
static void load_wallpaper(Display *d, int screen, Window root);
static void set_wallpaper(Display *d, Window win);

// Desktop functions
static char* get_desktop_path(void);
static void load_desktop_icons(Display *d);
static DesktopIcon* find_icon_at(int x, int y);
static void draw_desktop(Display *d);
static void desktop_new_folder(Display *d);
static void desktop_new_file(Display *d);
static void desktop_paste(Display *d);
static void desktop_cut(DesktopIcon *icon);
static void desktop_copy(DesktopIcon *icon);
static void desktop_open_terminal(void);
static void desktop_show_files(Display *d);
static void desktop_arrange_icons(Display *d, const char *mode);
static int compare_icon_by_name(const void *a, const void *b);
static int compare_icon_by_size(const void *a, const void *b);
static int compare_icon_by_date(const void *a, const void *b);

// Desktop button handling
static void handle_desktop_button(Display *d, XButtonEvent *ev);

// Context menu callback implementations
static void context_new_folder(const char *path, const char *name);
static void context_new_file(const char *path, const char *name);
static void context_open_terminal(void);
static void context_change_background(void);
static void context_display_settings(void);
static void context_show_files(void);
static void context_open_file_manager(void);
static char* context_get_desktop_path(void);

static ClientWindow *window_list = NULL;
static DesktopIcon *desktop_icons = NULL;
static Window desktop_window = None;
static int desktop_icon_selected = -1;
static char clipboard_path[1024] = "";
static int clipboard_is_cut = 0;
static Pixmap wallpaper_pixmap = None;
static GC wallpaper_gc = None;
// Track the current wallpaper path used by the WM
static char wm_current_wallpaper[1024] = "";
static DesktopIcon *selected_icon = NULL;
static Window focused_window = None;
static Display *global_display = NULL;

/**
 * Add a window to the client list.
 *
 * @param id X11 window ID.
 *
 * Allocates a new ClientWindow structure, initializes fields to defaults,
 * and prepends it to the global window_list.
 */
static void add_window(Window id) 

{
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
    new_win->is_dock = 0;  // Default to normal window
    new_win->next = window_list;
    window_list = new_win;
}

/**
 * Find a client window by its XID.
 *
 * @param id X11 window ID to locate.
 * @return Pointer to ClientWindow if found, NULL otherwise.
 */
static ClientWindow* find_window(Window id) 

{
    ClientWindow *curr = window_list;
    while (curr) {
        if (curr->id == id) return curr;
        curr = curr->next;
    }
    return NULL;
}

/**
 * Remove a window from the client list and free its memory.
 *
 * @param id X11 window ID to remove.
 */
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

/**
 * Get the screen dimensions.
 *
 * @param d      X11 display.
 * @param screen Screen number.
 * @param width  Output parameter for screen width.
 * @param height Output parameter for screen height.
 */
static void get_screen_size(Display *d, int screen, int *width, int *height)

{
    *width = DisplayWidth(d, screen);
    *height = DisplayHeight(d, screen);
}

/**
 * Check if a window is a dock type (panel, toolbar, etc.).
 *
 * @param d   X11 display.
 * @param win X11 window ID.
 * @return 1 if window is a dock, 0 otherwise.
 *
 * Queries the _NET_WM_WINDOW_TYPE property and compares against
 * net_wm_window_type_dock atom.
 */
static int is_dock_window(Display *d, Window win) 

{
    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char *data = NULL;
    
    int result = XGetWindowProperty(d, win, net_wm_window_type, 0, 1, False, XA_ATOM,
                                    &actual_type, &actual_format, &nitems, &bytes_after,
                                    &data);
    
    if (result == Success && actual_type == XA_ATOM && nitems == 1) {
        Atom window_type = *(Atom*)data;
        XFree(data);
        return (window_type == net_wm_window_type_dock);
    }
    
    if (data) XFree(data);
    return 0;
}

/**
 * Maximize a window to fill the screen (with panel offset).
 *
 * @param d   X11 display.
 * @param win X11 window ID.
 *
 * Saves current geometry, then moves and resizes the window to fill the
 * screen, leaving 30 pixels at the top for the panel. Updates the
 * _NET_WM_STATE property to indicate maximized state.
 */
static void maximize_window(Display *d, Window win) 

{
    ClientWindow *w = find_window(win);
    if (!w || w->is_dock) return;  // Don't maximize dock windows
    
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

/**
 * Restore a maximized window to its saved geometry.
 *
 * @param d   X11 display.
 * @param win X11 window ID.
 *
 * Restores the saved position and dimensions, clears the _NET_WM_STATE
 * maximized property.
 */
static void unmaximize_window(Display *d, Window win) 

{
    ClientWindow *w = find_window(win);
    if (!w || w->is_dock || !w->is_maximized) return;
    
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

/**
 * Handle _NET_WM_STATE client messages for maximize requests.
 *
 * @param d      X11 display.
 * @param win    X11 window ID.
 * @param action 0=REMOVE, 1=ADD, 2=TOGGLE.
 */
static void handle_maximize_request(Display *d, Window win, long action)

{
    ClientWindow *w = find_window(win);
    if (!w || w->is_dock) return;  // Ignore for dock windows
    
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

/**
 * Set input focus to a window.
 *
 * @param d   X11 display.
 * @param win X11 window ID to focus.
 *
 * Updates the X server's input focus, tracks the focused window globally,
 * and updates the _NET_ACTIVE_WINDOW root window property.
 */
static void set_focus(Display *d, Window win)

{
    if (win == None || win == desktop_window) return;
    
    ClientWindow *w = find_window(win);
    if (w && w->is_dock) return; // Don't focus dock windows
    
    XSetInputFocus(d, win, RevertToPointerRoot, CurrentTime);
    focused_window = win;
    
    // Update _NET_ACTIVE_WINDOW property on root
    XChangeProperty(d, RootWindow(d, DefaultScreen(d)), net_active_window, XA_WINDOW, 32,
                   PropModeReplace, (unsigned char*)&win, 1);
}

// ==================== WALLPAPER FUNCTIONS ====================

/**
 * Load an image file and scale it to fit the screen using Imlib2.
 *
 * @param d        X11 display.
 * @param screen   Screen number.
 * @param root     Root window ID.
 * @param filename Path to the image file.
 * @return Pixmap containing the scaled image, or None on failure.
 */
static Pixmap load_wallpaper_imlib2(Display *d, int screen, Window root, const char *filename)

{
    int screen_width, screen_height;
    get_screen_size(d, screen, &screen_width, &screen_height);
    
    // Initialize Imlib2
    imlib_context_set_display(d);
    imlib_context_set_visual(DefaultVisual(d, screen));
    imlib_context_set_colormap(DefaultColormap(d, screen));
    
    // Load the image
    Imlib_Image *image = imlib_load_image(filename);
    if (!image) {
        fprintf(stderr, "Failed to load image: %s\n", filename);
        return None;
    }
    
    imlib_context_set_image(image);
    
    // Get original dimensions
    int orig_width = imlib_image_get_width();
    int orig_height = imlib_image_get_height();
    
    // Create a new image scaled to screen size
    Imlib_Image *scaled = imlib_create_cropped_scaled_image(0, 0, orig_width, orig_height, 
                                                              screen_width, screen_height);
    
    // Free original image
    imlib_free_image_and_decache();
    
    if (!scaled) {
        fprintf(stderr, "Failed to scale image\n");
        return None;
    }
    
    imlib_context_set_image(scaled);
    
    // Create pixmap
    Pixmap pixmap = XCreatePixmap(d, root, screen_width, screen_height, 
                                   DefaultDepth(d, screen));
    
    // Render to pixmap
    imlib_context_set_drawable(pixmap);
    imlib_render_image_on_drawable(0, 0);
    
    // Free the scaled image
    imlib_free_image_and_decache();
    
    return pixmap;
}

/**
 * Create a solid color wallpaper as fallback.
 *
 * @param d      X11 display.
 * @param screen Screen number.
 * @param root   Root window ID.
 * @return Pixmap filled with the default color (#0b0f14).
 */
static Pixmap create_solid_wallpaper(Display *d, int screen, Window root)

{
    int screen_width, screen_height;
    get_screen_size(d, screen, &screen_width, &screen_height);
    
    Pixmap pixmap = XCreatePixmap(d, root, screen_width, screen_height, 
                                   DefaultDepth(d, screen));
    
    GC gc = XCreateGC(d, pixmap, 0, NULL);
    
    // Dark green color (#0b0f14)
    XSetForeground(d, gc, 0x0b0f14);
    XFillRectangle(d, pixmap, gc, 0, 0, screen_width, screen_height);
    
    XFreeGC(d, gc);
    
    return pixmap;
}

/**
 * Load the wallpaper from disk or create a solid color fallback.
 *
 * @param d      X11 display.
 * @param screen Screen number.
 * @param root   Root window ID.
 *
 * Searches multiple paths for an image file. If found, loads and scales it.
 * Otherwise creates a solid color pixmap. Stores the result in the global
 * wallpaper_pixmap for later use.
 */
static void load_wallpaper(Display *d, int screen, Window root)

{
    char path[1024];
    int found = 0;
    
    // Try multiple paths in order of preference
    const char *search_paths[] = {
        "./images/logo.png",              // Current working directory
        "./images/wal1.png",              // Alternative wallpaper
        "images/logo.png",                // Relative from current dir
        "images/wal1.png",                // Alternative relative
    };
    
    // Try each search path
    for (int i = 0; i < sizeof(search_paths) / sizeof(search_paths[0]); i++) {
        if (access(search_paths[i], R_OK) == 0) {
            wallpaper_pixmap = load_wallpaper_imlib2(d, screen, root, search_paths[i]);
            if (wallpaper_pixmap != None) {
                printf("Loaded wallpaper from %s\n", search_paths[i]);
                found = 1;
                break;
            }
        }
    }
    
    // Try user's home directory as fallback
    if (!found) {
        char *home = getenv("HOME");
        if (home) {
            const char *home_paths[] = {
                "%s/Desktop/LIDE/images/logo.png",
                "%s/.config/LIDE/images/logo.png",
            };
            
            for (int i = 0; i < sizeof(home_paths) / sizeof(home_paths[0]); i++) {
                snprintf(path, sizeof(path), home_paths[i], home);
                if (access(path, R_OK) == 0) {
                    wallpaper_pixmap = load_wallpaper_imlib2(d, screen, root, path);
                    if (wallpaper_pixmap != None) {
                        printf("Loaded wallpaper from %s\n", path);
                        found = 1;
                        break;
                    }
                }
            }
        }
    }
    
    // If all else fails, create solid color
    if (!found) {
        wallpaper_pixmap = create_solid_wallpaper(d, screen, root);
        printf("Created solid color wallpaper\n");
    }
    
    // Create GC for copying wallpaper
    if (wallpaper_gc == None) {
        wallpaper_gc = XCreateGC(d, root, 0, NULL);
    }
}

/**
 * Apply the wallpaper to a desktop window.
 *
 * @param d   X11 display.
 * @param win Desktop window ID.
 *
 * Sets the window background pixmap and clears the window to trigger redraw.
 */
static void set_wallpaper(Display *d, Window win)

{
    if (wallpaper_pixmap == None) return;
    
    int screen = DefaultScreen(d);
    int width, height;
    get_screen_size(d, screen, &width, &height);
    
    // Clear window and set background
    XSetWindowBackgroundPixmap(d, win, wallpaper_pixmap);
    XClearWindow(d, win);
    XFlush(d);
}

/**
 * Check the config file for a wallpaper path and reload if changed.
 * Reads ~/.config/blackline/current_wallpaper.conf and updates
 * `wallpaper_pixmap` when the value differs from `wm_current_wallpaper`.
 */
static void wm_reload_wallpaper_if_changed(Display *d, int screen, Window root)
{
    char config_path[1024];
    const char *home = getenv("HOME");
    if (!home) home = "/root";
    snprintf(config_path, sizeof(config_path), "%s/.config/blackline/current_wallpaper.conf", home);

    FILE *f = fopen(config_path, "r");
    if (!f) {
        return;
    }

    char path[1024] = "";
    if (fgets(path, sizeof(path), f) != NULL) {
        size_t len = strlen(path);
        if (len > 0 && path[len-1] == '\n') path[len-1] = '\0';
    }
    fclose(f);

    if (path[0] == '\0') return;

    if (strcmp(path, wm_current_wallpaper) == 0) return;

    // Free previous pixmap
    if (wallpaper_pixmap != None) {
        XFreePixmap(d, wallpaper_pixmap);
        wallpaper_pixmap = None;
    }

    // Try to load the new wallpaper image
    Pixmap new_pm = load_wallpaper_imlib2(d, screen, root, path);
    if (new_pm == None) {
        // Fallback to solid wallpaper
        new_pm = create_solid_wallpaper(d, screen, root);
    }

    wallpaper_pixmap = new_pm;
    strncpy(wm_current_wallpaper, path, sizeof(wm_current_wallpaper) - 1);
    wm_current_wallpaper[sizeof(wm_current_wallpaper)-1] = '\0';

    // Apply immediately
    set_wallpaper(d, desktop_window);
}

// ==================== DESKTOP FUNCTIONS ====================

/**
 * Get the user's desktop directory path.
 *
 * @return Newly allocated string containing the desktop path.
 *         Caller must free with free().
 */
static char* get_desktop_path(void)

{
    char *path = malloc(1024);
    const char *home = getenv("HOME");
    if (!home) {
        struct passwd *pw = getpwuid(getuid());
        home = pw->pw_dir;
    }
    snprintf(path, 1024, "%s/Desktop", home);
    return path;
}

/**
 * Scan the desktop directory and build the icons list.
 *
 * @param d X11 display (unused, kept for API consistency).
 *
 * Frees existing icons, reads all files in ~/Desktop (excluding hidden files),
 * and creates DesktopIcon structures with default positions in a grid layout.
 */
static void load_desktop_icons(Display *d)

{
    (void)d; // Suppress unused warning
    
    // Free existing icons
    DesktopIcon *curr = desktop_icons;
    while (curr) {
        DesktopIcon *next = curr->next;
        if (curr->name) free(curr->name);
        if (curr->path) free(curr->path);
        free(curr);
        curr = next;
    }
    desktop_icons = NULL;
    
    char *desktop_path = get_desktop_path();
    DIR *dir = opendir(desktop_path);
    if (!dir) {
        free(desktop_path);
        return;
    }
    
    struct dirent *entry;
    int icon_x = 50, icon_y = 50;
    
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue; // Skip hidden files
        
        char full_path[1024];
        snprintf(full_path, 1024, "%s/%s", desktop_path, entry->d_name);
        
        struct stat st;
        if (stat(full_path, &st) != 0) continue;
        
        DesktopIcon *icon = malloc(sizeof(DesktopIcon));
        icon->id = None;
        icon->name = strdup(entry->d_name);
        icon->path = strdup(full_path);
        icon->x = icon_x;
        icon->y = icon_y;
        icon->width = 64;
        icon->height = 64;
        icon->is_selected = 0;
        icon->mod_time = st.st_mtime;
        icon->size = st.st_size;
        icon->next = desktop_icons;
        desktop_icons = icon;
        
        icon_x += 100;
        if (icon_x > 800) {
            icon_x = 50;
            icon_y += 100;
        }
    }
    
    closedir(dir);
    free(desktop_path);
}

/**
 * Find a desktop icon at given screen coordinates.
 *
 * @param x,y Cursor coordinates relative to desktop window.
 * @return Pointer to DesktopIcon if found, NULL otherwise.
 */
static DesktopIcon* find_icon_at(int x, int y)

{
    DesktopIcon *curr = desktop_icons;
    while (curr) {
        if (x >= curr->x && x <= curr->x + curr->width &&
            y >= curr->y && y <= curr->y + curr->height) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

/**
 * Draw the desktop (currently just sets wallpaper).
 *
 * @param d X11 display.
 */
static void draw_desktop(Display *d)

{
    if (desktop_window == None) return;
    
    // Set wallpaper
    set_wallpaper(d, desktop_window);
}

/**
 * Create a new folder on the desktop.
 *
 * @param d X11 display.
 *
 * Generates a unique "New Folder" name and creates the directory.
 * Reloads icons and refreshes the desktop display.
 */
static void desktop_new_folder(Display *d)

{
    char *desktop_path = get_desktop_path();
    char folder_path[1024];
    int count = 1;
    
    while (1) {
        snprintf(folder_path, 1024, "%s/New Folder%d", desktop_path, count);
        struct stat st;
        if (stat(folder_path, &st) != 0) break;
        count++;
    }
    
    mkdir(folder_path, 0755);
    free(desktop_path);
    
    // Reload icons
    load_desktop_icons(d);
    draw_desktop(d);
}

/**
 * Create a new empty file on the desktop.
 *
 * @param d X11 display.
 */
static void desktop_new_file(Display *d)

{
    char *desktop_path = get_desktop_path();
    char file_path[1024];
    int count = 1;
    
    while (1) {
        snprintf(file_path, 1024, "%s/New File%d.txt", desktop_path, count);
        struct stat st;
        if (stat(file_path, &st) != 0) break;
        count++;
    }
    
    FILE *f = fopen(file_path, "w");
    if (f) fclose(f);
    free(desktop_path);
    
    // Reload icons
    load_desktop_icons(d);
    draw_desktop(d);
}

/**
 * Paste a file from clipboard to the desktop.
 *
 * @param d X11 display.
 *
 * If the clipboard contains a cut operation, the file is moved.
 * If copy, the file is duplicated. The operation clears the clipboard
 * after completion for cut operations.
 */
static void desktop_paste(Display *d)

{
    if (strlen(clipboard_path) == 0) return;
    
    char *desktop_path = get_desktop_path();
    char dest_path[1024];
    
    char *filename = strrchr(clipboard_path, '/');
    if (filename) filename++;
    else filename = clipboard_path;
    
    snprintf(dest_path, 1024, "%s/%s", desktop_path, filename);
    
    if (clipboard_is_cut) {
        rename(clipboard_path, dest_path);
        clipboard_path[0] = '\0';
        clipboard_is_cut = 0;
    } else {
        // Copy file
        FILE *src = fopen(clipboard_path, "rb");
        FILE *dst = fopen(dest_path, "wb");
        if (src && dst) {
            char buffer[4096];
            size_t bytes;
            while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
                fwrite(buffer, 1, bytes, dst);
            }
            fclose(src);
            fclose(dst);
        }
    }
    
    free(desktop_path);
    load_desktop_icons(d);
    draw_desktop(d);
}

/**
 * Cut a file (copy path to clipboard with cut flag set).
 *
 * @param icon Desktop icon representing the file to cut.
 */
static void desktop_cut(DesktopIcon *icon)

{
    if (!icon) return;
    strncpy(clipboard_path, icon->path, 1023);
    clipboard_path[1023] = '\0';
    clipboard_is_cut = 1;
}

/**
 * Copy a file (copy path to clipboard without cut flag).
 *
 * @param icon Desktop icon representing the file to copy.
 */
static void desktop_copy(DesktopIcon *icon)

{
    if (!icon) return;
    strncpy(clipboard_path, icon->path, 1023);
    clipboard_path[1023] = '\0';
    clipboard_is_cut = 0;
}

/**
 * Open a terminal window in the desktop directory.
 */
static void desktop_open_terminal(void)

{
    char *desktop_path = get_desktop_path();
    
    pid_t pid = fork();
    if (pid == 0) {
        execl("./blackline-terminal", "blackline-terminal", NULL);
        exit(0);
    }
    
    free(desktop_path);
}

/**
 * Refresh the desktop display by reloading icons and redrawing.
 *
 * @param d X11 display.
 */
static void desktop_show_files(Display *d)

{
    load_desktop_icons(d);
    draw_desktop(d);
}

/**
 * Compare two icons by name for qsort.
 *
 * @param a,b Pointers to DesktopIcon pointers.
 * @return Negative if a < b, zero if equal, positive if a > b.
 */
static int compare_icon_by_name(const void *a, const void *b)

{
    DesktopIcon *ia = *(DesktopIcon**)a;
    DesktopIcon *ib = *(DesktopIcon**)b;
    return strcmp(ia->name, ib->name);
}

/**
 * Compare two icons by file size for qsort.
 *
 * @param a,b Pointers to DesktopIcon pointers.
 * @return Negative if a < b, zero if equal, positive if a > b.
 */
static int compare_icon_by_size(const void *a, const void *b)

{
    DesktopIcon *ia = *(DesktopIcon**)a;
    DesktopIcon *ib = *(DesktopIcon**)b;
    if (ia->size < ib->size) return -1;
    if (ia->size > ib->size) return 1;
    return 0;
}

/**
 * Compare two icons by modification date for qsort.
 *
 * @param a,b Pointers to DesktopIcon pointers.
 * @return Negative if a < b, zero if equal, positive if a > b.
 */
static int compare_icon_by_date(const void *a, const void *b)

{
    DesktopIcon *ia = *(DesktopIcon**)a;
    DesktopIcon *ib = *(DesktopIcon**)b;
    if (ia->mod_time < ib->mod_time) return -1;
    if (ia->mod_time > ib->mod_time) return 1;
    return 0;
}

/**
 * Arrange desktop icons in the specified sort order.
 *
 * @param d    X11 display.
 * @param mode Sort mode: "name", "size", or "date".
 *
 * Collects all icons into an array, sorts using the appropriate comparator,
 * then reassigns positions in a grid layout.
 */
static void desktop_arrange_icons(Display *d, const char *mode)

{
    (void)d; // Suppress unused warning
    
    int count = 0;
    DesktopIcon *curr = desktop_icons;
    while (curr) {
        count++;
        curr = curr->next;
    }
    
    if (count == 0) return;
    
    DesktopIcon **array = malloc(count * sizeof(DesktopIcon*));
    curr = desktop_icons;
    for (int i = 0; i < count; i++) {
        array[i] = curr;
        curr = curr->next;
    }
    
    if (strcmp(mode, "name") == 0) {
        qsort(array, count, sizeof(DesktopIcon*), compare_icon_by_name);
    } else if (strcmp(mode, "size") == 0) {
        qsort(array, count, sizeof(DesktopIcon*), compare_icon_by_size);
    } else if (strcmp(mode, "date") == 0) {
        qsort(array, count, sizeof(DesktopIcon*), compare_icon_by_date);
    }
    
    int icon_x = 50, icon_y = 50;
    for (int i = 0; i < count; i++) {
        array[i]->x = icon_x;
        array[i]->y = icon_y;
        
        icon_x += 100;
        if (icon_x > 800) {
            icon_x = 50;
            icon_y += 100;
        }
    }
    
    free(array);
    draw_desktop(d);
}

// ==================== CONTEXT MENU CALLBACKS ====================

/**
 * Creates a new folder at the specified path.
 */
static void context_new_folder(const char *path, const char *name)
{
    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "%s/%s", path, name);
    mkdir(full_path, 0755);
    
    /* Refresh desktop if the folder was created on desktop */
    char *desktop_path = get_desktop_path();
    if (strcmp(path, desktop_path) == 0) {
        load_desktop_icons(global_display);
        draw_desktop(global_display);
    }
    free(desktop_path);
}

/**
 * Creates a new empty file at the specified path.
 */
static void context_new_file(const char *path, const char *name)
{
    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "%s/%s", path, name);
    
    FILE *f = fopen(full_path, "w");
    if (f) fclose(f);
    
    /* Refresh desktop if the file was created on desktop */
    char *desktop_path = get_desktop_path();
    if (strcmp(path, desktop_path) == 0) {
        load_desktop_icons(global_display);
        draw_desktop(global_display);
    }
    free(desktop_path);
}

/**
 * Opens a terminal window.
 */
static void context_open_terminal(void)
{
    pid_t pid = fork();
    if (pid == 0) {
        execl("./blackline-terminal", "blackline-terminal", NULL);
        exit(0);
    }
}

/**
 * Opens the wallpaper/display settings.
 */
static void context_change_background(void)
{
    pid_t pid = fork();
    if (pid == 0) {
        execl("./blackline-settings", "blackline-settings", NULL);
        exit(0);
    }
}

/**
 * Opens the display settings.
 */
static void context_display_settings(void)
{
    pid_t pid = fork();
    if (pid == 0) {
        execl("./blackline-settings", "blackline-settings", NULL);
        exit(0);
    }
}

/**
 * Refreshes the desktop file listing.
 */
static void context_show_files(void)
{
    load_desktop_icons(global_display);
    draw_desktop(global_display);
}

/**
 * Opens the file manager.
 */
static void context_open_file_manager(void)
{
    pid_t pid = fork();
    if (pid == 0) {
        execl("./blackline-fm", "blackline-fm", NULL);
        exit(0);
    }
}

/**
 * Returns the desktop path.
 */
static char* context_get_desktop_path(void)
{
    return get_desktop_path();
}

// ==================== DESKTOP BUTTON HANDLING ====================

/**
 * Handle button presses on the desktop window.
 *
 * @param d  X11 display.
 * @param ev Button event structure.
 *
 * Left-click selects an icon. Right-click shows context menu.
 */
static void handle_desktop_button(Display *d, XButtonEvent *ev)

{
    if (ev->button == Button1) {
        // Left click - select icon
        DesktopIcon *icon = find_icon_at(ev->x, ev->y);
        
        DesktopIcon *curr = desktop_icons;
        while (curr) {
            curr->is_selected = 0;
            curr = curr->next;
        }
        
        if (icon) {
            icon->is_selected = 1;
            desktop_icon_selected = 1;
            selected_icon = icon;
        } else {
            desktop_icon_selected = -1;
            selected_icon = NULL;
        }
        draw_desktop(d);
        
        // Close menu if open
        if (is_menu_active()) {
            context_menu_cleanup();
        }
    } 
    else if (ev->button == Button3) {
        // Right click - show context menu
        show_context_menu(d, ev->x_root, ev->y_root);
    }
}

/**
 * Create the desktop window that sits below all other windows.
 *
 * @param d      X11 display.
 * @param screen Screen number.
 * @param root   Root window ID.
 *
 * Loads the wallpaper, creates an override-redirect window sized to fill
 * the screen below the panel (y=30, height=screen_height-30), and maps it
 * at the bottom of the stacking order.
 */
static void create_desktop_window(Display *d, int screen, Window root)

{
    int screen_width, screen_height;
    get_screen_size(d, screen, &screen_width, &screen_height);
    
    // Load wallpaper first
    load_wallpaper(d, screen, root);
    
    // Create desktop window
    XSetWindowAttributes attr;
    attr.override_redirect = True;
    attr.background_pixmap = wallpaper_pixmap;
    attr.event_mask = ButtonPressMask | ButtonReleaseMask | 
                      ExposureMask | KeyPressMask;
    
    desktop_window = XCreateWindow(d, root, 0, 30, screen_width, screen_height - 30,
                                   0, CopyFromParent, InputOutput, CopyFromParent,
                                   CWOverrideRedirect | CWBackPixmap | CWEventMask,
                                   &attr);
    
    // Set window type to desktop
    XChangeProperty(d, desktop_window, net_wm_window_type, XA_ATOM, 32,
                   PropModeReplace, (unsigned char*)&net_wm_window_type_dock, 1);
    
    // Set window name
    XStoreName(d, desktop_window, "Desktop");
    
    XMapWindow(d, desktop_window);
    XLowerWindow(d, desktop_window); // Keep desktop at bottom
    
    // Set the wallpaper
    set_wallpaper(d, desktop_window);
}

// ==================== MAIN ====================

/**
 * Main entry point for the BlackLine Window Manager.
 *
 * Initializes X11, sets up EWMH atoms, creates the desktop window,
 * loads desktop icons, and enters the main event loop.
 *
 * Handles window management events: MapRequest, ConfigureRequest, ClientMessage,
 * ButtonPress, KeyPress, and DestroyNotify. Manages window focus, maximize state,
 * and desktop interaction.
 *
 * @return 0 on exit (never reached in normal operation).
 */
int main(void) 

{
    global_display = XOpenDisplay(NULL);
    if (!global_display) 
    {
        fprintf(stderr, "Cannot open display\n");
        return 1;
    }
    
    Display *d = global_display;
    int screen = DefaultScreen(d);
    Window root = RootWindow(d, screen);
    
   // Register context menu callbacks
   register_context_menu_callbacks(
        context_new_folder,
        context_new_file,
        context_open_terminal,
        context_change_background,
        context_display_settings,
        context_show_files,
        context_get_desktop_path
    );
    
    // Get atoms for window states
    wm_state = XInternAtom(d, "WM_STATE", False);
    wm_change_state = XInternAtom(d, "WM_CHANGE_STATE", False);
    
    // EWMH atoms
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
    net_wm_name = XInternAtom(d, "_NET_WM_NAME", False);
    utf8_string = XInternAtom(d, "UTF8_STRING", False);
    
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
    
    // Create desktop window with wallpaper
    create_desktop_window(d, screen, root);
    load_desktop_icons(d);
    
    // Create cursor
    Cursor cursor = XCreateFontCursor(d, XC_left_ptr);  
    XDefineCursor(d, root, cursor);
    
    // Select events on root
    XSelectInput(d, root, SubstructureNotifyMask | SubstructureRedirectMask | 
                        ButtonPressMask | KeyPressMask);
    
    XSync(d, False);
    
    XEvent ev;
    int xfd = ConnectionNumber(d);

    while (1)
    {
        // Wait for X events or timeout to check wallpaper config
        fd_set in;
        FD_ZERO(&in);
        FD_SET(xfd, &in);
        struct timeval tv;
        tv.tv_sec = 1; tv.tv_usec = 0; // 1s timeout

        int sel = select(xfd + 1, &in, NULL, NULL, &tv);
        if (sel > 0 && FD_ISSET(xfd, &in)) {
            XNextEvent(d, &ev);
        } else {
            // Timeout or interrupted - reload wallpaper if config changed
            wm_reload_wallpaper_if_changed(d, screen, root);
            continue;
        }

        switch(ev.type) {
            case MapRequest: {
                XWindowAttributes attr;
                XGetWindowAttributes(d, ev.xmaprequest.window, &attr);
                
                int dock = is_dock_window(d, ev.xmaprequest.window);
                
                add_window(ev.xmaprequest.window);
                ClientWindow *w = find_window(ev.xmaprequest.window);
                if (w) {
                    w->x = attr.x;
                    w->y = attr.y;
                    w->width = attr.width;
                    w->height = attr.height;
                    w->is_dock = dock;
                    
                    // Select events on the client window to track focus
                    if (!dock) {
                        XSelectInput(d, ev.xmaprequest.window, StructureNotifyMask | 
                                    PropertyChangeMask | ButtonPressMask | KeyPressMask);
                    } else {
                        XSelectInput(d, ev.xmaprequest.window, 
                                    StructureNotifyMask | PropertyChangeMask);
                    }
                }
                
                XMapWindow(d, ev.xmaprequest.window);
                
                if (!dock) {
                    XRaiseWindow(d, ev.xmaprequest.window);
                    set_focus(d, ev.xmaprequest.window);
                }
                
                XLowerWindow(d, desktop_window);
                set_wallpaper(d, desktop_window);
                
                break;
            }
            
            case UnmapNotify: {
                if (ev.xunmap.window != root) 
                {
                    // If the unmapped window was focused, clear focus
                    if (ev.xunmap.window == focused_window) {
                        focused_window = None;
                        XDeleteProperty(d, root, net_active_window);
                    }
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
                if (ev.xclient.message_type == wm_change_state) {
                    if (ev.xclient.data.l[0] == IconicState) {
                        XUnmapWindow(d, ev.xclient.window);
                    }
                }
                else if (ev.xclient.message_type == net_wm_state) {
                    long action = ev.xclient.data.l[0];
                    Atom property1 = ev.xclient.data.l[1];
                    Atom property2 = ev.xclient.data.l[2];
                    (void)property2;
                    
                    if (property1 == net_wm_state_maximized_horz ||
                        property1 == net_wm_state_maximized_vert) {
                        handle_maximize_request(d, ev.xclient.window, action);
                    }
                }
                else if (ev.xclient.message_type == net_active_window) {
                    // Application requests focus
                    set_focus(d, ev.xclient.window);
                }
                break;
            }
            
            case DestroyNotify: {
                remove_window(ev.xdestroywindow.window);
                if (ev.xdestroywindow.window == focused_window) {
                    focused_window = None;
                    XDeleteProperty(d, root, net_active_window);
                }
                break;
            }
            
            case ButtonPress: {
                // First check if click is on context menu
                if (handle_menu_button(d, &ev.xbutton)) {
                    break;  // Menu handled the event
                }
                
                if (ev.xbutton.window == desktop_window) {
                    handle_desktop_button(d, &ev.xbutton);
                } else {
                    // Click on a client window - set focus
                    ClientWindow *w = find_window(ev.xbutton.window);
                    if (w && !w->is_dock) {
                        set_focus(d, ev.xbutton.window);
                        XRaiseWindow(d, ev.xbutton.window);
                    }
                }
                break;
            }
            
            case Expose: {
                if (ev.xexpose.window == desktop_window) {
                    set_wallpaper(d, desktop_window);
                }
                break;
            }
            
            case KeyPress: {
                KeySym keysym = XLookupKeysym(&ev.xkey, 0);
                unsigned int mods = ev.xkey.state;

                /* F5: Refresh desktop files */
                if (keysym == XK_F5) {
                    desktop_show_files(d);
                }

                /* Ctrl+Shift+P: Open Command Palette */
                if (keysym == XK_p && (mods & ControlMask) && (mods & ShiftMask)) {
                    char *const argv[] = { "./blackline-command-palette", NULL };
                    pid_t pid = fork();
                    if (pid == 0) {
                        setsid();
                        execvp(argv[0], argv);
                        exit(1);
                    }
                }

                break;
            }
        }
    }
    
    // Cleanup context menu (unreachable, but here for completeness)
    context_menu_cleanup();
    
    return 0;
}