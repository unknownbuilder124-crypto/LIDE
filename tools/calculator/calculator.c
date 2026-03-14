#include "calculator.h"
#include "window_resize.h"

#include "calculator.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_E
#define M_E 2.71828182845904523536
#endif

// Dragging handlers
gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data)

{
    Calculator *calc = (Calculator *)data;

    if (event->button == 1) {
        // Check if cursor is on an edge (for resizing)
        calc->resize_edge = detect_resize_edge_absolute(GTK_WINDOW(calc->window), event->x_root, event->y_root);

        if (calc->resize_edge != RESIZE_NONE) {
            calc->is_resizing = 1;
        } else {
            calc->is_dragging = 1;
        }

        calc->drag_start_x = event->x_root;
        calc->drag_start_y = event->y_root;
        gtk_window_present(GTK_WINDOW(calc->window));
        return TRUE;
    }
    return FALSE;
}

gboolean on_button_release(GtkWidget *widget, GdkEventButton *event, gpointer data)

{
    Calculator *calc = (Calculator *)data;

    if (event->button == 1) {
        calc->is_dragging = 0;
        calc->is_resizing = 0;
        calc->resize_edge = RESIZE_NONE;
        return TRUE;
    }
    return FALSE;
}

gboolean on_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer data)

{
    Calculator *calc = (Calculator *)data;

    // Update cursor for resize hints
    if (!calc->is_dragging && !calc->is_resizing) {
        int resize_edge = detect_resize_edge_absolute(GTK_WINDOW(calc->window), event->x_root, event->y_root);
        update_resize_cursor(widget, resize_edge);
    }

    if (calc->is_resizing) {
        int delta_x = event->x_root - calc->drag_start_x;
        int delta_y = event->y_root - calc->drag_start_y;

        int window_width, window_height;
        gtk_window_get_size(GTK_WINDOW(calc->window), &window_width, &window_height);

        apply_window_resize(GTK_WINDOW(calc->window), calc->resize_edge,
                           delta_x, delta_y, window_width, window_height);

        calc->drag_start_x = event->x_root;
        calc->drag_start_y = event->y_root;
        return TRUE;
    } else if (calc->is_dragging) {
        int dx = event->x_root - calc->drag_start_x;
        int dy = event->y_root - calc->drag_start_y;

        int x, y;
        gtk_window_get_position(GTK_WINDOW(calc->window), &x, &y);
        gtk_window_move(GTK_WINDOW(calc->window), x + dx, y + dy);

        calc->drag_start_x = event->x_root;
        calc->drag_start_y = event->y_root;
        return TRUE;
    }
    return FALSE;
}

// Window control callbacks
static void on_minimize_clicked(GtkButton *button, gpointer data) 

{
    (void)button;
    Calculator *calc = (Calculator *)data;
    gtk_window_iconify(GTK_WINDOW(calc->window));
}

static void on_close_clicked(GtkButton *button, gpointer data) 

{
    (void)button;
    Calculator *calc = (Calculator *)data;
    gtk_window_close(GTK_WINDOW(calc->window));
}

// Calculator functions
static void update_display(Calculator *calc) 

{
    if (calc->error_state) {
        gtk_label_set_text(GTK_LABEL(calc->display), "Error");
    } else {
        gtk_label_set_text(GTK_LABEL(calc->display), calc->current_input);
    }
}

static void append_to_input(Calculator *calc, const char *text) 

{
    if (calc->error_state) {
        strcpy(calc->current_input, "");
        calc->error_state = FALSE;
    }
    
    if (calc->new_input) {
        strcpy(calc->current_input, text);
        calc->new_input = FALSE;
    } else {
        // Prevent multiple decimals
        if (strcmp(text, ".") == 0 && strchr(calc->current_input, '.') != NULL) {
            return;
        }
        // Prevent multiple zeros at start
        if (strcmp(calc->current_input, "0") == 0 && strcmp(text, "0") != 0 && strcmp(text, ".") != 0) {
            strcpy(calc->current_input, text);
        } else {
            strcat(calc->current_input, text);
        }
    }
    update_display(calc);
}

void number_clicked(GtkButton *button, gpointer data) 

{
    Calculator *calc = (Calculator *)data;
    const char *text = gtk_button_get_label(button);
    append_to_input(calc, text);
}

void operator_clicked(GtkButton *button, gpointer data) 

