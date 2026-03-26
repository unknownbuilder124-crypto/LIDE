#include "voidfox.h"
#include "app_menu.h"
#include "bookmarks.h"
#include "history.h"
#include "settings.h"
#include "window_resize.h"
#include <ctype.h>
#include <stdio.h>
#include <libgen.h> // for dirname
#include <limits.h> // for PATH_MAX
#include <unistd.h> // for readlink, access
#include <stdlib.h> // for realpath

static void on_close_clicked(GtkButton *button, gpointer window);


/* Define PATH_MAX if not defined (for systems that don't have it) */
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

/* Debug macro - comment out to disable debug output */
#define DEBUG 1
#if DEBUG
    #define DEBUG_PRINT(fmt, ...) printf("VoidFox [%s:%d]: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
    #define DEBUG_PRINT(fmt, ...)
#endif

/**
 * Gets the absolute path to the custom home page HTML file.
 * Searches multiple locations: executable directory, current directory,
 * and project structure. Falls back to a default path.
 *
 * @return Pointer to static string containing the absolute path.
 *         Valid until next call to this function.
 */
char* get_home_page_path(void)

{
    static char path[PATH_MAX];
    char exe_path[PATH_MAX];
    char *exe_dir;
    char *resolved_path;
    
    /* Clear the path buffer */
    memset(path, 0, sizeof(path));
    
    /* Try to get the path to the executable */
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    
    if (len != -1) {
        exe_path[len] = '\0';
        
        /* Get the directory containing the executable */
        char *exe_copy = g_strdup(exe_path);
        exe_dir = dirname(exe_copy);
        
        /* First try: same directory as executable */
        snprintf(path, sizeof(path) - 1, "%s/homePage.html", exe_dir);
        g_free(exe_copy);
        
        if (access(path, F_OK) == 0) {
            DEBUG_PRINT("Home page found at: %s", path);
            return path;
        }
        
        /* Second try: in tools/web-browser subdirectory (if executable is in parent) */
        snprintf(path, sizeof(path) - 1, "%s/tools/web-browser/homePage.html", exe_dir);
        resolved_path = realpath(path, NULL);
        if (resolved_path) {
            if (access(resolved_path, F_OK) == 0) {
                strncpy(path, resolved_path, sizeof(path) - 1);
                free(resolved_path);
                DEBUG_PRINT("Home page found at: %s", path);
                return path;
            }
            free(resolved_path);
        }
    }
    
    /* Try current directory */
    snprintf(path, sizeof(path) - 1, "./homePage.html");
    if (access(path, F_OK) == 0) {
        resolved_path = realpath(path, NULL);
        if (resolved_path) {
            strncpy(path, resolved_path, sizeof(path) - 1);
            free(resolved_path);
            DEBUG_PRINT("Home page found at: %s", path);
            return path;
        }
        DEBUG_PRINT("Home page found at: %s", path);
        return path;
    }
    
    /* Try in tools/web-browser subdirectory from current directory */
    snprintf(path, sizeof(path) - 1, "./tools/web-browser/homePage.html");
    if (access(path, F_OK) == 0) {
        resolved_path = realpath(path, NULL);
        if (resolved_path) {
            strncpy(path, resolved_path, sizeof(path) - 1);
            free(resolved_path);
            DEBUG_PRINT("Home page found at: %s", path);
            return path;
        }
        DEBUG_PRINT("Home page found at: %s", path);
        return path;
    }
    
    /* Try from the executable's parent directory (for when run from project root) */
    if (len != -1) {
        char *exe_copy = g_strdup(exe_path);
        exe_dir = dirname(exe_copy);
        char *parent_dir = g_strdup(exe_dir);
        char *parent = dirname(parent_dir);
        
        snprintf(path, sizeof(path) - 1, "%s/tools/web-browser/homePage.html", parent);
        g_free(exe_copy);
        g_free(parent_dir);
        
        resolved_path = realpath(path, NULL);
        if (resolved_path) {
            if (access(resolved_path, F_OK) == 0) {
                strncpy(path, resolved_path, sizeof(path) - 1);
                free(resolved_path);
                DEBUG_PRINT("Home page found at: %s", path);
                return path;
            }
            free(resolved_path);
        }
    }
    
    /* Last resort: try absolute path from home directory */
    const char *home = getenv("HOME");
    if (home) {
        snprintf(path, sizeof(path) - 1, "%s/Desktop/LIDE/tools/web-browser/homePage.html", home);
        if (access(path, F_OK) == 0) {
            DEBUG_PRINT("Home page found at: %s", path);
            return path;
        }
    }
    
    /* Fallback - return the current directory path */
    snprintf(path, sizeof(path) - 1, "./tools/web-browser/homePage.html");
    DEBUG_PRINT("Home page not found, using fallback: %s", path);
    return path;
}

/**
 * Checks if a string is a valid URL.
 * Recognizes http://, https://, ftp://, file:// protocols,
 * or strings that look like domain names (have a dot and no spaces).
 *
 * @param str String to check.
 * @return 1 if valid URL, 0 otherwise.
 */
static int is_valid_url(const char *str)

{
    if (!str || *str == '\0') return 0;
    
    /* Check if it starts with common protocols */
    if (strstr(str, "http://") == str || 
        strstr(str, "https://") == str ||
        strstr(str, "ftp://") == str ||
        strstr(str, "file://") == str) {
        return 1;
    }
    
    /* Check if it looks like a domain (has a dot and no spaces) */
    if (strchr(str, '.') && !strchr(str, ' ') && strlen(str) > 3) {
        const char *last_dot = strrchr(str, '.');
        if (last_dot && strlen(last_dot) > 2 && strlen(last_dot) < 6) {
            return 1;
        }
    }
    
    return 0;
}

/* Dragging handlers - similar to calculator implementation */

