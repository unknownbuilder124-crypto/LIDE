#include "firefox.h"

// Dragging variables
static int is_dragging = 0;
static int drag_start_x, drag_start_y;

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

static void on_close_clicked(GtkButton *button, gpointer window)
{
    (void)button;
    gtk_window_close(GTK_WINDOW(window));
}

// Navigation callbacks
static void on_url_activate(GtkEntry *entry, FirefoxWindow *firefox)
{
    const char *text = gtk_entry_get_text(entry);
    if (text && *text) {
        load_url(firefox, text);
    }
}

static void on_go_clicked(GtkButton *button, FirefoxWindow *firefox)
{
    (void)button;
    const char *text = gtk_entry_get_text(GTK_ENTRY(firefox->url_entry));
    if (text && *text) {
        load_url(firefox, text);
    }
}

static void on_home_clicked(GtkButton *button, FirefoxWindow *firefox)
{
    (void)button;
    load_url(firefox, "https://www.google.com");
}

static void on_load_changed(WebKitWebView *web_view, WebKitLoadEvent event, FirefoxWindow *firefox)
{
    if (event == WEBKIT_LOAD_FINISHED) {
        const char *uri = webkit_web_view_get_uri(web_view);
        if (uri && *uri) {
            gtk_entry_set_text(GTK_ENTRY(firefox->url_entry), uri);
        }
        update_navigation_buttons(firefox);
    }
}

static void on_title_changed(WebKitWebView *web_view, GParamSpec *pspec, FirefoxWindow *firefox)
{
    (void)pspec;
    const char *title = webkit_web_view_get_title(web_view);
    if (title && *title) {
        char *window_title = g_strdup_printf("Firefox - %s", title);
        gtk_window_set_title(GTK_WINDOW(firefox->window), window_title);
        g_free(window_title);
    }
}