{
    Calculator *calc = (Calculator *)data;
    const char *op = gtk_button_get_label(button);
    
    if (calc->error_state) return;
    
    if (strlen(calc->current_input) > 0) {
        calc->last_value = strtod(calc->current_input, NULL);
        calc->last_operator = op[0];
        calc->new_input = TRUE;
    }
}

void function_clicked(GtkButton *button, gpointer data) 

{
    Calculator *calc = (Calculator *)data;
    const char *func = gtk_button_get_label(button);
    
    if (calc->error_state) return;
    
    double value = strtod(calc->current_input, NULL);
    double result = 0;
    
    if (strcmp(func, "1/x") == 0) {
        if (value != 0) result = 1.0 / value;
        else calc->error_state = TRUE;
    }
    else if (strcmp(func, "x²") == 0) result = value * value;
    else if (strcmp(func, "√") == 0) {
        if (value >= 0) result = sqrt(value);
        else calc->error_state = TRUE;
    }
    else if (strcmp(func, "sin") == 0) result = sin(value * M_PI / 180.0);
    else if (strcmp(func, "cos") == 0) result = cos(value * M_PI / 180.0);
    else if (strcmp(func, "tan") == 0) result = tan(value * M_PI / 180.0);
    else if (strcmp(func, "log") == 0) {
        if (value > 0) result = log10(value);
        else calc->error_state = TRUE;
    }
    else if (strcmp(func, "ln") == 0) {
        if (value > 0) result = log(value);
        else calc->error_state = TRUE;
    }
    else if (strcmp(func, "x^y") == 0) {
        // Store first number and wait for second
        calc->last_value = value;
        calc->last_operator = '^';
        calc->new_input = TRUE;
        return;
    }
    else if (strcmp(func, "π") == 0) {
        result = M_PI;
    }
    else if (strcmp(func, "e") == 0) {
        result = M_E;
    }
    
    if (!calc->error_state) {
        snprintf(calc->current_input, sizeof(calc->current_input), "%g", result);
    }
    update_display(calc);
}

void equals_clicked(GtkButton *button, gpointer data) 

{
    (void)button;
    Calculator *calc = (Calculator *)data;
    
    if (calc->error_state) return;
    
    double current = strtod(calc->current_input, NULL);
    double result = current;
    
    switch (calc->last_operator) {
        case '+': result = calc->last_value + current; break;
        case '-': result = calc->last_value - current; break;
        case '*': result = calc->last_value * current; break;
        case '/': 
            if (current != 0) result = calc->last_value / current;
            else calc->error_state = TRUE;
            break;
        case '^': result = pow(calc->last_value, current); break;
        default: break;
    }
    
    if (!calc->error_state) {
        // Save to history
        char history[512];
        if (calc->last_operator) {
            snprintf(history, sizeof(history), "%g %c %g = %g\n%s", 
                     calc->last_value, calc->last_operator, current, result,
                     gtk_label_get_text(GTK_LABEL(calc->history_display)));
        } else {
            snprintf(history, sizeof(history), "%g\n%s", result,
                     gtk_label_get_text(GTK_LABEL(calc->history_display)));
        }
        gtk_label_set_text(GTK_LABEL(calc->history_display), history);
        
        snprintf(calc->last_result, sizeof(calc->last_result), "%g", result);
        snprintf(calc->current_input, sizeof(calc->current_input), "%g", result);
        calc->new_input = TRUE;
        calc->last_operator = 0;
    }
    update_display(calc);
}

void clear_clicked(GtkButton *button, gpointer data) 

{
    (void)button;
    Calculator *calc = (Calculator *)data;
    
    strcpy(calc->current_input, "0");
    calc->last_value = 0;
    calc->last_operator = 0;
    calc->new_input = TRUE;
    calc->error_state = FALSE;
    update_display(calc);
}

void backspace_clicked(GtkButton *button, gpointer data) 

{
    (void)button;
    Calculator *calc = (Calculator *)data;
    
    if (calc->error_state) return;
    
    int len = strlen(calc->current_input);
    if (len > 1) {
        calc->current_input[len - 1] = '\0';
    } else {
        strcpy(calc->current_input, "0");
        calc->new_input = TRUE;
    }
    update_display(calc);
}

// FIXED MEMORY FUNCTIONS
void memory_clicked(GtkButton *button, gpointer data) 