/**
 * Callback for mouse button press on browser window.
 * Initiates window dragging or resizing based on cursor position.
 *
 * @param widget The window widget.
 * @param event  Button event details.
 * @param data   BrowserWindow instance.
 * @return       TRUE to stop event propagation.
 */
gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    BrowserWindow *browser = (BrowserWindow *)data;

    if (event->button == 1) {
        /* Check if cursor is on an edge (for resizing) - use absolute coordinates */
        browser->resize_edge = detect_resize_edge_absolute(GTK_WINDOW(browser->window),
                                                           event->x_root, event->y_root);

        if (browser->resize_edge != RESIZE_NONE) {
            browser->is_resizing = 1;
        } else {
            browser->is_dragging = 1;
        }

        browser->drag_start_x = event->x_root;
        browser->drag_start_y = event->y_root;
        gtk_window_present(GTK_WINDOW(browser->window));
        return TRUE;
    }
    return FALSE;
}

/**
 * Callback for mouse button release on browser window.
 * Terminates window dragging or resizing.
 *
 * @param widget The window widget.
 * @param event  Button event details.
 * @param data   BrowserWindow instance.
 * @return       TRUE to stop event propagation.
 */
gboolean on_button_release(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    BrowserWindow *browser = (BrowserWindow *)data;
    (void)browser;

    if (event->button == 1) {
        browser->is_dragging = 0;
        browser->is_resizing = 0;
        browser->resize_edge = RESIZE_NONE;
        return TRUE;
    }
    return FALSE;
}

/**
 * Callback for mouse motion on browser window.
 * Handles window dragging, resizing, and cursor updates.
 *
 * @param widget The window widget.
 * @param event  Motion event details.
 * @param data   BrowserWindow instance.
 * @return       TRUE if event was handled.
 */
gboolean on_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer data)
{
    BrowserWindow *browser = (BrowserWindow *)data;
    int window_width, window_height;
    gtk_window_get_size(GTK_WINDOW(browser->window), &window_width, &window_height);

    /* Update cursor for resize hints using absolute coordinates */
    if (!browser->is_dragging && !browser->is_resizing) {
        int resize_edge = detect_resize_edge_absolute(GTK_WINDOW(browser->window),
                                                      event->x_root, event->y_root);
        update_resize_cursor(widget, resize_edge);
    }

    if (browser->is_resizing) {
        int delta_x = event->x_root - browser->drag_start_x;
        int delta_y = event->y_root - browser->drag_start_y;

        apply_window_resize(GTK_WINDOW(browser->window), browser->resize_edge,
                           delta_x, delta_y, window_width, window_height);

        browser->drag_start_x = event->x_root;
        browser->drag_start_y = event->y_root;
        return TRUE;
    } else if (browser->is_dragging) {
        int dx = event->x_root - browser->drag_start_x;
        int dy = event->y_root - browser->drag_start_y;

        int x, y;
        gtk_window_get_position(GTK_WINDOW(browser->window), &x, &y);
        gtk_window_move(GTK_WINDOW(browser->window), x + dx, y + dy);

        browser->drag_start_x = event->x_root;
        browser->drag_start_y = event->y_root;
        return TRUE;
    }
    return FALSE;
}

/* Window control callbacks */

/**
 * Callback for minimize button click.
 *
 * @param button The button that was clicked.
 * @param window The window to minimize.
 */
static void on_minimize_clicked(GtkButton *button, gpointer window)

{
    (void)button;
    gtk_window_iconify(GTK_WINDOW(window));
}

/**
 * Callback for maximize/restore button click.
 * Toggles between maximized and restored states.
 *
 * @param button The button that was clicked.
 * @param window The window to maximize or restore.
 */
static void on_maximize_clicked(GtkButton *button, gpointer window)

{
    (void)button;
    GtkWindow *win = GTK_WINDOW(window);
    
    if (gtk_window_is_maximized(win)) {
        gtk_window_unmaximize(win);
    } else {
        gtk_window_maximize(win);
    }
}

/**
 * Callback for window state changes.
 * Updates the maximize button label when window is maximized or restored.
 *
 * @param window The window whose state changed.
 * @param event  Window state event details.
 * @param data   Maximize button widget.
 * @return       FALSE to allow further processing.
 */
static gboolean on_window_state_changed(GtkWidget *window, GdkEventWindowState *event, gpointer data)

{
    GtkButton *max_btn = GTK_BUTTON(data);
    
    if (event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED) {
        gtk_button_set_label(max_btn, "❐"); /* Restore symbol when maximized */
        gtk_widget_set_tooltip_text(GTK_WIDGET(max_btn), "Restore");
    } else {
        gtk_button_set_label(max_btn, "□"); /* Maximize symbol when normal */
        gtk_widget_set_tooltip_text(GTK_WIDGET(max_btn), "Maximize");
    }
    
    return FALSE;
}

/**
 * Callback for close button click.
 *
 * @param button The button that was clicked.
 * @param window The window to close.
 */
static void on_close_clicked(GtkButton *button, gpointer window)

{
    (void)button;
    gtk_window_close(GTK_WINDOW(window));
}

/* Navigation callbacks */

/**
 * Callback for URL entry activation (Enter key).
 *
 * @param entry   The URL entry widget.
 * @param browser BrowserWindow instance.
 */
static void on_url_activate(GtkEntry *entry, BrowserWindow *browser) 

{   
    const char *text = gtk_entry_get_text(entry);
    if (text && *text) {
        load_url(browser, text);
    }
}

/**
 * Callback for Go button click.
 *
 * @param button  The button that was clicked.
 * @param browser BrowserWindow instance.
 */
static void on_go_clicked(GtkButton *button, BrowserWindow *browser) 

{
    const char *text = gtk_entry_get_text(GTK_ENTRY(browser->url_entry));
    if (text && *text) {
        load_url(browser, text);
    }
}

/**
 * Callback for Home button click.
 * Loads the custom home page.
 *
 * @param button  The button that was clicked.
 * @param browser BrowserWindow instance.
 */
