#include <gtk/gtk.h>
#include <vte/vte.h>
#include <gdk/gdkkeysyms.h>
#include <stdlib.h>
#include <string.h>
#include "window_resize.h"

// Terminal application data
typedef struct {
    GtkWidget *window;
    GtkWidget *notebook;
    GtkWidget *title_bar;

    // For dragging and resizing
    int is_dragging;
    int is_resizing;
    int resize_edge;
    int drag_start_x;
    int drag_start_y;
} Terminal;

// Function prototypes
static void new_terminal_tab(const char *initial_directory, Terminal *term);
static void spawn_callback(VteTerminal *terminal, GPid pid, GError *error, gpointer user_data);
static void close_tab_callback(GtkButton *button, gpointer terminal);
static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data);
static gboolean on_button_release(GtkWidget *widget, GdkEventButton *event, gpointer data);
static gboolean on_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer data);
static void on_minimize_clicked(GtkButton *button, gpointer window);
static void on_maximize_clicked(GtkButton *button, gpointer window);
static void on_close_clicked(GtkButton *button, gpointer window);
static gboolean on_window_state_changed(GtkWidget *window, GdkEventWindowState *event, gpointer data);

// Dragging handlers
static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data)

{
    Terminal *term = (Terminal *)data;

    if (event->button == 1)
    {
        // Check if cursor is on an edge (for resizing)
        term->resize_edge = detect_resize_edge_absolute(GTK_WINDOW(term->window), event->x_root, event->y_root);

        if (term->resize_edge != RESIZE_NONE) {
            term->is_resizing = 1;
        } else {
            term->is_dragging = 1;
        }

        term->drag_start_x = event->x_root;
        term->drag_start_y = event->y_root;
        gtk_window_present(GTK_WINDOW(term->window));
        return TRUE;
    }
    return FALSE;
}

static gboolean on_button_release(GtkWidget *widget, GdkEventButton *event, gpointer data)

{
    Terminal *term = (Terminal *)data;

    if (event->button == 1) {
        term->is_dragging = 0;
        term->is_resizing = 0;
        term->resize_edge = RESIZE_NONE;
        return TRUE;
    }
    return FALSE;
}

static gboolean on_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer data)

