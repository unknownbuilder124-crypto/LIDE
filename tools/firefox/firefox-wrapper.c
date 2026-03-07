#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

// Dragging variables
static int is_dragging = 0;
static int drag_start_x, drag_start_y;
static pid_t firefox_pid = 0;
static GtkWidget *status_label = NULL;
static GtkWidget *launch_btn = NULL;

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

// Track window state changes
static gboolean on_window_state_changed(GtkWidget *window, GdkEventWindowState *event, gpointer data)
{
    GtkButton *max_btn = GTK_BUTTON(data);
    
    if (event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED) {
        gtk_button_set_label(max_btn, "❐");
        gtk_widget_set_tooltip_text(GTK_WIDGET(max_btn), "Restore");
    } else {
        gtk_button_set_label(max_btn, "□");
        gtk_widget_set_tooltip_text(GTK_WIDGET(max_btn), "Maximize");
    }
    
    return FALSE;
}

static void on_close_clicked(GtkButton *button, gpointer window)
{
    (void)button;
    if (firefox_pid > 0) {
        kill(firefox_pid, SIGTERM);
        waitpid(firefox_pid, NULL, 0);
    }
    gtk_window_close(GTK_WINDOW(window));
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

// Check if Firefox is installed
static int is_firefox_installed(void)
{
    return (system("which firefox > /dev/null 2>&1") == 0 || 
            system("which firefox-esr > /dev/null 2>&1") == 0);
}

// Check if process is running
static int is_process_running(pid_t pid)
{
    return (kill(pid, 0) == 0);
}

// Launch Firefox
static void launch_firefox(GtkButton *button, gpointer window)
{
    (void)button;
    
    // Check if Firefox is installed
    if (!is_firefox_installed()) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window),
                                                  GTK_DIALOG_MODAL,
                                                  GTK_MESSAGE_ERROR,
                                                  GTK_BUTTONS_OK,
                                                  "Firefox is not installed!\n\n"
                                                  "Install it with:\n"
                                                  "sudo apt install firefox\n"
                                                  "or\n"
                                                  "sudo apt install firefox-esr");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    
    // Check if Firefox is already running
    if (firefox_pid > 0 && is_process_running(firefox_pid)) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window),
                                                  GTK_DIALOG_MODAL,
                                                  GTK_MESSAGE_INFO,
                                                  GTK_BUTTONS_OK,
                                                  "Firefox is already running!\n\n"
                                                  "Check your taskbar or system tray.");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    
    // Reset PID if process is not running
    if (firefox_pid > 0 && !is_process_running(firefox_pid)) {
        firefox_pid = 0;
    }
    
    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        setsid();
        
        // Launch Firefox normally (no custom profile)
        execlp("firefox", "firefox", "--new-window", "https://www.google.com", NULL);
        
        // If firefox fails, try firefox-esr
        execlp("firefox-esr", "firefox-esr", "--new-window", "https://www.google.com", NULL);
        
        exit(1);
    } else if (pid > 0) {
        firefox_pid = pid;
        
        // Update status
        if (status_label) {
            gtk_label_set_text(GTK_LABEL(status_label), "Firefox launched successfully!");
        }
        
        // Disable launch button temporarily
        gtk_widget_set_sensitive(launch_btn, FALSE);
    } else {
        perror("Fork failed");
    }
}

// Monitor Firefox process
static gboolean monitor_firefox_process(gpointer data)
{
    (void)data;
    
    if (firefox_pid > 0 && !is_process_running(firefox_pid)) {
        firefox_pid = 0;
        if (status_label) {
            gtk_label_set_text(GTK_LABEL(status_label), "Firefox is not running");
        }
        if (launch_btn) {
            gtk_widget_set_sensitive(launch_btn, TRUE);
        }
    }
    
    return G_SOURCE_CONTINUE;
}

