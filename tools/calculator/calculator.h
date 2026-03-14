#ifndef BLACKLINE_CALCULATOR_H
#define BLACKLINE_CALCULATOR_H

#include <gtk/gtk.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_E
#define M_E 2.71828182845904523536
#endif

typedef enum {
    MODE_BASIC,
    MODE_SCIENTIFIC
} CalculatorMode;

typedef struct {
    GtkWidget *window;
    GtkWidget *display;
    GtkWidget *history_display;
    GtkWidget *mode_button;
    GtkWidget *button_grid;

    char current_input[256];
    char last_result[64];
    char memory[64];
    double last_value;
    char last_operator;
    gboolean new_input;
    gboolean error_state;
    CalculatorMode mode;

    // For dragging
    int is_dragging;
    int drag_start_x;
    int drag_start_y;

    // For resizing
    int is_resizing;
    int resize_edge;
} Calculator;

// Function prototypes
void number_clicked(GtkButton *button, gpointer data);
void operator_clicked(GtkButton *button, gpointer data);
void function_clicked(GtkButton *button, gpointer data);
void equals_clicked(GtkButton *button, gpointer data);
void clear_clicked(GtkButton *button, gpointer data);
void backspace_clicked(GtkButton *button, gpointer data);
void memory_clicked(GtkButton *button, gpointer data);
void toggle_mode(GtkButton *button, gpointer data);

// Dragging functions
gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data);
gboolean on_button_release(GtkWidget *widget, GdkEventButton *event, gpointer data);
gboolean on_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer data);

#endif