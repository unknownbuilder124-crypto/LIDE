#include "fm.h"
#include "window_resize.h"

// Dragging functions
static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data)

{
    FileManager *fm = (FileManager *)data;

    if (event->button == 1)
    {
        // Check if cursor is on an edge (for resizing)
        fm->resize_edge = detect_resize_edge_absolute(GTK_WINDOW(fm->window), event->x_root, event->y_root);

        if (fm->resize_edge != RESIZE_NONE) {
            fm->is_resizing = 1;
        } else {
            fm->is_dragging = 1;
        }

        fm->drag_start_x = event->x_root;
        fm->drag_start_y = event->y_root;
        gtk_window_present(GTK_WINDOW(fm->window));
        return TRUE;
    }
    return FALSE;
}

static gboolean on_button_release(GtkWidget *widget, GdkEventButton *event, gpointer data)

{
    FileManager *fm = (FileManager *)data;

    if (event->button == 1) {
        fm->is_dragging = 0;
        fm->is_resizing = 0;
        fm->resize_edge = RESIZE_NONE;
        return TRUE;
    }
    return FALSE;
}

static gboolean on_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer data)

{
    FileManager *fm = (FileManager *)data;

    // Update cursor for resize hints
    if (!fm->is_dragging && !fm->is_resizing) {
        int resize_edge = detect_resize_edge_absolute(GTK_WINDOW(fm->window), event->x_root, event->y_root);
        update_resize_cursor(widget, resize_edge);
    }

    if (fm->is_resizing) {
        int delta_x = event->x_root - fm->drag_start_x;
        int delta_y = event->y_root - fm->drag_start_y;

        int window_width, window_height;
        gtk_window_get_size(GTK_WINDOW(fm->window), &window_width, &window_height);

        apply_window_resize(GTK_WINDOW(fm->window), fm->resize_edge,
                           delta_x, delta_y, window_width, window_height);

        fm->drag_start_x = event->x_root;
        fm->drag_start_y = event->y_root;
        return TRUE;
    } else if (fm->is_dragging) {
        int dx = event->x_root - fm->drag_start_x;
        int dy = event->y_root - fm->drag_start_y;

        int x, y;
        gtk_window_get_position(GTK_WINDOW(fm->window), &x, &y);
        gtk_window_move(GTK_WINDOW(fm->window), x + dx, y + dy);

        fm->drag_start_x = event->x_root;
        fm->drag_start_y = event->y_root;
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

static void activate(GtkApplication *app, gpointer user_data) 

{
    FileManager *fm = g_new(FileManager, 1);
    fm->history = NULL;
    fm->history_pos = NULL;
    fm->current_dir = NULL;

    // Main window
    fm->window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(fm->window), "Blackline File Manager");
    gtk_window_set_default_size(GTK_WINDOW(fm->window), 900, 600);
    gtk_window_set_position(GTK_WINDOW(fm->window), GTK_WIN_POS_CENTER);
    gtk_container_set_border_width(GTK_CONTAINER(fm->window), 0);
    gtk_window_set_decorated(GTK_WINDOW(fm->window), TRUE);
    gtk_window_set_resizable(GTK_WINDOW(fm->window), TRUE);
    
    // Enable events for dragging 
    gtk_widget_add_events(fm->window, GDK_BUTTON_PRESS_MASK | 
                                       GDK_BUTTON_RELEASE_MASK | 
                                       GDK_POINTER_MOTION_MASK);
    
    // Connect drag signals to the window itself
    g_signal_connect(fm->window, "button-press-event", G_CALLBACK(on_button_press), fm);
    g_signal_connect(fm->window, "button-release-event", G_CALLBACK(on_button_release), fm);
    g_signal_connect(fm->window, "motion-notify-event", G_CALLBACK(on_motion_notify), fm);

    // Apply CSS for professional dark theme with blue accents
    GtkCssProvider *provider = gtk_css_provider_new();

    gtk_css_provider_load_from_data(provider,
        "window { background-color: #1e1e1e; color: #e0e0e0; }\n"
        "treeview { background-color: #252525; color: #e0e0e0; }\n"
        "treeview:selected { background-color: #0d6efd; color: #ffffff; }\n"
        "treeview:selected:focus { background-color: #0b5ed7; }\n"
        "entry { background-color: #2d2d2d; color: #e0e0e0; border: 1px solid #404040; }\n"
        "entry:focus { border-color: #0d6efd; }\n"
        "button { background-color: #2d2d2d; color: #e0e0e0; border: 1px solid #404040; }\n"
        "button:hover { background-color: #3d3d3d; border-color: #0d6efd; }\n"
        "button:active { background-color: #1e1e1e; }\n"
        "statusbar { background-color: #1a1a1a; color: #e0e0e0; border-top: 1px solid #404040; }\n"
        "scrolledwindow { border: 1px solid #404040; }\n",
        -1, NULL);

    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);

    // Main vertical box
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(fm->window), vbox);

    // Title bar with minimize, maximize and close buttons
    GtkWidget *title_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_name(title_bar, "title-bar");
    gtk_widget_set_size_request(title_bar, -1, 30);
    gtk_box_pack_start(GTK_BOX(vbox), title_bar, FALSE, FALSE, 0);

    // Title text
    GtkWidget *title_label = gtk_label_new("Blackline File Manager");
    gtk_label_set_xalign(GTK_LABEL(title_label), 0.0);
    gtk_box_pack_start(GTK_BOX(title_bar), title_label, TRUE, TRUE, 10);

    // Button box for window controls
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_pack_end(GTK_BOX(title_bar), button_box, FALSE, FALSE, 5);

    // Minimize button
    GtkWidget *min_btn = gtk_button_new_with_label("─");
    gtk_widget_set_size_request(min_btn, 30, 25);
    g_signal_connect(min_btn, "clicked", G_CALLBACK(on_minimize_clicked), fm->window);
    gtk_box_pack_start(GTK_BOX(button_box), min_btn, FALSE, FALSE, 0);

    // Maximize button
    GtkWidget *max_btn = gtk_button_new_with_label("□");
    gtk_widget_set_size_request(max_btn, 30, 25);
    g_signal_connect(max_btn, "clicked", G_CALLBACK(on_maximize_clicked), fm->window);
    // Connect window state changes to update button appearance
    g_signal_connect(fm->window, "window-state-event", G_CALLBACK(on_window_state_changed), max_btn);
    gtk_box_pack_start(GTK_BOX(button_box), max_btn, FALSE, FALSE, 0);

    // Close button
    GtkWidget *close_btn = gtk_button_new_with_label("✕");
    gtk_widget_set_size_request(close_btn, 30, 25);
    g_signal_connect(close_btn, "clicked", G_CALLBACK(on_close_clicked), fm->window);
    gtk_box_pack_start(GTK_BOX(button_box), close_btn, FALSE, FALSE, 0);

    // Separator
    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), sep, FALSE, FALSE, 0);

    // Toolbar
    GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 5);

    fm->back_button = gtk_button_new_with_label("←");
    gtk_box_pack_start(GTK_BOX(toolbar), fm->back_button, FALSE, FALSE, 0);
    g_signal_connect_swapped(fm->back_button, "clicked", G_CALLBACK(fm_go_back), fm);

    fm->forward_button = gtk_button_new_with_label("→");
    gtk_box_pack_start(GTK_BOX(toolbar), fm->forward_button, FALSE, FALSE, 0);
    g_signal_connect_swapped(fm->forward_button, "clicked", G_CALLBACK(fm_go_forward), fm);

    fm->up_button = gtk_button_new_with_label("↑");
    gtk_box_pack_start(GTK_BOX(toolbar), fm->up_button, FALSE, FALSE, 0);
    g_signal_connect_swapped(fm->up_button, "clicked", G_CALLBACK(fm_go_up), fm);

    fm->home_button = gtk_button_new_with_label("🏠");
    gtk_box_pack_start(GTK_BOX(toolbar), fm->home_button, FALSE, FALSE, 0);
    g_signal_connect_swapped(fm->home_button, "clicked", G_CALLBACK(fm_go_home), fm);

    fm->refresh_button = gtk_button_new_with_label("↻");
    gtk_box_pack_start(GTK_BOX(toolbar), fm->refresh_button, FALSE, FALSE, 0);
    g_signal_connect_swapped(fm->refresh_button, "clicked", G_CALLBACK(fm_refresh), fm);

    // Location entry
    fm->location_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(toolbar), fm->location_entry, TRUE, TRUE, 0);
    g_signal_connect_swapped(fm->location_entry, "activate", G_CALLBACK(fm_on_location_activate), fm);

    // Horizontal paned for sidebar and main view
    GtkWidget *hpaned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), hpaned, TRUE, TRUE, 5);

    // Sidebar (tree view)
    GtkWidget *sidebar_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sidebar_scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_paned_pack1(GTK_PANED(hpaned), sidebar_scroll, FALSE, FALSE);

    fm->sidebar_store = gtk_list_store_new(1, G_TYPE_STRING);
    fm->sidebar_tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(fm->sidebar_store));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(fm->sidebar_tree), FALSE);
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(fm->sidebar_tree), -1, "Places", renderer, "text", 0, NULL);
    gtk_container_add(GTK_CONTAINER(sidebar_scroll), fm->sidebar_tree);
    g_signal_connect(fm->sidebar_tree, "row-activated", G_CALLBACK(fm_on_sidebar_row_activated), fm);
    fm_populate_sidebar(fm);

    // Main view (tree view)
    GtkWidget *main_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(main_scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_paned_pack2(GTK_PANED(hpaned), main_scroll, TRUE, TRUE);

    // Create columns: Name, Size, Type, Modified
    fm->main_store = gtk_tree_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    fm->main_tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(fm->main_store));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(fm->main_tree), TRUE);

    renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *col = gtk_tree_view_column_new_with_attributes("Name", renderer, "text", 0, NULL);
    gtk_tree_view_column_set_sort_column_id(col, 0);
    gtk_tree_view_append_column(GTK_TREE_VIEW(fm->main_tree), col);

    renderer = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("Size", renderer, "text", 1, NULL);
    gtk_tree_view_column_set_sort_column_id(col, 1);
    gtk_tree_view_append_column(GTK_TREE_VIEW(fm->main_tree), col);

    renderer = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("Type", renderer, "text", 2, NULL);
    gtk_tree_view_column_set_sort_column_id(col, 2);
    gtk_tree_view_append_column(GTK_TREE_VIEW(fm->main_tree), col);

    renderer = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("Modified", renderer, "text", 3, NULL);
    gtk_tree_view_column_set_sort_column_id(col, 3);
    gtk_tree_view_append_column(GTK_TREE_VIEW(fm->main_tree), col);

    gtk_container_add(GTK_CONTAINER(main_scroll), fm->main_tree);
    g_signal_connect(fm->main_tree, "row-activated", G_CALLBACK(fm_on_row_activated), fm);

    // Status bar
    fm->status_label = gtk_label_new("Ready");
    gtk_label_set_xalign(GTK_LABEL(fm->status_label), 0);
    gtk_box_pack_start(GTK_BOX(vbox), fm->status_label, FALSE, FALSE, 2);

    // Set home directory as initial
    const gchar *home = g_get_home_dir();
    fm_open_location(fm, home);

    gtk_widget_show_all(fm->window);
    g_object_set_data_full(G_OBJECT(fm->window), "fm", fm, g_free);
}

int main(int argc, char **argv) 

{
    GtkApplication *app = gtk_application_new("org.blackline.filemanager", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}