{
    Terminal *term = (Terminal *)data;

    // Update cursor for resize hints
    if (!term->is_dragging && !term->is_resizing) {
        int resize_edge = detect_resize_edge_absolute(GTK_WINDOW(term->window), event->x_root, event->y_root);
        update_resize_cursor(widget, resize_edge);
    }

    if (term->is_resizing) {
        int delta_x = event->x_root - term->drag_start_x;
        int delta_y = event->y_root - term->drag_start_y;

        int window_width, window_height;
        gtk_window_get_size(GTK_WINDOW(term->window), &window_width, &window_height);

        apply_window_resize(GTK_WINDOW(term->window), term->resize_edge,
                           delta_x, delta_y, window_width, window_height);

        term->drag_start_x = event->x_root;
        term->drag_start_y = event->y_root;
        return TRUE;
    } else if (term->is_dragging) {
        // Only drag from title bar
        GtkAllocation title_alloc;
        gtk_widget_get_allocation(term->title_bar, &title_alloc);
        
        int title_x, title_y;
        gtk_widget_translate_coordinates(term->title_bar, term->window, 0, 0, &title_x, &title_y);
        
        int rel_y = event->y - title_y;
        if (rel_y >= 0 && rel_y <= title_alloc.height) {
            int dx = event->x_root - term->drag_start_x;
            int dy = event->y_root - term->drag_start_y;

            int x, y;
            gtk_window_get_position(GTK_WINDOW(term->window), &x, &y);
            gtk_window_move(GTK_WINDOW(term->window), x + dx, y + dy);

            term->drag_start_x = event->x_root;
            term->drag_start_y = event->y_root;
        }
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
    gtk_window_close(GTK_WINDOW(window));
}

// CSS with visible borders
static void apply_css(void) {
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "window { background-color: #0b0f14; color: #ffffff; "
        "         border: 2px solid #00ff88; }"  // Added visible border around window
        "notebook { background-color: #0b0f14; }\n"
        "notebook tab { background-color: #1e2429; color: #ffffff; padding: 2px 5px; }\n"
        "notebook tab:checked { background-color: #00ff88; color: #0b0f14; }\n"
        "notebook tab button { padding: 0; min-width: 20px; min-height: 20px; }\n"
        "#title-bar { background-color: #0b0f14; border-bottom: 2px solid #00ff88; }\n"
        "button { background-color: #1e2429; color: #00ff88; border: none; }\n"
        "button:hover { background-color: #2a323a; }\n"
        "scrollbar { background-color: #1e2429; }\n"
        "scrollbar slider { background-color: #00ff88; border-radius: 4px; min-width: 8px; min-height: 8px; }\n"
        "scrollbar slider:hover { background-color: #33ffaa; }\n"
        "scrollbar slider:active { background-color: #00cc66; }\n"
        "scrollbar trough { background-color: #2a323a; border-radius: 4px; }",
        -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
}

// Spawn callback
static void spawn_callback(VteTerminal *terminal, GPid pid, GError *error, gpointer user_data) 

{
    (void)terminal;
    (void)pid;
    (void)user_data;
    
    if (error) {
        g_warning("Failed to spawn shell: %s", error->message);
        g_error_free(error);
    }
}

// Close tab callback
static void close_tab_callback(GtkButton *button, gpointer data)

{
    GtkWidget *tab = g_object_get_data(G_OBJECT(button), "tab");
    Terminal *term = (Terminal *)data;
    if (tab) {
        int page_num = gtk_notebook_page_num(GTK_NOTEBOOK(term->notebook), tab);
        if (page_num != -1) {
            gtk_notebook_remove_page(GTK_NOTEBOOK(term->notebook), page_num);
        }
    }
}

// Create a new terminal tab
static void new_terminal_tab(const char *initial_directory, Terminal *term) 

{
    GtkWidget *vte = vte_terminal_new();
    GtkWidget *scrolled_window;

    // Set basic options
    vte_terminal_set_scrollback_lines(VTE_TERMINAL(vte), 10000);

    // Set colors to match theme
    GdkRGBA foreground = {0.0, 1.0, 0.53, 1.0}; // #00ff88
    GdkRGBA background = {0.04, 0.06, 0.08, 1.0}; // #0b0f14
    vte_terminal_set_colors(VTE_TERMINAL(vte), &foreground, &background, NULL, 0);

    // Spawn the shell
    const char *shell = vte_get_user_shell();
    if (!shell || *shell == '\0') shell = "/bin/bash";

    char *argv[] = { (char*)shell, NULL };

    vte_terminal_spawn_async(VTE_TERMINAL(vte),
        VTE_PTY_DEFAULT,
        initial_directory,
        argv,
        NULL,
        0,
        NULL, NULL, NULL,
        -1,
        NULL,
        spawn_callback,
        NULL);

    // Create scrolled window for the terminal
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled_window), vte);
    gtk_widget_show(vte);

    // Create tab with close button
    GtkWidget *tab_label_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *tab_label = gtk_label_new("Terminal");
    GtkWidget *close_btn = gtk_button_new_from_icon_name("window-close", GTK_ICON_SIZE_MENU);
    gtk_button_set_relief(GTK_BUTTON(close_btn), GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(close_btn, "Close tab");
    gtk_widget_set_size_request(close_btn, 20, 20);

    gtk_box_pack_start(GTK_BOX(tab_label_box), tab_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(tab_label_box), close_btn, FALSE, FALSE, 0);
    gtk_widget_show_all(tab_label_box);

    // Add to notebook 
    int page_num = gtk_notebook_append_page(GTK_NOTEBOOK(term->notebook), scrolled_window, tab_label_box);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(term->notebook), page_num);
    gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(term->notebook), scrolled_window, TRUE);

    // Connect close button
    g_object_set_data(G_OBJECT(close_btn), "tab", scrolled_window);
    g_signal_connect(close_btn, "clicked", G_CALLBACK(close_tab_callback), term);
}

// Window activate callback
static void activate(GtkApplication *app, gpointer user_data)