{
    Calculator *calc = (Calculator *)data;
    const char *action = gtk_button_get_label(button);
    
    double current_val = strtod(calc->current_input, NULL);
    double mem_val = strtod(calc->memory, NULL);
    
    if (strcmp(action, "MC") == 0) {  // Memory Clear
        strcpy(calc->memory, "0");
        g_print("Memory cleared: %s\n", calc->memory); // Debug
    } 
    else if (strcmp(action, "MR") == 0) {  // Memory Recall
        snprintf(calc->current_input, sizeof(calc->current_input), "%g", mem_val);
        calc->new_input = TRUE;
        update_display(calc);
        g_print("Memory recalled: %s\n", calc->memory); // Debug
    } 
    else if (strcmp(action, "M+") == 0) {  // Memory Add
        double new_mem = mem_val + current_val;
        snprintf(calc->memory, sizeof(calc->memory), "%g", new_mem);
        calc->new_input = TRUE;
        g_print("Memory added: %s + %g = %s\n", calc->memory, current_val, calc->memory); // Debug
    } 
    else if (strcmp(action, "M-") == 0) {  // Memory Subtract
        double new_mem = mem_val - current_val;
        snprintf(calc->memory, sizeof(calc->memory), "%g", new_mem);
        calc->new_input = TRUE;
        g_print("Memory subtracted: %s - %g = %s\n", calc->memory, current_val, calc->memory); // Debug
    }
}

void toggle_mode(GtkButton *button, gpointer data) 

{
    Calculator *calc = (Calculator *)data;
    
    if (calc->mode == MODE_BASIC) {
        calc->mode = MODE_SCIENTIFIC;
        gtk_button_set_label(button, "Basic");
        g_print("Switched to Scientific mode\n"); // Debug
    } else {
        calc->mode = MODE_BASIC;
        gtk_button_set_label(button, "Scientific");
        g_print("Switched to Basic mode\n"); // Debug
    }
}

static void create_button_grid(Calculator *calc) 

{
    calc->button_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(calc->button_grid), 2);
    gtk_grid_set_column_spacing(GTK_GRID(calc->button_grid), 2);
    
    // Basic calculator buttons
    const char *basic_buttons[] = {
        "MC", "MR", "M+", "M-",
        "7", "8", "9", "/",
        "4", "5", "6", "*",
        "1", "2", "3", "-",
        "0", ".", "=", "+",
        "C", "⌫", "1/x", "x²"
    };
    
    int row = 0, col = 0;
    for (int i = 0; i < 24; i++) {
        GtkWidget *button = gtk_button_new_with_label(basic_buttons[i]);
        gtk_widget_set_size_request(button, 60, 40);
        
        // Connect signals based on button type
        if (strcmp(basic_buttons[i], "=") == 0) {
            g_signal_connect(button, "clicked", G_CALLBACK(equals_clicked), calc);
        }
        else if (strcmp(basic_buttons[i], "C") == 0) {
            g_signal_connect(button, "clicked", G_CALLBACK(clear_clicked), calc);
        }
        else if (strcmp(basic_buttons[i], "⌫") == 0) {
            g_signal_connect(button, "clicked", G_CALLBACK(backspace_clicked), calc);
        }
        else if (strchr("+-*/", basic_buttons[i][0]) && strlen(basic_buttons[i]) == 1) {
            g_signal_connect(button, "clicked", G_CALLBACK(operator_clicked), calc);
        }
        else if (strncmp(basic_buttons[i], "M", 1) == 0) {
            g_signal_connect(button, "clicked", G_CALLBACK(memory_clicked), calc);
        }
        else if (strcmp(basic_buttons[i], "1/x") == 0 || strcmp(basic_buttons[i], "x²") == 0) {
            g_signal_connect(button, "clicked", G_CALLBACK(function_clicked), calc);
        }
        else {
            g_signal_connect(button, "clicked", G_CALLBACK(number_clicked), calc);
        }
        
        gtk_grid_attach(GTK_GRID(calc->button_grid), button, col, row, 1, 1);
        
        col++;
        if (col > 3) {
            col = 0;
            row++;
        }
    }
}

static void activate(GtkApplication *app, gpointer user_data) 