static void on_home_clicked(GtkButton *button, BrowserWindow *browser) 

{
    /* Load the custom home page */
    char *home_path = get_home_page_path();
    char *home_uri = g_strconcat("file://", home_path, NULL);
    DEBUG_PRINT("Loading home page: %s", home_uri);
    load_url(browser, home_uri);
    g_free(home_uri);
}

/**
 * Callback for Search button click.
 *
 * @param button  The button that was clicked.
 * @param browser BrowserWindow instance.
 */
static void on_search_clicked(GtkButton *button, BrowserWindow *browser) 

{
    const char *text = gtk_entry_get_text(GTK_ENTRY(browser->url_entry));
    if (text && *text) {
        load_url(browser, text); /* load_url will handle search via settings */
    }
}

/**
 * Callback for New Tab button click.
 *
 * @param button  The button that was clicked.
 * @param browser BrowserWindow instance.
 */
static void on_new_tab_clicked(GtkButton *button, BrowserWindow *browser) 

{
    /* Load custom home page in new tab */
    char *home_path = get_home_page_path();
    char *home_uri = g_strconcat("file://", home_path, NULL);
    DEBUG_PRINT("Opening new tab with home page: %s", home_uri);
    new_tab(browser, home_uri);
    g_free(home_uri);
}

/**
 * Callback for tab switch event.
 * Updates URL bar and navigation buttons for the newly selected tab.
 *
 * @param notebook  The GtkNotebook.
 * @param page      The new active page.
 * @param page_num  Page number of the new active tab.
 * @param browser   BrowserWindow instance.
 */
static void on_tab_switched(GtkNotebook *notebook, GtkWidget *page, guint page_num, BrowserWindow *browser) 

{
    GtkWidget *tab_child = gtk_notebook_get_nth_page(notebook, page_num);
    if (!tab_child) return;
    
    BrowserTab *tab = g_object_get_data(G_OBJECT(tab_child), "browser-tab");
    if (tab && tab->web_view) {
        const char *uri = webkit_web_view_get_uri(tab->web_view);
        if (uri && *uri) {
            gtk_entry_set_text(GTK_ENTRY(browser->url_entry), uri);
        } else {
            gtk_entry_set_text(GTK_ENTRY(browser->url_entry), "");
        }
    }
    update_navigation_buttons(browser);
}

/**
 * Callback for WebView load state changes.
 * Updates URL bar, adds to history, and updates navigation buttons when page finishes loading.
 *
 * @param web_view The WebKitWebView that emitted the signal.
 * @param event    Load event type.
 * @param browser  BrowserWindow instance.
 */
void on_load_changed(WebKitWebView *web_view, WebKitLoadEvent event, BrowserWindow *browser) 

{
    if (!browser || !browser->notebook) return;
    
    if (event == WEBKIT_LOAD_FINISHED) {
        const char *uri = webkit_web_view_get_uri(web_view);
        const char *title = webkit_web_view_get_title(web_view);
        
        if (uri && *uri) {
            printf("Page loaded: %s (%s)\n", uri, title ? title : "no title");
            
            /* Add to history if enabled */
            if (settings.remember_history) {
                add_to_history(uri, title);
            }
            
            /* Update URL bar only if this is the current tab */
            int current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(browser->notebook));
            GtkWidget *current_page_widget = gtk_notebook_get_nth_page(GTK_NOTEBOOK(browser->notebook), current_page);
            if (!current_page_widget) return;
            
            BrowserTab *tab = g_object_get_data(G_OBJECT(current_page_widget), "browser-tab");
            if (tab && tab->web_view == web_view) {
                gtk_entry_set_text(GTK_ENTRY(browser->url_entry), uri);
            }
        }
        update_navigation_buttons(browser);
    }
}

/**
 * Callback for WebView title changes.
 * Updates the tab label with the page title.
 *
 * @param web_view The WebKitWebView that emitted the signal.
 * @param pspec    GParamSpec for the changed property (unused).
 * @param tab      BrowserTab instance.
 */
void on_title_changed(WebKitWebView *web_view, GParamSpec *pspec, BrowserTab *tab) 

{
    if (!tab || !tab->tab_label) return;
    
    const char *title = webkit_web_view_get_title(web_view);
    if (title && *title) {
        char *label_text = g_strdup_printf("%s  ", title);
        gtk_label_set_text(GTK_LABEL(tab->tab_label), label_text);
        g_free(label_text);
    } else {
        gtk_label_set_text(GTK_LABEL(tab->tab_label), "New Tab  ");
    }
}

/**
 * Callback for tab close button click.
 *
 * @param button  The close button that was clicked.
 * @param browser BrowserWindow instance.
 * @return        TRUE to stop event propagation.
 */
gboolean on_close_tab_clicked(GtkButton *button, BrowserWindow *browser)

{
    GtkWidget *tab_child = g_object_get_data(G_OBJECT(button), "tab-child");
    if (tab_child) {
        close_tab(browser, tab_child);
    }
    return TRUE;
}

/* ==================== SETTINGS INTEGRATION ==================== */

/**
 * Permission request handler for WebView.
 * Grants or denies permissions based on user settings.
 *
 * @param web_view The WebKitWebView that requested permission.
 * @param request  The permission request object.
 * @param browser  BrowserWindow instance.
 * @return         TRUE to indicate the request was handled.
 */