// Firefox window creation
void firefox_activate(GtkApplication *app, gpointer user_data)
{
    (void)user_data;
    
    FirefoxWindow *firefox = g_new0(FirefoxWindow, 1);
    
    firefox->window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(firefox->window), "Firefox");
    gtk_window_set_default_size(GTK_WINDOW(firefox->window), 1024, 768);
    gtk_window_set_position(GTK_WINDOW(firefox->window), GTK_WIN_POS_CENTER);
    gtk_window_set_decorated(GTK_WINDOW(firefox->window), FALSE); // Remove default title bar

    // Enable dragging
    gtk_widget_add_events(firefox->window, GDK_BUTTON_PRESS_MASK |
                                           GDK_BUTTON_RELEASE_MASK |
                                           GDK_POINTER_MOTION_MASK);
    g_signal_connect(firefox->window, "button-press-event", G_CALLBACK(on_button_press), firefox->window);
    g_signal_connect(firefox->window, "button-release-event", G_CALLBACK(on_button_release), firefox->window);
    g_signal_connect(firefox->window, "motion-notify-event", G_CALLBACK(on_motion_notify), firefox->window);

    // Main vertical box
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(firefox->window), vbox);

    // Custom title bar
    GtkWidget *title_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_name(title_bar, "title-bar");
    gtk_widget_set_size_request(title_bar, -1, 30);
    gtk_box_pack_start(GTK_BOX(vbox), title_bar, FALSE, FALSE, 0);

    // Firefox logo in title bar
    GtkWidget *logo_label = gtk_label_new("🦊");
    gtk_box_pack_start(GTK_BOX(title_bar), logo_label, FALSE, FALSE, 10);
    
    GtkWidget *title_label = gtk_label_new("Firefox Browser");
    gtk_label_set_xalign(GTK_LABEL(title_label), 0.0);
    gtk_box_pack_start(GTK_BOX(title_bar), title_label, FALSE, FALSE, 5);

    GtkWidget *window_buttons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_pack_end(GTK_BOX(title_bar), window_buttons, FALSE, FALSE, 5);

    // Minimize button
    GtkWidget *min_btn = gtk_button_new_with_label("─");
    gtk_widget_set_size_request(min_btn, 30, 25);
    g_signal_connect(min_btn, "clicked", G_CALLBACK(on_minimize_clicked), firefox->window);
    gtk_box_pack_start(GTK_BOX(window_buttons), min_btn, FALSE, FALSE, 0);

    // Maximize button
    GtkWidget *max_btn = gtk_button_new_with_label("□");
    gtk_widget_set_size_request(max_btn, 30, 25);
    g_signal_connect(max_btn, "clicked", G_CALLBACK(on_maximize_clicked), firefox->window);
    gtk_box_pack_start(GTK_BOX(window_buttons), max_btn, FALSE, FALSE, 0);

    // Close button
    GtkWidget *close_btn = gtk_button_new_with_label("✕");
    gtk_widget_set_size_request(close_btn, 30, 25);
    g_signal_connect(close_btn, "clicked", G_CALLBACK(on_close_clicked), firefox->window);
    gtk_box_pack_start(GTK_BOX(window_buttons), close_btn, FALSE, FALSE, 0);

    // Separator
    GtkWidget *sep1 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), sep1, FALSE, FALSE, 0);

    // Toolbar
    GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 5);

    // Back button
    firefox->back_button = gtk_button_new_from_icon_name("go-previous", GTK_ICON_SIZE_MENU);
    g_signal_connect_swapped(firefox->back_button, "clicked", G_CALLBACK(go_back), firefox);
    gtk_box_pack_start(GTK_BOX(toolbar), firefox->back_button, FALSE, FALSE, 0);

    // Forward button
    firefox->forward_button = gtk_button_new_from_icon_name("go-next", GTK_ICON_SIZE_MENU);
    g_signal_connect_swapped(firefox->forward_button, "clicked", G_CALLBACK(go_forward), firefox);
    gtk_box_pack_start(GTK_BOX(toolbar), firefox->forward_button, FALSE, FALSE, 0);

    // Reload button
    firefox->reload_button = gtk_button_new_from_icon_name("view-refresh", GTK_ICON_SIZE_MENU);
    g_signal_connect_swapped(firefox->reload_button, "clicked", G_CALLBACK(reload_page), firefox);
    gtk_box_pack_start(GTK_BOX(toolbar), firefox->reload_button, FALSE, FALSE, 0);

    // Home button
    firefox->home_button = gtk_button_new_from_icon_name("go-home", GTK_ICON_SIZE_MENU);
    g_signal_connect(firefox->home_button, "clicked", G_CALLBACK(on_home_clicked), firefox);
    gtk_box_pack_start(GTK_BOX(toolbar), firefox->home_button, FALSE, FALSE, 0);

    // URL entry
    firefox->url_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(firefox->url_entry), "Enter URL");
    g_signal_connect(firefox->url_entry, "activate", G_CALLBACK(on_url_activate), firefox);
    gtk_box_pack_start(GTK_BOX(toolbar), firefox->url_entry, TRUE, TRUE, 0);

    // Go button
    GtkWidget *go_button = gtk_button_new_with_label("Go");
    g_signal_connect(go_button, "clicked", G_CALLBACK(on_go_clicked), firefox);
    gtk_box_pack_start(GTK_BOX(toolbar), go_button, FALSE, FALSE, 0);

    // Separator
    GtkWidget *sep2 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), sep2, FALSE, FALSE, 0);

    // WebView
    firefox->web_view = WEBKIT_WEB_VIEW(webkit_web_view_new());
    
    // Configure WebView settings
    WebKitSettings *settings = webkit_web_view_get_settings(firefox->web_view);
    webkit_settings_set_hardware_acceleration_policy(settings, WEBKIT_HARDWARE_ACCELERATION_POLICY_NEVER);
    webkit_settings_set_enable_webgl(settings, FALSE);
    webkit_settings_set_user_agent(settings, 
        "Mozilla/5.0 (X11; Linux x86_64; rv:120.0) Gecko/20100101 Firefox/120.0");
    
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(firefox->web_view), TRUE, TRUE, 0);

    // Connect signals
    g_signal_connect(firefox->web_view, "load-changed", G_CALLBACK(on_load_changed), firefox);
    g_signal_connect(firefox->web_view, "notify::title", G_CALLBACK(on_title_changed), firefox);

    // Load home page
    webkit_web_view_load_uri(firefox->web_view, "https://www.google.com");

    // Apply Firefox theme
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "window { background-color: #0b0f14; color: #ffffff; }\n"
        "entry { background-color: #1e2429; color: #ffffff; border: 1px solid #ff6600; border-radius: 3px; }\n"
        "button { background-color: #1e2429; color: #ff6600; border: 1px solid #ff6600; }\n"
        "button:hover { background-color: #2a323a; }\n"
        "#title-bar { background-color: #0b0f14; border-bottom: 2px solid #ff6600; }\n",
        -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    gtk_widget_show_all(firefox->window);
    update_navigation_buttons(firefox);
}

// Navigation functions
void load_url(FirefoxWindow *firefox, const char *text)
{
    if (!text || *text == '\0') return;
    
    char *full_url;
    
    if (strstr(text, "://") == NULL) {
        full_url = g_strconcat("http://", text, NULL);
    } else {
        full_url = g_strdup(text);
    }
    
    webkit_web_view_load_uri(firefox->web_view, full_url);
    g_free(full_url);
}

void go_back(FirefoxWindow *firefox)
{
    if (webkit_web_view_can_go_back(firefox->web_view)) {
        webkit_web_view_go_back(firefox->web_view);
    }
}

void go_forward(FirefoxWindow *firefox)
{
    if (webkit_web_view_can_go_forward(firefox->web_view)) {
        webkit_web_view_go_forward(firefox->web_view);
    }
}

void reload_page(FirefoxWindow *firefox)
{
    webkit_web_view_reload(firefox->web_view);
}

void update_navigation_buttons(FirefoxWindow *firefox)
{
    gtk_widget_set_sensitive(firefox->back_button, webkit_web_view_can_go_back(firefox->web_view));
    gtk_widget_set_sensitive(firefox->forward_button, webkit_web_view_can_go_forward(firefox->web_view));
}

int main(int argc, char **argv)
{
    GtkApplication *app = gtk_application_new("org.lide.firefox", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(firefox_activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}