{
    (void)user_data;
    Calculator *calc = g_new(Calculator, 1);
    memset(calc, 0, sizeof(Calculator));
    
    strcpy(calc->current_input, "0");
    strcpy(calc->memory, "0");  // Initialize memory to 0
    strcpy(calc->last_result, "0");
    calc->new_input = TRUE;
    calc->mode = MODE_BASIC;
    
    // Main window
    calc->window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(calc->window), "BlackLine Calculator");
    gtk_window_set_default_size(GTK_WINDOW(calc->window), 300, 450);
    gtk_window_set_position(GTK_WINDOW(calc->window), GTK_WIN_POS_CENTER);
    gtk_window_set_decorated(GTK_WINDOW(calc->window), TRUE);
    gtk_window_set_resizable(GTK_WINDOW(calc->window), TRUE);
    
    // Enable events for dragging
    gtk_widget_add_events(calc->window, GDK_BUTTON_PRESS_MASK | 
                                         GDK_BUTTON_RELEASE_MASK | 
                                         GDK_POINTER_MOTION_MASK);
    g_signal_connect(calc->window, "button-press-event", G_CALLBACK(on_button_press), calc);
    g_signal_connect(calc->window, "button-release-event", G_CALLBACK(on_button_release), calc);
    g_signal_connect(calc->window, "motion-notify-event", G_CALLBACK(on_motion_notify), calc);
    
    // Main vertical box
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(calc->window), vbox);
    
    // Custom title bar
    GtkWidget *title_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_name(title_bar, "title-bar");
    gtk_widget_set_size_request(title_bar, -1, 30);
    gtk_box_pack_start(GTK_BOX(vbox), title_bar, FALSE, FALSE, 0);
    
    GtkWidget *title_label = gtk_label_new("BlackLine Calculator");
    gtk_label_set_xalign(GTK_LABEL(title_label), 0.0);
    gtk_box_pack_start(GTK_BOX(title_bar), title_label, TRUE, TRUE, 10);
    
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_pack_end(GTK_BOX(title_bar), button_box, FALSE, FALSE, 5);
    
    GtkWidget *min_btn = gtk_button_new_with_label("─");
    gtk_widget_set_size_request(min_btn, 30, 25);
    g_signal_connect(min_btn, "clicked", G_CALLBACK(on_minimize_clicked), calc);
    gtk_box_pack_start(GTK_BOX(button_box), min_btn, FALSE, FALSE, 0);
    
    GtkWidget *close_btn = gtk_button_new_with_label("✕");
    gtk_widget_set_size_request(close_btn, 30, 25);
    g_signal_connect(close_btn, "clicked", G_CALLBACK(on_close_clicked), calc);
    gtk_box_pack_start(GTK_BOX(button_box), close_btn, FALSE, FALSE, 0);
    
    // Separator
    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), sep, FALSE, FALSE, 0);
    
    // Display area
    GtkWidget *display_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), display_box, FALSE, FALSE, 10);
    
    // History display
    calc->history_display = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(calc->history_display), 1.0);
    gtk_widget_set_margin_start(calc->history_display, 10);
    gtk_widget_set_margin_end(calc->history_display, 10);
    gtk_box_pack_start(GTK_BOX(display_box), calc->history_display, FALSE, FALSE, 0);
    
    // Main display
    calc->display = gtk_label_new("0");
    gtk_label_set_xalign(GTK_LABEL(calc->display), 1.0);
    gtk_widget_set_margin_start(calc->display, 10);
    gtk_widget_set_margin_end(calc->display, 10);
    gtk_widget_set_margin_top(calc->display, 5);
    gtk_widget_set_margin_bottom(calc->display, 5);
    
    // Style the display
    GtkWidget *display_frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(display_frame), calc->display);
    gtk_box_pack_start(GTK_BOX(display_box), display_frame, FALSE, FALSE, 0);
    
    // Mode toggle button
    calc->mode_button = gtk_button_new_with_label("Scientific");
    g_signal_connect(calc->mode_button, "clicked", G_CALLBACK(toggle_mode), calc);
    gtk_box_pack_start(GTK_BOX(vbox), calc->mode_button, FALSE, FALSE, 5);
    
    // Button grid
    create_button_grid(calc);
    gtk_box_pack_start(GTK_BOX(vbox), calc->button_grid, TRUE, TRUE, 5);
    
    // css
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "window { background-color: #000000; color: #ffffff; }\n"
        "label { color: #ffffff; }\n"
        "button { background-color: #1a1a1a; color: #00ff88; border: 1px solid #00ff88; }\n"
        "button:hover { background-color: #333333; }\n"
        "#title-bar { background-color: #000000; border-bottom: 2px solid #00ff88; }\n"
        "frame { border: 2px solid #00ff88; }\n"
        "frame > label { background-color: #1a1a1a; padding: 10px; font-size: 24px; }\n",
        -1, NULL);
    
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
    
    gtk_widget_show_all(calc->window);
    g_object_set_data_full(G_OBJECT(calc->window), "calculator", calc, g_free);
}

int main(int argc, char **argv) 

{
    GtkApplication *app = gtk_application_new("org.blackline.calculator", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}