gboolean on_web_view_permission_request(WebKitWebView *web_view, WebKitPermissionRequest *request, BrowserWindow *browser) {
    (void)web_view;
    (void)browser;
    
    /* Handle different permission types */
    if (WEBKIT_IS_GEOLOCATION_PERMISSION_REQUEST(request)) {
        if (settings.location_enabled) {
            webkit_permission_request_allow(request);
        } else {
            webkit_permission_request_deny(request);
        }
    } 
    else if (WEBKIT_IS_NOTIFICATION_PERMISSION_REQUEST(request)) {
        if (settings.notifications_enabled) {
            webkit_permission_request_allow(request);
        } else {
            webkit_permission_request_deny(request);
        }
    }
    else {
        /* For all other permission types (camera, microphone, etc.) */
        if (settings.camera_enabled || settings.microphone_enabled) {
            webkit_permission_request_allow(request);
        } else {
            webkit_permission_request_deny(request);
        }
    }
    
    return TRUE;
}

/**
 * Download policy decision handler.
 * Handles file downloads and determines where to save them.
 *
 * @param web_view The WebKitWebView.
 * @param decision The policy decision object.
 * @param type     Policy decision type.
 * @param browser  BrowserWindow instance.
 * @return         TRUE if handled, FALSE to let WebKit handle it.
 */
gboolean on_web_view_decide_policy(WebKitWebView *web_view, WebKitPolicyDecision *decision, WebKitPolicyDecisionType type, BrowserWindow *browser) {
    (void)browser;
    
    if (type == WEBKIT_POLICY_DECISION_TYPE_RESPONSE) {
        WebKitResponsePolicyDecision *response = WEBKIT_RESPONSE_POLICY_DECISION(decision);
        WebKitURIRequest *request = webkit_response_policy_decision_get_request(response);
        const gchar *uri = webkit_uri_request_get_uri(request);
        
        /* Check if this is a download (based on MIME type or content disposition) */
        if (webkit_response_policy_decision_is_mime_type_supported(response)) {
            return FALSE; /* Let WebKit handle it normally */
        }
        
        /* It's a download */
        WebKitDownload *download = webkit_web_view_download_uri(web_view, uri);
        if (download) {
            /* Set download destination */
            char *download_dir = g_strdup(settings.download_dir);
            /* Expand ~ if present */
            if (download_dir[0] == '~') {
                const char *home = g_get_home_dir();
                char *expanded = g_build_filename(home, download_dir + 1, NULL);
                g_free(download_dir);
                download_dir = expanded;
            }
            
            /* Create destination URI */
            char *filename = g_path_get_basename(uri);
            char *dest_path = g_build_filename(download_dir, filename, NULL);
            char *dest_uri = g_filename_to_uri(dest_path, NULL, NULL);
            
            webkit_download_set_destination(download, dest_uri);
            
            g_free(filename);
            g_free(dest_path);
            g_free(dest_uri);
            g_free(download_dir);
            
            if (settings.ask_download_location) {
                /* TODO: Show file chooser dialog to ask user */
            }
        }
        webkit_policy_decision_ignore(decision);
        return TRUE;
    }
    return FALSE;
}

/**
 * Reapplies WebKitSettings to all existing tabs.
 * Used when settings are updated.
 *
 * @param browser BrowserWindow instance.
 */
static void reapply_settings_to_all_tabs(BrowserWindow *browser) {
    if (!browser || !browser->notebook) return;
    
    int n_pages = gtk_notebook_get_n_pages(GTK_NOTEBOOK(browser->notebook));
    for (int i = 0; i < n_pages; i++) {
        GtkWidget *tab_child = gtk_notebook_get_nth_page(GTK_NOTEBOOK(browser->notebook), i);
        if (!tab_child) continue;
        
        BrowserTab *tab = g_object_get_data(G_OBJECT(tab_child), "browser-tab");
        if (tab && tab->web_view) {
            apply_settings_to_web_view(tab->web_view);
        }
    }
}

/**
 * Public function to be called after settings are saved.
 * Updates all browser UI elements to reflect new settings.
 *
 * @param browser BrowserWindow instance.
 */
void settings_updated(BrowserWindow *browser) {
    if (!browser) return;
    
    /* Reapply settings to all tabs */
    reapply_settings_to_all_tabs(browser);
    
    /* Update UI elements based on new settings */
    if (browser->bookmarks_bar) {
        gtk_widget_set_visible(browser->bookmarks_bar, settings.show_bookmarks_bar);
    }
    if (browser->home_button) {
        gtk_widget_set_visible(browser->home_button, settings.show_home_button);
    }
    
    /* Update title bar based on use_system_title_bar */
    if (browser->title_bar) {
        gtk_widget_set_visible(browser->title_bar, !settings.use_system_title_bar);
        gtk_window_set_decorated(GTK_WINDOW(browser->window), settings.use_system_title_bar);
        
        /* Force a resize to ensure proper window state */
        int width, height;
        gtk_window_get_size(GTK_WINDOW(browser->window), &width, &height);
        gtk_window_resize(GTK_WINDOW(browser->window), width, height);
    }
    
    /* Update CSS for dark mode */
    GtkCssProvider *provider = gtk_css_provider_new();
    if (settings.dark_mode) {
        gtk_css_provider_load_from_data(provider,
            "window { background-color: #0b0f14; color: #ffffff; }\n"
            "entry { background-color: #1e2429; color: #ffffff; border: 1px solid #00ff88; }\n"
            "button { background-color: #1e2429; color: #00ff88; border: none; }\n"
            "button:hover { background-color: #2a323a; }\n"
            "notebook { background-color: #0b0f14; }\n"
            "#title-bar { background-color: #0b0f14; border-bottom: 2px solid #00ff88; }\n"
            "#bookmarks-bar { background-color: #1e2429; padding: 2px; }\n",
            -1, NULL);
    } else {
        gtk_css_provider_load_from_data(provider,
            "window { background-color: #f0f0f0; color: #000000; }\n"
            "entry { background-color: #ffffff; color: #000000; border: 1px solid #888888; }\n"
            "button { background-color: #e0e0e0; color: #000000; border: none; }\n"
            "button:hover { background-color: #d0d0d0; }\n"
            "notebook { background-color: #f0f0f0; }\n"
            "#title-bar { background-color: #e0e0e0; border-bottom: 2px solid #888888; }\n"
            "#bookmarks-bar { background-color: #d0d0d0; padding: 2px; }\n",
            -1, NULL);
    }
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
}

