#include "voidfox.h"
#include "app_menu.h"
#include "bookmarks.h"
#include "history.h"
#include "settings.h"
#include <ctype.h>
#include <stdio.h>

// Debug macro - comment out to disable debug output
#define DEBUG 1
#if DEBUG
    #define DEBUG_PRINT(fmt, ...) printf("VoidFox [%s:%d]: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
    #define DEBUG_PRINT(fmt, ...)
#endif

// Dragging variables
static int is_dragging = 0;
static int drag_start_x, drag_start_y;

// Helper function to check if string is a URL
static int is_valid_url(const char *str)

{
    if (!str || *str == '\0') return 0;
    
    // Check if it starts with common protocols
    if (strstr(str, "http://") == str || 
        strstr(str, "https://") == str ||
        strstr(str, "ftp://") == str ||
        strstr(str, "file://") == str) {
        return 1;
    }
    
    // Check if it looks like a domain (has a dot and no spaces)
    if (strchr(str, '.') && !strchr(str, ' ') && strlen(str) > 3) {
        const char *last_dot = strrchr(str, '.');
        if (last_dot && strlen(last_dot) > 2 && strlen(last_dot) < 6) {
            return 1;
        }
    }
    
    return 0;
}

// Dragging handlers
static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer window)

{
    if (event->button == 1)
    {
        is_dragging = 1;
        drag_start_x = event->x_root;
        drag_start_y = event->y_root;
        gtk_window_present(GTK_WINDOW(window));
        return TRUE;
    }
    return FALSE;
}

static gboolean on_button_release(GtkWidget *widget, GdkEventButton *event, gpointer window)

{
    (void)widget;
    (void)event;
    (void)window;
    is_dragging = 0;
    return FALSE;
}

static gboolean on_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer window)

