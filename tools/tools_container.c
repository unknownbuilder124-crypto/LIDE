#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <gdk/gdkx.h>

static int is_dragging = 0;
static int drag_start_x, drag_start_y;

// Launch functions 
static void launch_file_manager(GtkButton *button, gpointer window) 

{
    (void)button;
    
    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        execl("./blackline-fm", "blackline-fm", NULL);
        exit(0);
    } else if (pid > 0) {
        // Parent process - close tools container
        gtk_window_close(GTK_WINDOW(window));
    }
}

static void launch_text_editor(GtkButton *button, gpointer window) 

{
    (void)button;
    
    pid_t pid = fork();
    if (pid == 0) {
        // Launch custom text editor
        execl("./blackline-editor", "blackline-editor", NULL);
        exit(0);
    } else if (pid > 0) {
        // Parent process - close tools container
        gtk_window_close(GTK_WINDOW(window));
    }
}

static void launch_calculator(GtkButton *button, gpointer window) 

{
    (void)button;
    
    pid_t pid = fork();
    if (pid == 0) {
        // Launch custom calculator 
        execl("./blackline-calculator", "blackline-calculator", NULL);
        exit(0);
    } else if (pid > 0) {
        // Parent process - close tools container
        gtk_window_close(GTK_WINDOW(window));
    }
}

static void launch_system_monitor(GtkButton *button, gpointer window) 

{
    (void)button;
    
    pid_t pid = fork();
    if (pid == 0) {
        // Launch custom system monitor
        execl("./blackline-system-monitor", "blackline-system-monitor", NULL);
        exit(0);
    } else if (pid > 0) {
        // Parent process - close tools container
        gtk_window_close(GTK_WINDOW(window));
    }
}

// NEW: Web browser launcher function
static void launch_web_browser(GtkButton *button, gpointer window) 

{
    (void)button;
    
    pid_t pid = fork();
    if (pid == 0) {
        // Launch VoidFox web browser
        execl("./voidfox", "voidfox", NULL);
        exit(0);
    } else if (pid > 0) {
        // Parent process - close tools container
        gtk_window_close(GTK_WINDOW(window));
    }
}

// Dragging functions
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
    if (event->button == 1) {
        is_dragging = 0;
        return TRUE;
    }
    return FALSE;
}

static gboolean on_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer window) 

{
    if (is_dragging) 
    {
        int dx = event->x_root - drag_start_x;
        int dy = event->y_root - drag_start_y;
        
        int x, y;
        gtk_window_get_position(GTK_WINDOW(window), &x, &y);
        gtk_window_move(GTK_WINDOW(window), x + dx, y + dy);
        
        drag_start_x = event->x_root;
        drag_start_y = event->y_root;
        return TRUE;
    }
    return FALSE;
}

static void on_close_clicked(GtkButton *button, gpointer window) 

{
    gtk_window_close(GTK_WINDOW(window));
}

static void activate(GtkApplication *app, gpointer user_data) 

{
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "BlackLine Tools");
    gtk_window_set_default_size(GTK_WINDOW(window), 300, 450);  // Increased height for new button
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);
    
    // Enable events for dragging
    gtk_widget_add_events(window, GDK_BUTTON_PRESS_MASK | 
                                   GDK_BUTTON_RELEASE_MASK | 
                                   GDK_POINTER_MOTION_MASK);
    
    // Connect drag signals
    g_signal_connect(window, "button-press-event", G_CALLBACK(on_button_press), window);
    g_signal_connect(window, "button-release-event", G_CALLBACK(on_button_release), window);
    g_signal_connect(window, "motion-notify-event", G_CALLBACK(on_motion_notify), window);
    
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    
    // Title bar
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<b>Tools</b>");
    gtk_box_pack_start(GTK_BOX(hbox), title, TRUE, TRUE, 0);
    
    GtkWidget *close_btn = gtk_button_new_with_label("X");
    gtk_widget_set_size_request(close_btn, 30, 25);
    g_signal_connect(close_btn, "clicked", G_CALLBACK(on_close_clicked), window);
    gtk_box_pack_end(GTK_BOX(hbox), close_btn, FALSE, FALSE, 0);
    
    // Separator
    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), sep, FALSE, FALSE, 5);
    
    // File Manager button
    GtkWidget *fm_btn = gtk_button_new_with_label("📁 File Manager");
    g_signal_connect(fm_btn, "clicked", G_CALLBACK(launch_file_manager), window);
    gtk_box_pack_start(GTK_BOX(vbox), fm_btn, FALSE, FALSE, 2);
    
    // Text Editor button 
    GtkWidget *editor_btn = gtk_button_new_with_label("📝 Text Editor");
    g_signal_connect(editor_btn, "clicked", G_CALLBACK(launch_text_editor), window);
    gtk_box_pack_start(GTK_BOX(vbox), editor_btn, FALSE, FALSE, 2);
    
    // Calculator button 
    GtkWidget *calc_btn = gtk_button_new_with_label("🧮 Calculator");
    g_signal_connect(calc_btn, "clicked", G_CALLBACK(launch_calculator), window);
    gtk_box_pack_start(GTK_BOX(vbox), calc_btn, FALSE, FALSE, 2);
    
    // System Monitor button 
    GtkWidget *monitor_btn = gtk_button_new_with_label("📊 System Monitor");
    g_signal_connect(monitor_btn, "clicked", G_CALLBACK(launch_system_monitor), window);
    gtk_box_pack_start(GTK_BOX(vbox), monitor_btn, FALSE, FALSE, 2);
    
    // Web Browser Button
    GtkWidget *browser_btn = gtk_button_new_with_label("🌐 VoidFox");
    g_signal_connect(browser_btn, "clicked", G_CALLBACK(launch_web_browser), window);
    gtk_box_pack_start(GTK_BOX(vbox), browser_btn, FALSE, FALSE, 2);
    
    // Add some spacing at the bottom
    GtkWidget *bottom_spacer = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(vbox), bottom_spacer, TRUE, TRUE, 0);
    
    gtk_widget_show_all(window);
    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv) 

{
    GtkApplication *app = gtk_application_new("org.blackline.tools", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}