/* ============================================================== */

/* Browser window creation */

/**
 * Application activation callback.
 * Creates and displays the browser window.
 *
 * @param app        The GtkApplication instance.
 * @param user_data  User data (unused).
 */
void voidfox_activate(GtkApplication *app, gpointer user_data) {

    DEBUG_PRINT("voidfox_activate started");
    
    BrowserWindow *browser = g_new0(BrowserWindow, 1);
    if (!browser) {
        DEBUG_PRINT("Failed to allocate browser window");
        return;
    }
    DEBUG_PRINT("Browser window struct allocated");

    /* Load settings first */
    load_settings();

    browser->window = gtk_application_window_new(app);
    if (!browser->window) {
        DEBUG_PRINT("Failed to create window");
        g_free(browser);
        return;
    }
    DEBUG_PRINT("Window created");
    
    gtk_window_set_title(GTK_WINDOW(browser->window), "VoidFox");
    gtk_window_set_default_size(GTK_WINDOW(browser->window), 1024, 768);
    gtk_window_set_position(GTK_WINDOW(browser->window), GTK_WIN_POS_CENTER);
    gtk_window_set_decorated(GTK_WINDOW(browser->window), settings.use_system_title_bar ? TRUE : FALSE);
    gtk_window_set_resizable(GTK_WINDOW(browser->window), TRUE);
    
    /* Enable events for dragging on the window */
    gtk_widget_add_events(browser->window, GDK_BUTTON_PRESS_MASK | 
                                         GDK_BUTTON_RELEASE_MASK | 
                                         GDK_POINTER_MOTION_MASK);
    g_signal_connect(browser->window, "button-press-event", G_CALLBACK(on_button_press), browser);
    g_signal_connect(browser->window, "button-release-event", G_CALLBACK(on_button_release), browser);
    g_signal_connect(browser->window, "motion-notify-event", G_CALLBACK(on_motion_notify), browser);

    /* Main vertical box */
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(browser->window), vbox);
    DEBUG_PRINT("VBox created");

    /* Custom title bar */
    GtkWidget *title_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_name(title_bar, "title-bar");
    gtk_widget_set_size_request(title_bar, -1, 30);
    gtk_box_pack_start(GTK_BOX(vbox), title_bar, FALSE, FALSE, 0);
    browser->title_bar = title_bar;

    GtkWidget *title_label = gtk_label_new("VoidFox Web Browser");
    gtk_label_set_xalign(GTK_LABEL(title_label), 0.0);
    gtk_box_pack_start(GTK_BOX(title_bar), title_label, TRUE, TRUE, 10);

    GtkWidget *window_buttons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_pack_end(GTK_BOX(title_bar), window_buttons, FALSE, FALSE, 5);

    /* Minimize button */
    GtkWidget *min_btn = gtk_button_new_with_label("─");
    gtk_widget_set_size_request(min_btn, 30, 25);
    g_signal_connect(min_btn, "clicked", G_CALLBACK(on_minimize_clicked), browser->window);
    gtk_box_pack_start(GTK_BOX(window_buttons), min_btn, FALSE, FALSE, 0);

    /* Maximize button */
    GtkWidget *max_btn = gtk_button_new_with_label("□");
    gtk_widget_set_size_request(max_btn, 30, 25);
    g_signal_connect(max_btn, "clicked", G_CALLBACK(on_maximize_clicked), browser->window);
    g_signal_connect(browser->window, "window-state-event", G_CALLBACK(on_window_state_changed), max_btn);
    gtk_box_pack_start(GTK_BOX(window_buttons), max_btn, FALSE, FALSE, 0);

    //* Close button */
    GtkWidget *close_btn = gtk_button_new_with_label("✕");
    gtk_widget_set_size_request(close_btn, 30, 25);
    g_signal_connect(close_btn, "clicked", G_CALLBACK(on_close_clicked), browser->window);
    gtk_box_pack_start(GTK_BOX(window_buttons), close_btn, FALSE, FALSE, 0);

    /* Show/hide title bar based on settings */
    gtk_widget_set_visible(title_bar, !settings.use_system_title_bar);

    /* Separator after title bar */
    GtkWidget *sep1 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), sep1, FALSE, FALSE, 0);

    /* Toolbar */
    GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 5);
    DEBUG_PRINT("Toolbar created");

    /* Application menu button (hamburger menu) */
    GtkWidget *app_menu_button = gtk_menu_button_new();
    gtk_button_set_label(GTK_BUTTON(app_menu_button), "☰");
    gtk_widget_set_tooltip_text(app_menu_button, "Menu");
    gtk_box_pack_start(GTK_BOX(toolbar), app_menu_button, FALSE, FALSE, 0);
    browser->app_menu_button = app_menu_button;

    /* Create and attach app menu */
    GtkWidget *app_menu = create_application_menu(browser);
    gtk_menu_button_set_popup(GTK_MENU_BUTTON(app_menu_button), app_menu);

    /* Bookmarks button */
    GtkWidget *bookmarks_button = gtk_menu_button_new();
    gtk_button_set_label(GTK_BUTTON(bookmarks_button), "★");
    gtk_widget_set_tooltip_text(bookmarks_button, "Bookmarks");
    gtk_box_pack_start(GTK_BOX(toolbar), bookmarks_button, FALSE, FALSE, 0);
    browser->bookmarks_button = bookmarks_button;

    /* Load bookmarks and create menu */
    load_bookmarks();
    GtkWidget *bookmarks_menu = create_bookmarks_menu(browser);
    gtk_menu_button_set_popup(GTK_MENU_BUTTON(bookmarks_button), bookmarks_menu);

    /* Back button */
    browser->back_button = gtk_button_new_from_icon_name("go-previous", GTK_ICON_SIZE_MENU);
    if (browser->back_button) {
        g_signal_connect_swapped(browser->back_button, "clicked", G_CALLBACK(go_back), browser);
        gtk_box_pack_start(GTK_BOX(toolbar), browser->back_button, FALSE, FALSE, 0);
    }

    /* Forward button */
    browser->forward_button = gtk_button_new_from_icon_name("go-next", GTK_ICON_SIZE_MENU);
    if (browser->forward_button) {
        g_signal_connect_swapped(browser->forward_button, "clicked", G_CALLBACK(go_forward), browser);
        gtk_box_pack_start(GTK_BOX(toolbar), browser->forward_button, FALSE, FALSE, 0);
    }

    /* Reload button */
    browser->reload_button = gtk_button_new_from_icon_name("view-refresh", GTK_ICON_SIZE_MENU);
    if (browser->reload_button) {
        g_signal_connect_swapped(browser->reload_button, "clicked", G_CALLBACK(reload_page), browser);
        gtk_box_pack_start(GTK_BOX(toolbar), browser->reload_button, FALSE, FALSE, 0);
    }

    /* Stop button */
    browser->stop_button = gtk_button_new_from_icon_name("process-stop", GTK_ICON_SIZE_MENU);
    if (browser->stop_button) {
        g_signal_connect_swapped(browser->stop_button, "clicked", G_CALLBACK(stop_loading), browser);
        gtk_box_pack_start(GTK_BOX(toolbar), browser->stop_button, FALSE, FALSE, 0);
    }

    /* URL entry */
    browser->url_entry = gtk_entry_new();
    if (browser->url_entry) {
        gtk_entry_set_placeholder_text(GTK_ENTRY(browser->url_entry), "Enter URL or search term");
        g_signal_connect(browser->url_entry, "activate", G_CALLBACK(on_url_activate), browser);
        gtk_box_pack_start(GTK_BOX(toolbar), browser->url_entry, TRUE, TRUE, 0);
    }

    /* Go button */
    GtkWidget *go_button = gtk_button_new_with_label("Go");
    if (go_button) {
        g_signal_connect(go_button, "clicked", G_CALLBACK(on_go_clicked), browser);
        gtk_box_pack_start(GTK_BOX(toolbar), go_button, FALSE, FALSE, 0);
    }
    
    /* Home button (visibility controlled by settings) */
    GtkWidget *home_button = gtk_button_new_from_icon_name("go-home", GTK_ICON_SIZE_MENU);
    if (home_button) {
        g_signal_connect(home_button, "clicked", G_CALLBACK(on_home_clicked), browser);
        gtk_box_pack_start(GTK_BOX(toolbar), home_button, FALSE, FALSE, 0);
        gtk_widget_set_visible(home_button, settings.show_home_button);
        browser->home_button = home_button;
    }
    
    /* Search button */
    GtkWidget *search_button = gtk_button_new_with_label("Search");
    if (search_button) {
        g_signal_connect(search_button, "clicked", G_CALLBACK(on_search_clicked), browser);
        gtk_box_pack_start(GTK_BOX(toolbar), search_button, FALSE, FALSE, 0);
    }

    /* New tab button */
    GtkWidget *new_tab_button = gtk_button_new_from_icon_name("tab-new", GTK_ICON_SIZE_MENU);
    if (new_tab_button) {
        g_signal_connect(new_tab_button, "clicked", G_CALLBACK(on_new_tab_clicked), browser);
        gtk_box_pack_start(GTK_BOX(toolbar), new_tab_button, FALSE, FALSE, 0);
    }

    /* Separator */
    GtkWidget *sep2 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), sep2, FALSE, FALSE, 0);

    /* Bookmarks bar (optional) */
    if (settings.show_bookmarks_bar) {
        GtkWidget *bookmarks_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
        gtk_widget_set_name(bookmarks_bar, "bookmarks-bar");
        gtk_box_pack_start(GTK_BOX(vbox), bookmarks_bar, FALSE, FALSE, 0);
        browser->bookmarks_bar = bookmarks_bar;
        
        /* Add some placeholder bookmarks */
        GtkWidget *bookmark_btn = gtk_button_new_with_label("Example");
        gtk_box_pack_start(GTK_BOX(bookmarks_bar), bookmark_btn, FALSE, FALSE, 0);
        
        GtkWidget *bookmark_btn2 = gtk_button_new_with_label("Google");
        gtk_box_pack_start(GTK_BOX(bookmarks_bar), bookmark_btn2, FALSE, FALSE, 0);
    } else {
        browser->bookmarks_bar = NULL;
    }

    /* Notebook for tabs */
    browser->notebook = gtk_notebook_new();
    if (!browser->notebook) {
        DEBUG_PRINT("Failed to create notebook");
        g_free(browser);
        return;
    }
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(browser->notebook), TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), browser->notebook, TRUE, TRUE, 0);
    DEBUG_PRINT("Notebook created");

    g_signal_connect(browser->notebook, "switch-page", G_CALLBACK(on_tab_switched), browser);

    /* Apply initial CSS based on dark mode setting */
    GtkCssProvider *provider = gtk_css_provider_new();
    if (settings.dark_mode) {
        gtk_css_provider_load_from_data(provider,
            "window { background-color: #0b0f14; color: #ffffff; }\n"
            "entry { background-color: #1e2429; color: #ffffff; border: 1px solid #00ff88; }\n"
            "button { background-color: #1e2429; color: #00ff88; border: none; }\n"
            "button:hover { background-color: #2a323a; }\n"
            "notebook { background-color: #0b0f14; }\n"
            "#title-bar { background-color: #0b0f14; border-bottom: 2px solid #00ff88; }\n"
            "#bookmarks-bar { background-color: #1e2429; padding: 2px; }\n",
            -1, NULL);
    } else {
        gtk_css_provider_load_from_data(provider,
            "window { background-color: #f0f0f0; color: #000000; }\n"
            "entry { background-color: #ffffff; color: #000000; border: 1px solid #888888; }\n"
            "button { background-color: #e0e0e0; color: #000000; border: none; }\n"
            "button:hover { background-color: #d0d0d0; }\n"
            "notebook { background-color: #f0f0f0; }\n"
            "#title-bar { background-color: #e0e0e0; border-bottom: 2px solid #888888; }\n"
            "#bookmarks-bar { background-color: #d0d0d0; padding: 2px; }\n",
            -1, NULL);
    }
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);

    /* Load history at startup if enabled */
    if (settings.remember_history) {
        load_history();
    }
    
    DEBUG_PRINT("Creating first tab...");
    
    /* Determine initial URL based on startup behavior */
    char *initial_url = NULL;
    char *home_path = get_home_page_path();
    char *home_uri = g_strconcat("file://", home_path, NULL);
    
    switch (settings.startup_behavior) {
        case STARTUP_NEW_TAB_PAGE:
            initial_url = g_strdup("about:blank");
            break;
        case STARTUP_CONTINUE:
            /* TODO: Restore previous session */
            initial_url = g_strdup(home_uri);
            break;
        case STARTUP_HOME_PAGE:
            initial_url = g_strdup(home_uri);
            break;
        case STARTUP_SPECIFIC_PAGES:
            if (settings.startup_pages) {
                initial_url = g_strdup((char*)settings.startup_pages->data);
            } else {
                initial_url = g_strdup(home_uri);
            }
            break;
        default:
            initial_url = g_strdup(home_uri);
            break;
    }
    
    DEBUG_PRINT("First tab URL: %s", initial_url);
    new_tab(browser, initial_url);
    g_free(initial_url);
    g_free(home_uri);
    
    DEBUG_PRINT("First tab created");

    gtk_widget_show_all(browser->window);
    DEBUG_PRINT("Window shown");
    
    update_navigation_buttons(browser);
    DEBUG_PRINT("voidfox_activate completed");
}

