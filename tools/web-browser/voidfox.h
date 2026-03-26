/* voidfox.h */
#ifndef VOIDFOX_H
#define VOIDFOX_H

#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

/* Resize edge constants - must match window_resize.h */
#define RESIZE_NONE 0
#define RESIZE_LEFT 1
#define RESIZE_RIGHT 2
#define RESIZE_TOP 4
#define RESIZE_BOTTOM 8
#define RESIZE_TOP_LEFT 5
#define RESIZE_TOP_RIGHT 6
#define RESIZE_BOTTOM_LEFT 9
#define RESIZE_BOTTOM_RIGHT 10

/**
 * BrowserWindow structure.
 * Encapsulates all UI components and state for the main browser window.
 */
typedef struct {
    GtkWidget *window;           /* Main application window */
    GtkWidget *notebook;         /* Tab container for multiple pages */
    GtkWidget *url_entry;        /* Entry widget for URL/location bar */
    GtkWidget *back_button;      /* Button to navigate backward in history */
    GtkWidget *forward_button;   /* Button to navigate forward in history */
    GtkWidget *reload_button;    /* Button to reload current page */
    GtkWidget *stop_button;      /* Button to stop page loading */
    GtkWidget *app_menu_button;  /* Button to open application menu */
    GtkWidget *bookmarks_button; /* Button to open bookmarks menu */
    GtkWidget *home_button;      /* Button to navigate to home page */
    GtkWidget *title_bar;        /* Custom title bar widget */
    GtkWidget *bookmarks_bar;    /* Bookmarks bar widget */

    /* Window manipulation state */
    int is_dragging;             /* TRUE while user is dragging the window */
    int is_resizing;             /* TRUE while user is resizing the window */
    int resize_edge;             /* Which edge/corner is being resized */
    int drag_start_x;            /* Initial mouse X position for drag operation */
    int drag_start_y;            /* Initial mouse Y position for drag operation */
} BrowserWindow;

/**
 * BrowserTab structure.
 * Represents a single browser tab with its WebView and UI elements.
 */
typedef struct {
    WebKitWebView *web_view;     /* WebKit rendering engine instance */
    GtkWidget *tab_label;        /* Label widget in tab bar */
    GtkWidget *close_button;     /* Close button for the tab */
    gboolean is_pinned;          /* TRUE if tab is pinned to the left */
    gboolean is_muted;           /* TRUE if audio is muted */
    char *url;                   /* Current URL of the tab */
    char *title;                 /* Current page title */
} BrowserTab;

/* Function declarations */

/**
 * Application activation callback.
 * Creates and displays the main browser window.
 *
 * @param app        The GtkApplication instance.
 * @param user_data  User data passed during signal connection.
 */
void voidfox_activate(GtkApplication *app, gpointer user_data);

/**
 * Creates a new browser tab.
 *
 * @param browser BrowserWindow instance.
 * @param url     URL to load in the new tab (NULL for home page).
 */
void new_tab(BrowserWindow *browser, const char *url);

/**
 * Closes a browser tab.
 *
 * @param browser   BrowserWindow instance.
 * @param tab_child The tab widget to close.
 */
void close_tab(BrowserWindow *browser, GtkWidget *tab_child);

/**
 * Loads a URL or performs a search in the current tab.
 *
 * @param browser BrowserWindow instance.
 * @param text    URL or search term.
 */
void load_url(BrowserWindow *browser, const char *text);

/**
 * Navigates back in browser history.
 *
 * @param browser BrowserWindow instance.
 */
void go_back(BrowserWindow *browser);

/**
 * Navigates forward in browser history.
 *
 * @param browser BrowserWindow instance.
 */
void go_forward(BrowserWindow *browser);

/**
 * Reloads the current page.
 *
 * @param browser BrowserWindow instance.
 */
void reload_page(BrowserWindow *browser);

/**
 * Stops page loading.
 *
 * @param browser BrowserWindow instance.
 */
void stop_loading(BrowserWindow *browser);

/**
 * Updates the state of navigation buttons based on WebKit history.
 *
 * @param browser BrowserWindow instance.
 */
void update_navigation_buttons(BrowserWindow *browser);

/**
 * Applies settings changes to the browser window.
 *
 * @param browser BrowserWindow instance.
 */
void settings_updated(BrowserWindow *browser);

/* Additional function declarations needed for tab.c */

/**
 * Applies settings to a WebKitWebView.
 *
 * @param web_view The WebKitWebView to configure.
 */
void apply_settings_to_web_view(WebKitWebView *web_view);

/**
 * Returns the absolute path to the custom home page HTML file.
 *
 * @return Pointer to static string containing the absolute path.
 */
char* get_home_page_path(void);

/* Callback functions for WebView signals */

/**
 * Callback for WebView load state changes.
 *
 * @param web_view The WebKitWebView that emitted the signal.
 * @param event    Load event type.
 * @param browser  BrowserWindow instance.
 */
void on_load_changed(WebKitWebView *web_view, WebKitLoadEvent event, BrowserWindow *browser);

/**
 * Callback for WebView title changes.
 *
 * @param web_view The WebKitWebView that emitted the signal.
 * @param pspec    GParamSpec for the changed property.
 * @param tab      BrowserTab instance.
 */
void on_title_changed(WebKitWebView *web_view, GParamSpec *pspec, BrowserTab *tab);

/**
 * Callback for tab close button click.
 *
 * @param button  The close button that was clicked.
 * @param browser BrowserWindow instance.
 */
gboolean on_close_tab_clicked(GtkButton *button, BrowserWindow *browser);

/**
 * Permission request handler for WebView.
 *
 * @param web_view The WebKitWebView that requested permission.
 * @param request  The permission request object.
 * @param browser  BrowserWindow instance.
 * @return         TRUE to indicate the request was handled.
 */
gboolean on_web_view_permission_request(WebKitWebView *web_view, WebKitPermissionRequest *request, BrowserWindow *browser);

/**
 * Policy decision handler for WebView (downloads, navigation).
 *
 * @param web_view The WebKitWebView.
 * @param decision The policy decision object.
 * @param type     Policy decision type.
 * @param browser  BrowserWindow instance.
 * @return         TRUE if handled.
 */
gboolean on_web_view_decide_policy(WebKitWebView *web_view, WebKitPolicyDecision *decision, WebKitPolicyDecisionType type, BrowserWindow *browser);

/* Window resize helpers (from window_resize.h) */

/**
 * Detects which edge of the window the cursor is on.
 *
 * @param window The window to check.
 * @param x      Cursor X coordinate in absolute screen coordinates.
 * @param y      Cursor Y coordinate in absolute screen coordinates.
 * @return       RESIZE_* constant indicating the edge.
 */
int detect_resize_edge_absolute(GtkWindow *window, int x, int y);

/**
 * Updates the cursor shape based on resize edge.
 *
 * @param widget      The widget to update cursor for.
 * @param resize_edge RESIZE_* constant indicating the edge.
 */
void update_resize_cursor(GtkWidget *widget, int resize_edge);

/**
 * Applies window resize based on drag delta.
 *
 * @param window   The window to resize.
 * @param edge     RESIZE_* constant indicating which edge is being dragged.
 * @param delta_x  Change in X position since drag start.
 * @param delta_y  Change in Y position since drag start.
 * @param width    Current window width.
 * @param height   Current window height.
 */
void apply_window_resize(GtkWindow *window, int edge, int delta_x, int delta_y, int width, int height);

#endif /* VOIDFOX_H */