{
    GtkWidget *win = GTK_WIDGET(window);

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

// Window control callbacks
static void on_minimize_clicked(GtkButton *button, gpointer window)

{
    (void)button;
    gtk_window_iconify(GTK_WINDOW(window));
}

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

// Track window state changes to update maximize button
static gboolean on_window_state_changed(GtkWidget *window, GdkEventWindowState *event, gpointer data)

{
    GtkButton *max_btn = GTK_BUTTON(data);
    
    if (event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED) {
        gtk_button_set_label(max_btn, "❐"); // Restore symbol when maximized
        gtk_widget_set_tooltip_text(GTK_WIDGET(max_btn), "Restore");
    } else {
        gtk_button_set_label(max_btn, "□"); // Maximize symbol when normal
        gtk_widget_set_tooltip_text(GTK_WIDGET(max_btn), "Maximize");
    }
    
    return FALSE;
}

static void on_close_clicked(GtkButton *button, gpointer window)

{
    (void)button;
    gtk_window_close(GTK_WINDOW(window));
}

// Callbacks
static void on_url_activate(GtkEntry *entry, BrowserWindow *browser) 

{   
    const char *text = gtk_entry_get_text(entry);
    if (text && *text) {
        load_url(browser, text);
    }
}

static void on_go_clicked(GtkButton *button, BrowserWindow *browser) 

{
    const char *text = gtk_entry_get_text(GTK_ENTRY(browser->url_entry));
    if (text && *text) {
        load_url(browser, text);
    }
}

static void on_home_clicked(GtkButton *button, BrowserWindow *browser) 

{
    load_url(browser, settings.home_page);
}

static void on_search_clicked(GtkButton *button, BrowserWindow *browser) 

{
    const char *text = gtk_entry_get_text(GTK_ENTRY(browser->url_entry));
    if (text && *text) {
        load_url(browser, text); // load_url will handle search via settings
    }
}

static void on_new_tab_clicked(GtkButton *button, BrowserWindow *browser) 

{
    new_tab(browser, settings.home_page);
}

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

static void on_load_changed(WebKitWebView *web_view, WebKitLoadEvent event, BrowserWindow *browser) 

{
    if (!browser || !browser->notebook) return;
    
    if (event == WEBKIT_LOAD_FINISHED) {
        const char *uri = webkit_web_view_get_uri(web_view);
        const char *title = webkit_web_view_get_title(web_view);
        
        if (uri && *uri) {
            printf("Page loaded: %s (%s)\n", uri, title ? title : "no title");
            
            // Add to history if enabled
            if (settings.remember_history) {
                add_to_history(uri, title);
            }
            
            // Update URL bar only if this is the current tab
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

static void on_title_changed(WebKitWebView *web_view, GParamSpec *pspec, BrowserTab *tab) 

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

static gboolean on_close_tab_clicked(GtkButton *button, BrowserWindow *browser)

{
    GtkWidget *tab_child = g_object_get_data(G_OBJECT(button), "tab-child");
    if (tab_child) {
        close_tab(browser, tab_child);
    }
    return TRUE;
}

// Browser window creation
void voidfox_activate(GtkApplication *app, gpointer user_data) {

    DEBUG_PRINT("voidfox_activate started");
    
    BrowserWindow *browser = g_new0(BrowserWindow, 1);
    if (!browser) {
        DEBUG_PRINT("Failed to allocate browser window");
        return;
    }
    DEBUG_PRINT("Browser window struct allocated");

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
    gtk_window_set_decorated(GTK_WINDOW(browser->window), FALSE); // Remove default title bar

    // Enable events for dragging on the whole window
    gtk_widget_add_events(browser->window, GDK_BUTTON_PRESS_MASK |
                                           GDK_BUTTON_RELEASE_MASK |
                                           GDK_POINTER_MOTION_MASK);
    g_signal_connect(browser->window, "button-press-event", G_CALLBACK(on_button_press), browser->window);
    g_signal_connect(browser->window, "button-release-event", G_CALLBACK(on_button_release), browser->window);
    g_signal_connect(browser->window, "motion-notify-event", G_CALLBACK(on_motion_notify), browser->window);

    // Main vertical box
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(browser->window), vbox);
    DEBUG_PRINT("VBox created");

    // Custom title bar
    GtkWidget *title_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_name(title_bar, "title-bar");
    gtk_widget_set_size_request(title_bar, -1, 30);
    gtk_box_pack_start(GTK_BOX(vbox), title_bar, FALSE, FALSE, 0);

    GtkWidget *title_label = gtk_label_new("VoidFox Web Browser");
    gtk_label_set_xalign(GTK_LABEL(title_label), 0.0);
    gtk_box_pack_start(GTK_BOX(title_bar), title_label, TRUE, TRUE, 10);

    GtkWidget *window_buttons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_pack_end(GTK_BOX(title_bar), window_buttons, FALSE, FALSE, 5);

    // Minimize button
    GtkWidget *min_btn = gtk_button_new_with_label("─");
    gtk_widget_set_size_request(min_btn, 30, 25);
    g_signal_connect(min_btn, "clicked", G_CALLBACK(on_minimize_clicked), browser->window);
    gtk_box_pack_start(GTK_BOX(window_buttons), min_btn, FALSE, FALSE, 0);

    // Maximize button
    GtkWidget *max_btn = gtk_button_new_with_label("□");
    gtk_widget_set_size_request(max_btn, 30, 25);
    g_signal_connect(max_btn, "clicked", G_CALLBACK(on_maximize_clicked), browser->window);
    g_signal_connect(browser->window, "window-state-event", G_CALLBACK(on_window_state_changed), max_btn);
    gtk_box_pack_start(GTK_BOX(window_buttons), max_btn, FALSE, FALSE, 0);

    // Close button
    GtkWidget *close_btn = gtk_button_new_with_label("✕");
    gtk_widget_set_size_request(close_btn, 30, 25);
    g_signal_connect(close_btn, "clicked", G_CALLBACK(on_close_clicked), browser->window);
    gtk_box_pack_start(GTK_BOX(window_buttons), close_btn, FALSE, FALSE, 0);

    // Separator
    GtkWidget *sep1 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), sep1, FALSE, FALSE, 0);

    // Toolbar
    GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 5);
    DEBUG_PRINT("Toolbar created");

    // Application menu button (hamburger menu)
    GtkWidget *app_menu_button = gtk_menu_button_new();
    gtk_button_set_label(GTK_BUTTON(app_menu_button), "☰");
    gtk_widget_set_tooltip_text(app_menu_button, "Menu");
    gtk_box_pack_start(GTK_BOX(toolbar), app_menu_button, FALSE, FALSE, 0);
    browser->app_menu_button = app_menu_button;

    // Create and attach app menu
    GtkWidget *app_menu = create_application_menu(browser);
    gtk_menu_button_set_popup(GTK_MENU_BUTTON(app_menu_button), app_menu);

    // Bookmarks button
    GtkWidget *bookmarks_button = gtk_menu_button_new();
    gtk_button_set_label(GTK_BUTTON(bookmarks_button), "★");
    gtk_widget_set_tooltip_text(bookmarks_button, "Bookmarks");
    gtk_box_pack_start(GTK_BOX(toolbar), bookmarks_button, FALSE, FALSE, 0);
    browser->bookmarks_button = bookmarks_button;

    // Load bookmarks and create menu
    load_bookmarks();
    GtkWidget *bookmarks_menu = create_bookmarks_menu(browser);
    gtk_menu_button_set_popup(GTK_MENU_BUTTON(bookmarks_button), bookmarks_menu);

    // Back button
    browser->back_button = gtk_button_new_from_icon_name("go-previous", GTK_ICON_SIZE_MENU);
    if (browser->back_button) {
        g_signal_connect_swapped(browser->back_button, "clicked", G_CALLBACK(go_back), browser);
        gtk_box_pack_start(GTK_BOX(toolbar), browser->back_button, FALSE, FALSE, 0);
    }

    // Forward button
    browser->forward_button = gtk_button_new_from_icon_name("go-next", GTK_ICON_SIZE_MENU);
    if (browser->forward_button) {
        g_signal_connect_swapped(browser->forward_button, "clicked", G_CALLBACK(go_forward), browser);
        gtk_box_pack_start(GTK_BOX(toolbar), browser->forward_button, FALSE, FALSE, 0);
    }

    // Reload button
    browser->reload_button = gtk_button_new_from_icon_name("view-refresh", GTK_ICON_SIZE_MENU);
    if (browser->reload_button) {
        g_signal_connect_swapped(browser->reload_button, "clicked", G_CALLBACK(reload_page), browser);
        gtk_box_pack_start(GTK_BOX(toolbar), browser->reload_button, FALSE, FALSE, 0);
    }

    // Stop button
    browser->stop_button = gtk_button_new_from_icon_name("process-stop", GTK_ICON_SIZE_MENU);
    if (browser->stop_button) {
        g_signal_connect_swapped(browser->stop_button, "clicked", G_CALLBACK(stop_loading), browser);
        gtk_box_pack_start(GTK_BOX(toolbar), browser->stop_button, FALSE, FALSE, 0);
    }

    // URL entry
    browser->url_entry = gtk_entry_new();
    if (browser->url_entry) {
        gtk_entry_set_placeholder_text(GTK_ENTRY(browser->url_entry), "Enter URL or search term");
        g_signal_connect(browser->url_entry, "activate", G_CALLBACK(on_url_activate), browser);
        gtk_box_pack_start(GTK_BOX(toolbar), browser->url_entry, TRUE, TRUE, 0);
    }

    // Go button
    GtkWidget *go_button = gtk_button_new_with_label("Go");
    if (go_button) {
        g_signal_connect(go_button, "clicked", G_CALLBACK(on_go_clicked), browser);
        gtk_box_pack_start(GTK_BOX(toolbar), go_button, FALSE, FALSE, 0);
    }
    
    // Home button
    GtkWidget *home_button = gtk_button_new_from_icon_name("go-home", GTK_ICON_SIZE_MENU);
    if (home_button) {
        g_signal_connect(home_button, "clicked", G_CALLBACK(on_home_clicked), browser);
        gtk_box_pack_start(GTK_BOX(toolbar), home_button, FALSE, FALSE, 0);
    }
    
    // Search button
    GtkWidget *search_button = gtk_button_new_with_label("Search");
    if (search_button) {
        g_signal_connect(search_button, "clicked", G_CALLBACK(on_search_clicked), browser);
        gtk_box_pack_start(GTK_BOX(toolbar), search_button, FALSE, FALSE, 0);
    }

    // New tab button
    GtkWidget *new_tab_button = gtk_button_new_from_icon_name("tab-new", GTK_ICON_SIZE_MENU);
    if (new_tab_button) {
        g_signal_connect(new_tab_button, "clicked", G_CALLBACK(on_new_tab_clicked), browser);
        gtk_box_pack_start(GTK_BOX(toolbar), new_tab_button, FALSE, FALSE, 0);
    }

    // Separator
    GtkWidget *sep2 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), sep2, FALSE, FALSE, 0);

    // Load settings
    load_settings();

    // Notebook for tabs
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

    // Apply dark theme with title bar styling (based on settings)
    GtkCssProvider *provider = gtk_css_provider_new();
    if (settings.dark_mode) {
        gtk_css_provider_load_from_data(provider,
            "window { background-color: #0b0f14; color: #ffffff; }\n"
            "entry { background-color: #1e2429; color: #ffffff; border: 1px solid #00ff88; }\n"
            "button { background-color: #1e2429; color: #00ff88; border: none; }\n"
            "button:hover { background-color: #2a323a; }\n"
            "notebook { background-color: #0b0f14; }\n"
            "#title-bar { background-color: #0b0f14; border-bottom: 2px solid #00ff88; }\n",
            -1, NULL);
    } else {
        gtk_css_provider_load_from_data(provider,
            "window { background-color: #f0f0f0; color: #000000; }\n"
            "entry { background-color: #ffffff; color: #000000; border: 1px solid #888888; }\n"
            "button { background-color: #e0e0e0; color: #000000; border: none; }\n"
            "button:hover { background-color: #d0d0d0; }\n"
            "notebook { background-color: #f0f0f0; }\n"
            "#title-bar { background-color: #e0e0e0; border-bottom: 2px solid #888888; }\n",
            -1, NULL);
    }
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    // Load history at startup if enabled
    if (settings.remember_history) {
        load_history();
    }
    
    DEBUG_PRINT("Creating first tab...");
    // Create first tab
    new_tab(browser, settings.home_page);
    DEBUG_PRINT("First tab created");

    gtk_widget_show_all(browser->window);
    DEBUG_PRINT("Window shown");
    
    update_navigation_buttons(browser);
    DEBUG_PRINT("voidfox_activate completed");
}