/* Tab management */

/**
 * Creates a new browser tab.
 *
 * @param browser BrowserWindow instance.
 * @param url     URL to load in the new tab (NULL for home page).
 */
void new_tab(BrowserWindow *browser, const char *url) 

{
    if (!browser || !browser->notebook) {
        DEBUG_PRINT("new_tab: invalid browser or notebook");
        return;
    }
    
    DEBUG_PRINT("new_tab: creating new tab with URL: %s", url ? url : "NULL");
    
    /* Create WebView with settings */
    WebKitWebView *web_view = WEBKIT_WEB_VIEW(webkit_web_view_new());
    if (!web_view) {
        DEBUG_PRINT("Failed to create web view");
        return;
    }
    DEBUG_PRINT("WebView created");
    
    /* Apply settings to web view */
    apply_settings_to_web_view(web_view);
    
    g_signal_connect(web_view, "load-changed", G_CALLBACK(on_load_changed), browser);
    
    /* Connect permission and policy signals to the WebView */
    g_signal_connect(web_view, "permission-request", G_CALLBACK(on_web_view_permission_request), browser);
    g_signal_connect(web_view, "decide-policy", G_CALLBACK(on_web_view_decide_policy), browser);

    /* Load URL - use provided URL or custom home page */
    if (url && *url) {
        webkit_web_view_load_uri(web_view, url);
    } else {
        char *home_path = get_home_page_path();
        char *home_uri = g_strconcat("file://", home_path, NULL);
        webkit_web_view_load_uri(web_view, home_uri);
        g_free(home_uri);
    }

    /* Create tab content container */
    GtkWidget *tab_child = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    if (!tab_child) {
        DEBUG_PRINT("Failed to create tab child");
        g_object_unref(web_view);
        return;
    }
    gtk_box_pack_start(GTK_BOX(tab_child), GTK_WIDGET(web_view), TRUE, TRUE, 0);
    gtk_widget_show(tab_child);
    DEBUG_PRINT("Tab child container created");

    /* Create BrowserTab struct */
    BrowserTab *tab = g_new0(BrowserTab, 1);
    if (!tab) {
        DEBUG_PRINT("Failed to allocate tab struct");
        g_object_unref(web_view);
        return;
    }
    tab->web_view = web_view;
    tab->tab_label = gtk_label_new("Loading...");
    tab->close_button = gtk_button_new_from_icon_name("window-close", GTK_ICON_SIZE_MENU);
    
    if (tab->close_button) {
        gtk_button_set_relief(GTK_BUTTON(tab->close_button), GTK_RELIEF_NONE);
        gtk_widget_set_tooltip_text(tab->close_button, "Close tab");
    }
    DEBUG_PRINT("Tab struct created");

    /* Pack close button into a box */
    GtkWidget *tab_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    if (tab_box) {
        if (tab->tab_label) gtk_box_pack_start(GTK_BOX(tab_box), tab->tab_label, FALSE, FALSE, 0);
        if (tab->close_button) gtk_box_pack_start(GTK_BOX(tab_box), tab->close_button, FALSE, FALSE, 0);
        gtk_widget_show_all(tab_box);
    }

    /* Store tab data */
    g_object_set_data_full(G_OBJECT(tab_child), "browser-tab", tab, g_free);
    if (tab->close_button) {
        g_object_set_data_full(G_OBJECT(tab->close_button), "tab-child", tab_child, NULL);
        g_signal_connect(tab->close_button, "clicked", G_CALLBACK(on_close_tab_clicked), browser);
    }
    g_signal_connect(web_view, "notify::title", G_CALLBACK(on_title_changed), tab);

    /* Append tab */
    int page_num = gtk_notebook_append_page(GTK_NOTEBOOK(browser->notebook), tab_child, tab_box);
    gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(browser->notebook), tab_child, TRUE);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(browser->notebook), page_num);
    DEBUG_PRINT("Tab appended, page number: %d", page_num);
}