{
    Terminal *term = g_new0(Terminal, 1);
    term->is_dragging = 0;
    term->is_resizing = 0;
    term->resize_edge = RESIZE_NONE;

    GtkWidget *vbox;
    GtkWidget *title_bar;
    GtkWidget *title_label;
    GtkWidget *window_buttons;
    GtkWidget *min_btn;
    GtkWidget *max_btn;
    GtkWidget *close_btn;

    // Create main window
    GtkWidget *main_window = gtk_application_window_new(app);
    term->window = main_window;
    gtk_window_set_title(GTK_WINDOW(main_window), "Terminal");
    gtk_window_set_default_size(GTK_WINDOW(main_window), 800, 600);
    gtk_window_set_position(GTK_WINDOW(main_window), GTK_WIN_POS_CENTER);
    
    // Set undecorated to allow custom resize handling
    gtk_window_set_decorated(GTK_WINDOW(main_window), FALSE);

    // Enable events for dragging and resizing on the WINDOW itself
    gtk_widget_add_events(main_window, GDK_BUTTON_PRESS_MASK |
                                       GDK_BUTTON_RELEASE_MASK |
                                       GDK_POINTER_MOTION_MASK |
                                       GDK_ENTER_NOTIFY_MASK |
                                       GDK_LEAVE_NOTIFY_MASK);
    g_signal_connect(main_window, "button-press-event", G_CALLBACK(on_button_press), term);
    g_signal_connect(main_window, "button-release-event", G_CALLBACK(on_button_release), term);
    g_signal_connect(main_window, "motion-notify-event", G_CALLBACK(on_motion_notify), term);

    apply_css();

    // Main vertical box
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(main_window), vbox);

    // Custom title bar
    title_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_name(title_bar, "title-bar");
    gtk_widget_set_size_request(title_bar, -1, 30);
    gtk_box_pack_start(GTK_BOX(vbox), title_bar, FALSE, FALSE, 0);
    term->title_bar = title_bar; // Store for drag detection

    title_label = gtk_label_new("Terminal");
    gtk_label_set_xalign(GTK_LABEL(title_label), 0.0);
    gtk_box_pack_start(GTK_BOX(title_bar), title_label, TRUE, TRUE, 10);

    window_buttons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_pack_end(GTK_BOX(title_bar), window_buttons, FALSE, FALSE, 5);

    // Minimize button
    min_btn = gtk_button_new_with_label("─");
    gtk_widget_set_size_request(min_btn, 30, 25);
    g_signal_connect(min_btn, "clicked", G_CALLBACK(on_minimize_clicked), term->window);
    gtk_box_pack_start(GTK_BOX(window_buttons), min_btn, FALSE, FALSE, 0);

    // Maximize button
    max_btn = gtk_button_new_with_label("□");
    gtk_widget_set_size_request(max_btn, 30, 25);
    g_signal_connect(max_btn, "clicked", G_CALLBACK(on_maximize_clicked), term->window);
    g_signal_connect(main_window, "window-state-event", G_CALLBACK(on_window_state_changed), max_btn);
    gtk_box_pack_start(GTK_BOX(window_buttons), max_btn, FALSE, FALSE, 0);

    // Close button
    close_btn = gtk_button_new_with_label("✕");
    gtk_widget_set_size_request(close_btn, 30, 25);
    g_signal_connect(close_btn, "clicked", G_CALLBACK(on_close_clicked), term->window);
    gtk_box_pack_start(GTK_BOX(window_buttons), close_btn, FALSE, FALSE, 0);

    // Notebook for tabs
    GtkWidget *notebook = gtk_notebook_new();
    term->notebook = notebook;
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), TRUE);
    gtk_notebook_set_show_border(GTK_NOTEBOOK(notebook), FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);

    // Create first tab
    new_terminal_tab(NULL, term);

    gtk_window_set_resizable(GTK_WINDOW(main_window), TRUE);
    gtk_widget_show_all(main_window);
    g_object_set_data_full(G_OBJECT(main_window), "terminal", term, g_free);
}

int main(int argc, char **argv) 

{
    GtkApplication *app = gtk_application_new("org.blackline.terminal", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}