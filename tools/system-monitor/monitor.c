#include "monitor.h"
#include "window_resize.h"

// Data history for graphs 
double cpu_history[HISTORY_SIZE] = {0};
int cpu_history_index = 0;
double mem_history[HISTORY_SIZE] = {0};
int mem_history_index = 0;

static CpuData cpu_data = {0};
static MemData mem_data = {0};

// Dragging handlers
static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data)

{
    Monitor *mon = (Monitor *)data;

    if (event->button == 1)
    {
        // Check if cursor is on an edge (for resizing)
        mon->resize_edge = detect_resize_edge_absolute(GTK_WINDOW(mon->window), event->x_root, event->y_root);

        if (mon->resize_edge != RESIZE_NONE) {
            mon->is_resizing = 1;
        } else {
            mon->is_dragging = 1;
        }

        mon->drag_start_x = event->x_root;
        mon->drag_start_y = event->y_root;
        gtk_window_present(GTK_WINDOW(mon->window));
        return TRUE;
    }
    return FALSE;
}

static gboolean on_button_release(GtkWidget *widget, GdkEventButton *event, gpointer data)

{
    Monitor *mon = (Monitor *)data;

    if (event->button == 1) {
        mon->is_dragging = 0;
        mon->is_resizing = 0;
        mon->resize_edge = RESIZE_NONE;
        return TRUE;
    }
    return FALSE;
}

static gboolean on_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer data)