/**
 * Closes a browser tab.
 *
 * @param browser   BrowserWindow instance.
 * @param tab_child The tab widget to close.
 */
void close_tab(BrowserWindow *browser, GtkWidget *tab_child) 

{
    if (!browser || !browser->notebook || !tab_child) return;
    
    int page_num = gtk_notebook_page_num(GTK_NOTEBOOK(browser->notebook), tab_child);
    if (page_num != -1) {
        /* Don't close if it's the last tab */
        if (gtk_notebook_get_n_pages(GTK_NOTEBOOK(browser->notebook)) <= 1) {
            char *home_path = get_home_page_path();
            char *home_uri = g_strconcat("file://", home_path, NULL);
            new_tab(browser, home_uri);
            g_free(home_uri);
        }
        gtk_notebook_remove_page(GTK_NOTEBOOK(browser->notebook), page_num);
    }
}

/* Navigation functions */

/**
 * Loads a URL or performs a search in the current tab.
 *
 * @param browser BrowserWindow instance.
 * @param text    URL or search term.
 */
void load_url(BrowserWindow *browser, const char *text)

{
    if (!browser || !browser->notebook || !text || *text == '\0') return;
    
    int current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(browser->notebook));
    GtkWidget *current_page_widget = gtk_notebook_get_nth_page(GTK_NOTEBOOK(browser->notebook), current_page);
    if (!current_page_widget) return;
    
    BrowserTab *tab = g_object_get_data(G_OBJECT(current_page_widget), "browser-tab");
    if (!tab || !tab->web_view) return;

    char *full_url;
    
    if (is_valid_url(text)) {
        /* If no scheme, prepend http:// */
        if (strstr(text, "://") == NULL) {
            full_url = g_strconcat("http://", text, NULL);
        } else {
            full_url = g_strdup(text);
        }
    } else {
        /* It's a search term - use settings to construct URL */
        full_url = g_strdup(get_search_url(text));
    }

    if (full_url) {
        DEBUG_PRINT("Loading URL: %s", full_url);
        webkit_web_view_load_uri(tab->web_view, full_url);
        g_free(full_url);
    }
}