// Tab management
void new_tab(BrowserWindow *browser, const char *url) 

{
    if (!browser || !browser->notebook) {
        DEBUG_PRINT("new_tab: invalid browser or notebook");
        return;
    }
    
    DEBUG_PRINT("new_tab: creating new tab with URL: %s", url ? url : "NULL");
    
    // Create WebView with settings
    WebKitWebView *web_view = WEBKIT_WEB_VIEW(webkit_web_view_new());
    if (!web_view) {
        DEBUG_PRINT("Failed to create web view");
        return;
    }
    DEBUG_PRINT("WebView created");
    
    // Apply settings to web view
    apply_settings_to_web_view(web_view);
    
    g_signal_connect(web_view, "load-changed", G_CALLBACK(on_load_changed), browser);

    // Load URL
    const char *load_url = url ? url : settings.home_page;
    DEBUG_PRINT("Loading URL: %s", load_url);
    webkit_web_view_load_uri(web_view, load_url);

    // Create tab content container
    GtkWidget *tab_child = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    if (!tab_child) {
        DEBUG_PRINT("Failed to create tab child");
        g_object_unref(web_view);
        return;
    }
    gtk_box_pack_start(GTK_BOX(tab_child), GTK_WIDGET(web_view), TRUE, TRUE, 0);
    gtk_widget_show(tab_child);
    DEBUG_PRINT("Tab child container created");

    // Create BrowserTab struct
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

    // Pack close button into a box
    GtkWidget *tab_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    if (tab_box) {
        if (tab->tab_label) gtk_box_pack_start(GTK_BOX(tab_box), tab->tab_label, FALSE, FALSE, 0);
        if (tab->close_button) gtk_box_pack_start(GTK_BOX(tab_box), tab->close_button, FALSE, FALSE, 0);
        gtk_widget_show_all(tab_box);
    }

    // Store tab data
    g_object_set_data_full(G_OBJECT(tab_child), "browser-tab", tab, g_free);
    if (tab->close_button) {
        g_object_set_data_full(G_OBJECT(tab->close_button), "tab-child", tab_child, NULL);
        g_signal_connect(tab->close_button, "clicked", G_CALLBACK(on_close_tab_clicked), browser);
    }
    g_signal_connect(web_view, "notify::title", G_CALLBACK(on_title_changed), tab);

    // Append tab
    int page_num = gtk_notebook_append_page(GTK_NOTEBOOK(browser->notebook), tab_child, tab_box);
    gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(browser->notebook), tab_child, TRUE);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(browser->notebook), page_num);
    DEBUG_PRINT("Tab appended, page number: %d", page_num);
}

void close_tab(BrowserWindow *browser, GtkWidget *tab_child) 

{
    if (!browser || !browser->notebook || !tab_child) return;
    
    int page_num = gtk_notebook_page_num(GTK_NOTEBOOK(browser->notebook), tab_child);
    if (page_num != -1) {
        // Don't close if it's the last tab 
        if (gtk_notebook_get_n_pages(GTK_NOTEBOOK(browser->notebook)) <= 1) {
            new_tab(browser, settings.home_page);
        }
        gtk_notebook_remove_page(GTK_NOTEBOOK(browser->notebook), page_num);
    }
}

// Navigation
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
        // If no scheme, prepend http://
        if (strstr(text, "://") == NULL) {
            full_url = g_strconcat("http://", text, NULL);
        } else {
            full_url = g_strdup(text);
        }
    } else {
        // It's a search term - use settings to construct URL
        full_url = g_strdup(get_search_url(text));
    }

    if (full_url) {
        webkit_web_view_load_uri(tab->web_view, full_url);
        g_free(full_url);
    }
}

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