static void activate(GtkApplication *app, gpointer user_data)
{
    (void)user_data;
    
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Firefox Launcher");
    gtk_window_set_default_size(GTK_WINDOW(window), 450, 350);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
    gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);

    // Enable dragging
    gtk_widget_add_events(window, GDK_BUTTON_PRESS_MASK |
                                   GDK_BUTTON_RELEASE_MASK |
                                   GDK_POINTER_MOTION_MASK);
    g_signal_connect(window, "button-press-event", G_CALLBACK(on_button_press), window);
    g_signal_connect(window, "button-release-event", G_CALLBACK(on_button_release), window);
    g_signal_connect(window, "motion-notify-event", G_CALLBACK(on_motion_notify), window);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // Custom title bar
    GtkWidget *title_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_name(title_bar, "title-bar");
    gtk_widget_set_size_request(title_bar, -1, 35);
    gtk_box_pack_start(GTK_BOX(vbox), title_bar, FALSE, FALSE, 0);

    GtkWidget *title_label = gtk_label_new("🦊 Firefox Browser");
    gtk_label_set_xalign(GTK_LABEL(title_label), 0.0);
    gtk_box_pack_start(GTK_BOX(title_bar), title_label, TRUE, TRUE, 10);

    GtkWidget *window_buttons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_pack_end(GTK_BOX(title_bar), window_buttons, FALSE, FALSE, 5);

    // Minimize button
    GtkWidget *min_btn = gtk_button_new_with_label("─");
    gtk_widget_set_size_request(min_btn, 35, 25);
    g_signal_connect(min_btn, "clicked", G_CALLBACK(on_minimize_clicked), window);
    gtk_box_pack_start(GTK_BOX(window_buttons), min_btn, FALSE, FALSE, 0);

    // Maximize button
    GtkWidget *max_btn = gtk_button_new_with_label("□");
    gtk_widget_set_size_request(max_btn, 35, 25);
    g_signal_connect(max_btn, "clicked", G_CALLBACK(on_maximize_clicked), window);
    g_signal_connect(window, "window-state-event", G_CALLBACK(on_window_state_changed), max_btn);
    gtk_box_pack_start(GTK_BOX(window_buttons), max_btn, FALSE, FALSE, 0);

    // Close button
    GtkWidget *close_btn = gtk_button_new_with_label("✕");
    gtk_widget_set_size_request(close_btn, 35, 25);
    g_signal_connect(close_btn, "clicked", G_CALLBACK(on_close_clicked), window);
    gtk_box_pack_start(GTK_BOX(window_buttons), close_btn, FALSE, FALSE, 0);

    // Separator
    GtkWidget *sep1 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), sep1, FALSE, FALSE, 0);

    // Content area
    GtkWidget *content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_container_set_border_width(GTK_CONTAINER(content), 25);
    gtk_box_pack_start(GTK_BOX(vbox), content, TRUE, TRUE, 0);

    // Firefox logo
    GtkWidget *logo_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(content), logo_box, FALSE, FALSE, 0);
    
    GtkWidget *logo_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(logo_label), 
        "<span font='64' foreground='#ff6600'>🦊</span>");
    gtk_box_pack_start(GTK_BOX(logo_box), logo_label, FALSE, FALSE, 20);
    
    GtkWidget *title_large = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title_large), 
        "<span font='32' weight='bold' foreground='#ff6600'>Firefox</span>");
    gtk_box_pack_start(GTK_BOX(logo_box), title_large, FALSE, FALSE, 0);

    // Description
    GtkWidget *desc = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(desc), 
        "<span size='12000'>Mozilla Firefox Web Browser</span>\n\n"
        "<span>Fast. Private. Yours.</span>");
    gtk_box_pack_start(GTK_BOX(content), desc, FALSE, FALSE, 10);

    // Features list
    GtkWidget *features = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(features), 
        "• Lightning fast performance\n"
        "• Enhanced tracking protection\n"
        "• Built-in password manager\n"
        "• Sync across devices\n"
        "• Thousands of add-ons");
    gtk_box_pack_start(GTK_BOX(content), features, FALSE, FALSE, 10);

    // Launch button
    launch_btn = gtk_button_new_with_label("🚀 Launch Firefox");
    gtk_widget_set_size_request(launch_btn, 250, 45);
    g_signal_connect(launch_btn, "clicked", G_CALLBACK(launch_firefox), window);
    gtk_box_pack_start(GTK_BOX(content), launch_btn, FALSE, FALSE, 20);

    // Status label
    status_label = gtk_label_new("Click Launch to start Firefox");
    gtk_widget_set_opacity(status_label, 0.7);
    gtk_box_pack_start(GTK_BOX(content), status_label, FALSE, FALSE, 5);

    // Apply Firefox theme (orange accent)
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "window { background-color: #0b0f14; color: #ffffff; }\n"
        "button { background-color: #1e2429; color: #ff6600; border: 2px solid #ff6600; border-radius: 5px; padding: 8px; }\n"
        "button:hover { background-color: #2a323a; }\n"
        "#title-bar { background-color: #0b0f14; border-bottom: 3px solid #ff6600; }\n"
        "#title-bar button { background-color: transparent; border: none; color: #ffffff; }\n"
        "#title-bar button:hover { background-color: #ff6600; color: #0b0f14; }\n"
        "label { color: #ffffff; }\n",
        -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);

    gtk_widget_show_all(window);
    
    // Set up process monitor
    g_timeout_add(1000, monitor_firefox_process, NULL);
}

int main(int argc, char **argv)
{
    GtkApplication *app = gtk_application_new("org.lide.firefox", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}