/**
 * Navigates back in browser history.
 *
 * @param browser BrowserWindow instance.
 */
void go_back(BrowserWindow *browser) 

{
    if (!browser || !browser->notebook) return;
    
    int current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(browser->notebook));
    GtkWidget *current_page_widget = gtk_notebook_get_nth_page(GTK_NOTEBOOK(browser->notebook), current_page);
    if (!current_page_widget) return;
    
    BrowserTab *tab = g_object_get_data(G_OBJECT(current_page_widget), "browser-tab");
    if (tab && tab->web_view && webkit_web_view_can_go_back(tab->web_view)) {
        webkit_web_view_go_back(tab->web_view);
    }
}

/**
 * Navigates forward in browser history.
 *
 * @param browser BrowserWindow instance.
 */
void go_forward(BrowserWindow *browser) 

{
    if (!browser || !browser->notebook) return;
    
    int current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(browser->notebook));
    GtkWidget *current_page_widget = gtk_notebook_get_nth_page(GTK_NOTEBOOK(browser->notebook), current_page);
    if (!current_page_widget) return;
    
    BrowserTab *tab = g_object_get_data(G_OBJECT(current_page_widget), "browser-tab");
    if (tab && tab->web_view && webkit_web_view_can_go_forward(tab->web_view)) {
        webkit_web_view_go_forward(tab->web_view);
    }
}

/**
 * Reloads the current page.
 *
 * @param browser BrowserWindow instance.
 */
void reload_page(BrowserWindow *browser) 

{
    if (!browser || !browser->notebook) return;
    
    int current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(browser->notebook));
    GtkWidget *current_page_widget = gtk_notebook_get_nth_page(GTK_NOTEBOOK(browser->notebook), current_page);
    if (!current_page_widget) return;
    
    BrowserTab *tab = g_object_get_data(G_OBJECT(current_page_widget), "browser-tab");
    if (tab && tab->web_view) {
        webkit_web_view_reload(tab->web_view);
    }
}

/**
 * Stops page loading.
 *
 * @param browser BrowserWindow instance.
 */
void stop_loading(BrowserWindow *browser) 

{
    if (!browser || !browser->notebook) return;
    
    int current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(browser->notebook));
    GtkWidget *current_page_widget = gtk_notebook_get_nth_page(GTK_NOTEBOOK(browser->notebook), current_page);
    if (!current_page_widget) return;
    
    BrowserTab *tab = g_object_get_data(G_OBJECT(current_page_widget), "browser-tab");
    if (tab && tab->web_view) {
        webkit_web_view_stop_loading(tab->web_view);
    }
}

/**
 * Updates the state of navigation buttons based on WebKit history.
 *
 * @param browser BrowserWindow instance.
 */
void update_navigation_buttons(BrowserWindow *browser) 

{
    if (!browser || !browser->notebook) return;
    
    int current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(browser->notebook));
    GtkWidget *current_page_widget = gtk_notebook_get_nth_page(GTK_NOTEBOOK(browser->notebook), current_page);
    if (!current_page_widget) return;
    
    BrowserTab *tab = g_object_get_data(G_OBJECT(current_page_widget), "browser-tab");
    if (tab && tab->web_view) {
        if (browser->back_button) 
            gtk_widget_set_sensitive(browser->back_button, webkit_web_view_can_go_back(tab->web_view));
        if (browser->forward_button) 
            gtk_widget_set_sensitive(browser->forward_button, webkit_web_view_can_go_forward(tab->web_view));
        if (browser->reload_button) 
            gtk_widget_set_sensitive(browser->reload_button, TRUE);
        if (browser->stop_button) 
            gtk_widget_set_sensitive(browser->stop_button, webkit_web_view_is_loading(tab->web_view));
    }
}