{
    Monitor *mon = (Monitor *)data;

    // Update cursor for resize hints
    if (!mon->is_dragging && !mon->is_resizing) {
        int resize_edge = detect_resize_edge_absolute(GTK_WINDOW(mon->window), event->x_root, event->y_root);
        update_resize_cursor(widget, resize_edge);
    }

    if (mon->is_resizing) {
        int delta_x = event->x_root - mon->drag_start_x;
        int delta_y = event->y_root - mon->drag_start_y;

        int window_width, window_height;
        gtk_window_get_size(GTK_WINDOW(mon->window), &window_width, &window_height);

        apply_window_resize(GTK_WINDOW(mon->window), mon->resize_edge,
                           delta_x, delta_y, window_width, window_height);

        mon->drag_start_x = event->x_root;
        mon->drag_start_y = event->y_root;
        return TRUE;
    } else if (mon->is_dragging) {
        int dx = event->x_root - mon->drag_start_x;
        int dy = event->y_root - mon->drag_start_y;

        int x, y;
        gtk_window_get_position(GTK_WINDOW(mon->window), &x, &y);
        gtk_window_move(GTK_WINDOW(mon->window), x + dx, y + dy);

        mon->drag_start_x = event->x_root;
        mon->drag_start_y = event->y_root;
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

// Update all data
static gboolean update_data(gpointer user_data) {
    Monitor *mon = user_data;

    // CPU
    update_cpu_usage(&cpu_data);
    cpu_history[cpu_history_index] = cpu_data.usage;
    cpu_history_index = (cpu_history_index + 1) % HISTORY_SIZE;

    // Memory
    update_mem_usage(&mem_data);
    mem_history[mem_history_index] = mem_data.percent;
    mem_history_index = (mem_history_index + 1) % HISTORY_SIZE;

    // Redraw graphs
    gtk_widget_queue_draw(mon->cpu_da);
    gtk_widget_queue_draw(mon->mem_da);

    return G_SOURCE_CONTINUE;
}

// Process list update (separate timer)
static gboolean update_process_list(gpointer user_data) {
    GtkListStore *store = GTK_LIST_STORE(user_data);
    gtk_list_store_clear(store);

    GList *processes = get_process_list();
    for (GList *l = processes; l != NULL; l = l->next) {
        ProcessEntry *p = l->data;
        GtkTreeIter iter;
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
            0, p->pid,
            1, p->name,
            2, p->cpu_percent,
            3, p->mem_percent,
            -1);
    }
    free_process_list(processes);

    return G_SOURCE_CONTINUE;
}

static void activate(GtkApplication *app, gpointer user_data) {
    Monitor *mon = g_new(Monitor, 1);
    mon->is_dragging = 0;
    mon->is_resizing = 0;
    mon->resize_edge = RESIZE_NONE;

    GtkWidget *window = gtk_application_window_new(app);
    mon->window = window;
    gtk_window_set_title(GTK_WINDOW(window), "BlackLine System Monitor");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 500);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

    // Enable events for dragging
    gtk_widget_add_events(window, GDK_BUTTON_PRESS_MASK |
                                   GDK_BUTTON_RELEASE_MASK |
                                   GDK_POINTER_MOTION_MASK);
    g_signal_connect(window, "button-press-event", G_CALLBACK(on_button_press), mon);
    g_signal_connect(window, "button-release-event", G_CALLBACK(on_button_release), mon);
    g_signal_connect(window, "motion-notify-event", G_CALLBACK(on_motion_notify), mon);

    // Main vertical box
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // Custom title bar with controls
    GtkWidget *title_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_name(title_bar, "title-bar");
    gtk_widget_set_size_request(title_bar, -1, 30);
    gtk_box_pack_start(GTK_BOX(vbox), title_bar, FALSE, FALSE, 0);

    GtkWidget *title_label = gtk_label_new("BlackLine System Monitor");
    gtk_label_set_xalign(GTK_LABEL(title_label), 0.0);
    gtk_box_pack_start(GTK_BOX(title_bar), title_label, TRUE, TRUE, 10);

    GtkWidget *window_buttons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_pack_end(GTK_BOX(title_bar), window_buttons, FALSE, FALSE, 5);

    // Minimize button
    GtkWidget *min_btn = gtk_button_new_with_label("─");
    gtk_widget_set_size_request(min_btn, 30, 25);
    g_signal_connect(min_btn, "clicked", G_CALLBACK(on_minimize_clicked), window);
    gtk_box_pack_start(GTK_BOX(window_buttons), min_btn, FALSE, FALSE, 0);

    // Maximize button - store reference for state tracking
    GtkWidget *max_btn = gtk_button_new_with_label("□");
    gtk_widget_set_size_request(max_btn, 30, 25);
    g_signal_connect(max_btn, "clicked", G_CALLBACK(on_maximize_clicked), window);
    // Connect window state changes to update button appearance
    g_signal_connect(window, "window-state-event", G_CALLBACK(on_window_state_changed), max_btn);
    gtk_box_pack_start(GTK_BOX(window_buttons), max_btn, FALSE, FALSE, 0);

    // Close button
    GtkWidget *close_btn = gtk_button_new_with_label("✕");
    gtk_widget_set_size_request(close_btn, 30, 25);
    g_signal_connect(close_btn, "clicked", G_CALLBACK(on_close_clicked), window);
    gtk_box_pack_start(GTK_BOX(window_buttons), close_btn, FALSE, FALSE, 0);

    // Separator
    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), sep, FALSE, FALSE, 0);

    // Notebook for tabs (rest of the content)
    GtkWidget *notebook = gtk_notebook_new();
    gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 5);

    // --- System tab ---
    GtkWidget *sys_tab = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(sys_tab), 10);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), sys_tab, gtk_label_new("System"));

    // CPU graph
    GtkWidget *cpu_frame = gtk_frame_new("CPU Usage");
    GtkWidget *cpu_da = gtk_drawing_area_new();
    mon->cpu_da = cpu_da;
    gtk_widget_set_size_request(cpu_da, -1, 150);
    g_signal_connect(cpu_da, "draw", G_CALLBACK(draw_cpu_graph), NULL);
    gtk_container_add(GTK_CONTAINER(cpu_frame), cpu_da);
    gtk_box_pack_start(GTK_BOX(sys_tab), cpu_frame, TRUE, TRUE, 0);

    // Memory bar
    GtkWidget *mem_frame = gtk_frame_new("Memory Usage");
    GtkWidget *mem_da = gtk_drawing_area_new();
    mon->mem_da = mem_da;
    gtk_widget_set_size_request(mem_da, -1, 50);
    g_signal_connect(mem_da, "draw", G_CALLBACK(draw_mem_bar), NULL);
    gtk_container_add(GTK_CONTAINER(mem_frame), mem_da);
    gtk_box_pack_start(GTK_BOX(sys_tab), mem_frame, FALSE, FALSE, 0);

    // Disk usage (root partition)
    struct statvfs stat;
    if (statvfs("/", &stat) == 0) {
        unsigned long total = stat.f_blocks * stat.f_frsize;
        unsigned long free = stat.f_bfree * stat.f_frsize;
        unsigned long used = total - free;
        double percent = 100.0 * used / total;
        char disk_info[256];
        snprintf(disk_info, sizeof(disk_info), "Disk (/): %.1f GB used / %.1f GB total (%.1f%%)",
                 used / 1e9, total / 1e9, percent);
        GtkWidget *disk_label = gtk_label_new(disk_info);
        gtk_box_pack_start(GTK_BOX(sys_tab), disk_label, FALSE, FALSE, 0);
    }

    // --- Processes tab ---
    GtkWidget *proc_tab = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(proc_tab), 5);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), proc_tab, gtk_label_new("Processes"));

    // Tree view for processes
    GtkListStore *store = gtk_list_store_new(4, G_TYPE_INT, G_TYPE_STRING, G_TYPE_DOUBLE, G_TYPE_DOUBLE);
    GtkWidget *tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    g_object_unref(store);

    // Columns
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("PID", renderer, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Name", renderer, "text", 1, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("CPU %", renderer, "text", 2, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Memory %", renderer, "text", 3, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

    // Scrolled window
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled), tree);
    gtk_box_pack_start(GTK_BOX(proc_tab), scrolled, TRUE, TRUE, 0);

    //CSS
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "window { background-color: #000000; color: #ffffff; }"
        "#title-bar { background-color: #000000; border-bottom: 2px solid #86559a; }"
        "frame { border-color: #86559a; color: #ffffff; }"
        "frame > label { color: #86559a; }"
        "treeview { background-color: #1a1a1a; color: #ffffff; }"
        "treeview:selected { background-color: #86559a; color: #000000; }"
        "treeview.view { border-color: #86559a; }"
        "button { background-color: #1a1a1a; color: #86559a; border: 1px solid #86559a; }"
        "button:hover { background-color: #333333; }"
        "label { color: #ffffff; }"
        , -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    gtk_window_set_decorated(GTK_WINDOW(window), TRUE);
    gtk_window_set_resizable(GTK_WINDOW(window), TRUE);
    gtk_widget_show_all(window);

    // Start update timers
    g_timeout_add_seconds(2, update_data, mon);
    g_timeout_add_seconds(3, update_process_list, store);
    g_object_set_data_full(G_OBJECT(window), "monitor", mon, g_free);
}

int main(int argc, char **argv) 

{
    GtkApplication *app = gtk_application_new("org.blackline.systemmonitor", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}